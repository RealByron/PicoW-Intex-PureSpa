#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "hardware/pio.h"
#include <stdint.h>
#include "purespa_ctrl.h"

namespace esphome {
namespace purespa {

#include "sbh20.pio.h"
static const char *const TAG = "SBH20";

#define BASE_PIN 3

// #define DEBUG_LOW_LEVEL_SPA
#define DEBUG_PIN 6

#define PRESS_DURATION 100

PIO pio = pio0;
uint offset;
uint sm;

SystemStatus status;
volatile uint8_t setpoint = 0;
volatile uint8_t blink_count = 0;

void IRQ_handler();
void sbh20_program_init(PIO pio, uint sm, uint offset, uint pin);
void short_press(uint8_t pb, uint16_t duration);
void pb_press(uint8_t bp);

uint32_t elapsed() {
  static uint32_t before = millis();
  uint32_t now = millis();
  uint32_t elapsed = now - before;
  before = now;
  return elapsed;
}

void SBH20::setup() {
  pio = pio0;
  offset = pio_add_program(pio, &sbh20_program);
  sm = pio_claim_unused_sm(pio, true);

#ifdef DEBUG_LOW_LEVEL_SPA
  gpio_init(DEBUG_PIN);
  gpio_set_dir(DEBUG_PIN, true);
#endif

  sbh20_program_init(pio, sm, offset, BASE_PIN);

  delay(1000);

  if (!(status.leds & LED(POWER))) {
    short_press(POWER, 1000);
  }
  if ((status.target_temperature == 0)) {
    short_press(DOWN, PRESS_DURATION);
  }
}

bool SBH20::loop() {
  static uint16_t pb_action = RELEASE;
  uint8_t target = status.target_temperature;

  if (setpoint == 0 || !(status.leds & LED(POWER)))
    return true;

  uint16_t pb = (setpoint == target) ? RELEASE : (setpoint > target) ? UP : DOWN;

  if (blink_count == 0 && pb != RELEASE) {
    ESP_LOGD("PIO", "Wait blink {%d}", elapsed());
    short_press(pb, PRESS_DURATION);
    uint32_t now = millis();
    while (((millis() - now) < 400) && blink_count == 0) {
    }
    ESP_LOGD("PIO", "Blinking {%d}", elapsed());
  }

  if (pb == pb_action)
    // Setpint not reached yet
    // nothing to do expect avoid component calls
    return false;

  pb_press(pb);
  pb_action = pb;

  if (setpoint != target) {
    ESP_LOGD("PIO", "Target %d != %d: Push %s {%d}", setpoint, target, NAME(pb), elapsed());
    blink_count = 1;
  } else {
    ESP_LOGI("SBH20", "Target temp %d reached {%d}", target, elapsed());
    setpoint = 0;
    blink_count = 10;
  }

  return false;
}

void SBH20::set_target_temperature(uint8_t temp) {
  if (temp >= 20 && temp <= 40) {
    ESP_LOGI("SBH20", "Set target to %d  {%d}", temp, elapsed());
    setpoint = temp;
  }
}

bool get_state(uint16_t led) { return (bool) (status.leds & led); }
void SBH20::set_state(uint8_t led, bool state) {
  ESP_LOGI("PIO", "Set state %d %s", led, state ? "On" : "Off");

  switch (led) {
    case HEATER:
      if ((bool) (status.leds & (LED(HEATER) | LED(STANDBY))) != state)
        short_press(led, PRESS_DURATION);
      break;
    default:
      if ((bool) (status.leds & LED(led)) != state)
        short_press(led, PRESS_DURATION);
      break;
  }
}

SystemStatus SBH20::get_status() { return status; }
uint8_t SBH20::getErrorValue() const { return status.error; }

void pb_press(uint8_t pb) {
  ESP_LOGVV("SBH20", "PB pressed %s", NAME(pb));
  pio_sm_put_blocking(pio, sm, (uint16_t) ~PB(pb));
  pio_sm_exec(pio, sm, pio_encode_jmp(offset + sbh20_offset_set_x));
}

void short_press(uint8_t pb, uint16_t duration) {
  ESP_LOGV("SBH20", "Short PB %s", NAME(pb), duration);

  pb_press(pb);
  delay(duration);
  pb_press(RELEASE);
}

void sbh20_program_init(PIO pio, uint sm, uint offset, uint pin) {
  uint data_pin = pin;
  uint clk_pin = pin + 1;
  uint cs_pin = pin + 2;

  pio_sm_config c = sbh20_program_get_default_config(offset);
  sm_config_set_in_pins(&c, data_pin);
  sm_config_set_set_pins(&c, data_pin, 1);
  sm_config_set_in_shift(&c, false, false, 16);
  sm_config_set_out_shift(&c, true, false, 16);

  pio_gpio_init(pio, data_pin);
  pio_gpio_init(pio, clk_pin);
  pio_gpio_init(pio, cs_pin);

  gpio_put(data_pin, false);

  pio_sm_set_consecutive_pindirs(pio, sm, data_pin, 3, false);
  pio_sm_clear_fifos(pio, sm);

  pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
  irq_set_exclusive_handler(PIO0_IRQ_0, IRQ_handler);
  irq_set_enabled(PIO0_IRQ_0, true);

#ifdef DEBUG_LOW_LEVEL_SPA
  pio_gpio_init(pio, DEBUG_PIN);
  gpio_put(DEBUG_PIN, false);
  sm_config_set_sideset_pins(&c, DEBUG_PIN);
  pio_sm_set_consecutive_pindirs(pio, sm, DEBUG_PIN, 1, true);
#endif

  pio_sm_init(pio, sm, offset, &c);
  pio_sm_set_enabled(pio, sm, true);
}

// Definitions
#define MAX_COUNT 150

#define DIGITS_SIZE 4
#define DIGIT_POS_MASK 0x0864
const uint16_t digitPos[DIGITS_SIZE] = {0x0040, 0x0020, 0x0800, 0x0004};

#define DIGITS 0b0011011010011000  // All Segments displayed
#define NB_DIGITS 16
const struct {
  uint8_t value;
  uint16_t bin;
} digits[NB_DIGITS] = {{0, 0b0011011010001000},  //    bit positions
                       {1, 0b0001001000000000},  //      ---13---
                       {2, 0b0011010010010000},  //     |        |
                       {3, 0b0011011000010000},  //     3       12
                       {4, 0b0001001000011000},  //     |        |
                       {5, 0b0010011000011000},  //      ----4---
                       {6, 0b0010011010011000},  //     |        |
                       {7, 0b0011001000000000},  //     7        9
                       {8, 0b0011011010011000},  //     |        |
                       {9, 0b0011011000011000},  //      ---10---
                       {0xA, 0b0011001010011000}, {0xC, 0b0010010010001000},
                       {0xD, 0b0001011010010000}, {0xE, 0b0010010010011000},
                       {0xF, 0b0010000010011000}, {0xB, 0b0000000000000000}};  // use 0xB for Blank digit

uint8_t bcd_to_digit(uint16_t value) {
  for (int i = 0; i < NB_DIGITS; i++) {
    if (digits[i].bin == (value & DIGITS)) {
      return digits[i].value;
    }
  }
  return 0xB;  // return Blank digit
}

uint8_t convert_to_decimal(uint16_t value) {
  uint16_t ret = value >> 4;
  ret = (ret >> 8) * 100 + ((ret & 0xf0) >> 4) * 10 + (ret & 0xf);

  if ((value & 0xf) == 0xF) {
    return (uint8_t) (((ret - 32) * 5.) / 9.);
  }
  return ret;
}

bool decode_display(uint16_t value) {
  static uint16_t raw_display = 0;
  static uint16_t last_display = 0;
  static uint8_t instant_temp = 0;
  static uint16_t count = 0;

  if (!(value & DIGIT_POS_MASK))
    return false;

  for (int i = 0; i < DIGITS_SIZE; i++) {
    if ((value & DIGIT_POS_MASK) == digitPos[i]) {
      if (i == 0)
        raw_display = 0;

      raw_display |= bcd_to_digit(value) << (4 * (3 - i));

      if (i != 3)
        return true;
    }
  }

  // full display
  if (last_display != raw_display) {
    last_display = raw_display;
    count = 0;
  }

  if (count < MAX_COUNT) {
    count++;
    if (count == 3) {
      ESP_LOGD("PIO", "\tDisplay %04x %d {%d}", raw_display, blink_count, elapsed());

      if (raw_display >> 4 == 0xBBB) {         // Blank
        blink_count++;
      } else if (raw_display >> 16 == 0xE9) {  // Error
        uint8_t error = convert_to_decimal(raw_display & 0xFFF);
        if (status.error != error) {
          status.error = error;
          ESP_LOGI(TAG, "\tError: %d", status.error);
        }
      } else {  // Temp
        instant_temp = convert_to_decimal(raw_display);
        if (instant_temp != status.target_temperature) {
          if (blink_count && blink_count < 5) {
            ESP_LOGI("PIO", "\tCurrent Target: %d {%d}", instant_temp, elapsed());
            status.target_temperature = instant_temp;
            blink_count = 1;
          }
        }
      }
    } else if (count == MAX_COUNT) {
      if (instant_temp != status.current_temperature) {
        status.current_temperature = instant_temp;
        ESP_LOGI("PIO", "\tCurrent Temperature: %d {%d}", status.current_temperature, elapsed());
      }
      if (blink_count) {
        ESP_LOGI("PIO", "End of blink {%d}", elapsed());
        blink_count = 0;
      }
    }
  }

  return true;
}

bool process_leds(uint16_t value) {
  if (value & 0x4000) {
    status.leds = value;
    return true;
  }
  return false;
}

void IRQ_handler() {
  uint value;

  if (pio_interrupt_get(pio, 0)) {
    while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
      value = pio_sm_get(pio, sm);
      if (value != 0) {
        value = ~value & 0xFFFF;
        if (decode_display(value) || process_leds(value)) {
          // nothing to do
        }
      }
    }
  }
  pio_interrupt_clear(pio, 0);
}

}  // namespace purespa
}  // namespace esphome