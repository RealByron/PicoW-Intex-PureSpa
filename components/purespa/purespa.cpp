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
  static SystemStatus last_status = {0, 0, 0, 0};

  if (!sbh_.loop())
    // target temperature not reached,
    // don't call other component treatment to avoid publication
    // during taget temperature setup
    return;

  SystemStatus status = sbh_.get_status();

  if (memcmp(&status, &last_status, sizeof(SystemStatus))) {
    switch_bubble_->publish_state((bool) (status.leds & LED(BUBBLE)));
    switch_power_->publish_state((bool) (status.leds & LED(POWER)));
    switch_filter_->publish_state((bool) (status.leds & LED(FILTER)));

    ESP_LOGI("PureSpa", "Status: %d %d %04x", status.current_temperature, status.target_temperature, status.leds);
    climate_->current_temperature = status.current_temperature;
    climate_->target_temperature = status.target_temperature;
    if (status.leds & LED(HEATER)) {
      climate_->action = esphome::climate::CLIMATE_ACTION_HEATING;
      climate_->mode = esphome::climate::CLIMATE_MODE_HEAT;
    } else if (status.leds & LED(STANDBY)) {
      climate_->action = esphome::climate::CLIMATE_ACTION_IDLE;
      climate_->mode = esphome::climate::CLIMATE_MODE_HEAT;
    } else {
      climate_->action = esphome::climate::CLIMATE_ACTION_OFF;
      climate_->mode = esphome::climate::CLIMATE_MODE_OFF;
    }
    climate_->publish_state();
    last_status = status;
  }
}

}  // namespace purespa
}  // namespace esphome
