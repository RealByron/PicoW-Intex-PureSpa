#include "purespa.h"
#include "purespa_switch.h"
#include "purespa_climate.h"
#include "purespa_ctrl.h"

namespace esphome {
namespace purespa {
SystemStatus prev_state = {0, 0, 0, 0};

void PureSpa::setup() { sbh_.setup(); }

void PureSpa::loop() {
  bool climate_flag = false;

  if (!sbh_.loop())
    // target temperature not reached,
    // don't call other component treatment to avoid publication
    // during taget temperature setup
    return;

  SystemStatus state = sbh_.get_status();

  if (prev_state.target_temperature != state.target_temperature) {
    climate_flag = true;
    climate_->target_temperature = state.target_temperature;
  }
  if (prev_state.current_temperature != state.current_temperature) {
    climate_flag = true;
    climate_->current_temperature = state.current_temperature;
  }

  if (prev_state.target_temperature != state.target_temperature) {
    climate_flag = true;
    climate_->target_temperature = state.target_temperature;
  }

  uint16_t led_changes = prev_state.leds ^ state.leds;
  if (led_changes) {
    if (led_changes & LED(BUBBLE)) {
      switch_bubble_->publish_state((bool) (state.leds & LED(BUBBLE)));
    }
    if (led_changes & LED(POWER)) {
      switch_power_->publish_state((bool) (state.leds & LED(POWER)));
    }
    if (led_changes & LED(FILTER)) {
      switch_filter_->publish_state((bool) (state.leds & LED(FILTER)));
    }
    if (led_changes & (LED(HEATER) | LED(STANDBY))) {
      if (state.leds & LED(HEATER)) {
        climate_->action = esphome::climate::CLIMATE_ACTION_HEATING;
        climate_->mode = esphome::climate::CLIMATE_MODE_HEAT;
      } else if (state.leds & LED(STANDBY)) {
        climate_->action = esphome::climate::CLIMATE_ACTION_IDLE;
        climate_->mode = esphome::climate::CLIMATE_MODE_HEAT;
      } else {
        climate_->action = esphome::climate::CLIMATE_ACTION_OFF;
        climate_->mode = esphome::climate::CLIMATE_MODE_OFF;
      }
      climate_flag = true;
    }
  }

  if (climate_flag) {
    climate_->publish_state();
  }

  prev_state = state;
}

}  // namespace purespa
}  // namespace esphome
