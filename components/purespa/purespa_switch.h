#ifndef PureSpaSwitch_H_
#define PureSpaSwitch_H_
#include "esphome/components/switch/switch.h"
#include "esphome/core/log.h"
#include "purespa.h"

namespace esphome {
namespace purespa {

class PureSpaSwitch : public esphome::switch_::Switch, public esphome::Parented<PureSpa> {
 public:
  void set_type(const char *type) { type_ = type; }
  void write_state(bool state) override {
    if (!type_) {
      ESP_LOGE("PureSpaSwitch", "No switch type set!");
      return;
    }

    if (strcmp(type_, "bubble") == 0) {
      get_parent()->sbh()->set_state(BUBBLE, state);
    } else if (strcmp(type_, "filter") == 0) {
      get_parent()->sbh()->set_state(FILTER, state);
    } else if (strcmp(type_, "power") == 0) {
      get_parent()->sbh()->set_state(POWER, state);
    } else {
      ESP_LOGE("PureSpaSwitch", "Unknown switch type: %s", type_);
      return;
    }

    publish_state(state);
  }

 private:
  const char *type_ = nullptr;
};

}  // namespace purespa
}  // namespace esphome

#endif
