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

#define DEBUG_LOW_LEVEL_SPA
#define DEBUG_PIN 6

#define PRESS_DURATION 100

#define abs(x) (((x) > 0) ? (x) : -(x))

PIO pio = pio0;
uint offset;
uint sm;

SystemStatus status;

void IRQ_handler();
void sbh20_program_init(PIO pio, uint sm, uint offset, uint pin);
void short_press(uint8_t pb, uint16_t duration);
void pb_press(uint8_t bp);

void SBH20::setup() {
  pio = pio0;
  offset = pio_add_program(pio, &sbh20_program);
  sm = pio_claim_unused_sm(pio, true);

  gpio_init(7);
  gpio_set_dir(7, true);
  gpio_put(7, true);

  gpio_init(DEBUG_PIN);
  gpio_set_dir(DEBUG_PIN, true);
  sbh20_program_init(pio, sm, offset, BASE_PIN);

  delay(1000);
  if (!(status.leds & LED(POWER))) {
    short_press(POWER, 1000);
  }
  if ((status.target_temperature == 0)) {
    short_press(DOWN, PRESS_DURATION);
  }
}

void pb_press(uint8_t pb) {
  ESP_LOGD("SBH20", "PB pressed %s", NAME(pb));
  pio_sm_put_blocking(pio, sm, (uint16_t) ~PB(pb));
  pio_sm_exec(pio, sm, pio_encode_jmp(offset + sbh20_offset_set_x));
}

void short_press(uint8_t pb, uint16_t duration) {
  ESP_LOGD("SBH20", "Short PB %s", NAME(pb), duration);

  pb_press(pb);
  delay(duration);
  pb_press(RELEASE);
  delay(200);
}

void SBH20::set_target_temperature(uint8_t temp) {
  if (status.leds & LED(POWER)) {
    ESP_LOGD("SBH20", "Expected target temperature %d", temp);
    while (true) {
      if (temp > status.target_temperature) {
        ESP_LOGD("SBH20", "Target temp %d => %d", status.target_temperature, temp);
        pb_press(UP);
        while (temp > status.target_temperature) {
          delay(10);
        }
        pb_press(RELEASE);
      }

      if (temp < status.target_temperature) {
        ESP_LOGD("SBH20", "Target temp %d => %d", status.target_temperature, temp);
        pb_press(DOWN);
        while (temp < status.target_temperature) {
          delay(10);
        }
        pb_press(RELEASE);
      }
      ESP_LOGD("SBH20", "Target temp %d reached", status.target_temperature);
      if (temp == status.target_temperature) {
        temp = 0;
        break;
      }
    }
  }
}

bool get_state(uint16_t led) { return (bool) (status.leds & led); }
void SBH20::set_state(uint8_t led, bool state) {
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
#define MAX_COUNT 200

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

uint8_t bcd_to_decimal_digit(uint16_t value) {
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
  static uint16_t rawDisplay = 0;
  static uint8_t instantTemp = 0;
  static uint8_t lastTemp = 0;
  static uint16_t count = 0;

  if (!(value & DIGIT_POS_MASK))
    return false;

  for (int i = 0; i < DIGITS_SIZE; i++) {
    if ((value & DIGIT_POS_MASK) == digitPos[i]) {
      if (i == 0)
        rawDisplay = 0;

      rawDisplay |= bcd_to_decimal_digit(value) << (4 * (3 - i));

      if (i != 3)
        return true;
    }
  }

  // full display
  if (rawDisplay >> 4 == 0xBBB)  // Blank
  {
    count = 0;
    if (status.target_temperature != instantTemp) {
      status.target_temperature = instantTemp;
      ESP_LOGD("PIO", "Setpoint: \t%d", status.target_temperature);
    }
  } else if (rawDisplay >> 12 == 0xE)  // Error
  {
    uint8_t error = convert_to_decimal(rawDisplay & 0xFFF);
    if (status.error != error) {
      status.error = error;
      ESP_LOGD(TAG, "\tError: \t%d\n", status.error);
    }
  } else  // Temp
  {
    instantTemp = convert_to_decimal(rawDisplay);

    if (instantTemp != lastTemp) {
      // Quick update during blinking
      // absolute diff avoid to set the setpoint to ambiante temp at the end of blinking
      // in any case last real setpoint have been set at last blank display
      if (abs(status.target_temperature - instantTemp) == 1) {
        status.target_temperature = instantTemp;
        ESP_LOGD("PIO", "\tSetpoint: \t%d", status.target_temperature);
      }
      lastTemp = instantTemp;
      count = 0;
    } else {
      if (count < MAX_COUNT) {
        count++;
        if (count == MAX_COUNT) {
          if (status.current_temperature != instantTemp) {
            status.current_temperature = instantTemp;
            ESP_LOGD("PIO", "\tTemperature: \t%d\n", status.current_temperature);
          }
        }
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
      value = pio_sm_get_blocking(pio, sm);
      if (value != 0) {
        gpio_put(7, true);
        value = ~value & 0xFFFF;
        if (decode_display(value) || process_leds(value)) {
        }
        gpio_put(7, false);
      }
    }
    pio_interrupt_clear(pio, 0);
  }
}

}  // namespace purespa
}  // namespace esphome