#ifndef PURESPA_H_
#define PURESPA_H_

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "purespa_ctrl.h"

namespace esphome {
namespace purespa {

class PureSpaClimate;
class PureSpaSwitch;

class PureSpa : public Component {
 public:
  void setup() override;
  void loop() override;

  SBH20 *sbh() { return &sbh_; }

  void set_climate(PureSpaClimate *climate) { climate_ = climate; }
  void set_switch_power(PureSpaSwitch *switch_power) { switch_power_ = switch_power; }
  void set_switch_filter(PureSpaSwitch *switch_filter) { switch_filter_ = switch_filter; }
  void set_switch_bubble(PureSpaSwitch *switch_bubble) { switch_bubble_ = switch_bubble; }
  void set_error_text_sensor(esphome::text_sensor::TextSensor *error) { error_text_ = error; };

 private:
  SBH20 sbh_;
  PureSpaClimate *climate_ = nullptr;
  PureSpaSwitch *switch_power_ = nullptr;
  PureSpaSwitch *switch_filter_ = nullptr;
  PureSpaSwitch *switch_bubble_ = nullptr;
  esphome::text_sensor::TextSensor *error_text_ = nullptr;
};

}  // namespace purespa
}  // namespace esphome

#endif
