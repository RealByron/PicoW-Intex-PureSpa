#ifndef PureSpaClimate_H_
#define PureSpaClimate_H_
#include "esphome/components/climate/climate.h"
#include "purespa.h"

namespace esphome {
namespace purespa {

class PureSpaClimate : public esphome::climate::Climate, public esphome::Parented<PureSpa> {
 protected:
  virtual void control(const esphome::climate::ClimateCall &call) override;

  virtual esphome::climate::ClimateTraits traits() override {
    esphome::climate::ClimateTraits rv;

    rv.set_visual_min_temperature(20);
    rv.set_visual_max_temperature(40);
    rv.set_visual_temperature_step(1);
    rv.set_supports_current_temperature(true);
    rv.set_supports_action(true);
    rv.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT});

    return rv;
  }
};
}  // namespace purespa
}  // namespace esphome
#endif
