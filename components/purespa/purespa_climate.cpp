#include "stdint.h"
#include "purespa_climate.h"

namespace esphome {
namespace purespa {

void PureSpaClimate::control(const esphome::climate::ClimateCall &call) {
  auto tt = call.get_target_temperature();
  if (tt) {
    get_parent()->sbh()->set_target_temperature(*tt);
  }

  auto mode = call.get_mode();
  if (mode) {
    ESP_LOGD("PureSpaClimate", "mode %d", *mode);

    get_parent()->sbh()->set_state(HEATER, *mode == climate::CLIMATE_MODE_HEAT);
  }
}

}  // namespace purespa
}  // namespace esphome
