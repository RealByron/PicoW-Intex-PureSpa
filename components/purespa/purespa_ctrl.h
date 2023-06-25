#ifndef SBH20IO_H
#define SBH20IO_H

#include <stdint.h>
#include "hardware/pio.h"

/**
 * The Intex serial protocol between the mainboard of the SB-H20 model and its
 * control panel uses a 16-bit data frame where each bit is sampled by the
 * rising clock signal and the frame is sampled by the rising slave select
 * signal, similar to SPI mode 3 but unidirectional. The clock speed is 100 kHz,
 * the data bits are inverted and the bit order is big endian (MSB).
 *
 * There are 4 telegram types, one for the 7-segment display, one for the LEDs,
 * one for the buttons and the cue telegram that is send each time the telegram
 * type changes. Only the button telegram is used for sending commands to the
 * SB-H20. All other telegrams are receive telegrams.
 *
 * To signal when a button is pressed the data line must be pulled low for
 * 2 microseconds after the slave select signal turns high if a switch telegram
 * has been received that matches the button function.
 *
 *        |-------------------------------------------------------------------------------|
 * BIT    | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
 *        |-------------------------------------------------------------------------------|
 * CUE    |  0 |  0 |  0 |  0 |  0 |  0 |  0 |  1 |  0 |  0 |  0 |  0 |  0 |  0 |  0 |  0 |
 * DIGIT  | DP |  x |  A |  B | S3 |  D |  C |  x |  E | S1 | S2 |  G |  F | S4 |  x |  x |
 * LED    |  0 |  1 |  0 |  x |  0 |  x |  x |  x |  x |  0 |  0 |  0 |  0 |  0 |  0 |  x |
 * BUTTON |  x |  0 |  x |  x |  0 |  x |  0 |  1 |  0 |  0 |  0 |  0 |  x |  0 |  0 |  0 |
 *        |-------------------------------------------------------------------------------|
 *
 * display:
 *
 * bits S1, S2, S3 and S4 each select a 7-segment display, digits are from left to right
 *
 * DP .    A
 *       -----
 *      |     |
 *    F |     | B
 *      |     |
 *       --G--
 *      |     |
 *    E |     | C
 *      |     |
 *       -----
 *         D
 *
 * telegram order:
 *
 * 32 frames, repeating every 21 ms (5x digit 1-4, 5x LEDs, 1x buttons 1-7)
 *
 * C D1 C D2 C D3 C D4 C L
 * C D1 C D2 C D3 C D4 C L
 * C D1 C D2 C D3 C D4 C L
 * C D1 C D2 C D3 C D4 C L
 * C D1 C D2 C D3 C D4 C L
 * C B B B B B B B L
 *
 */

#define PB(idx) def[idx].pb
#define LED(idx) def[idx].led
#define NAME(idx) def[idx].name

struct Control {
  const char *name;
  uint16_t pb;
  uint16_t led;
};

// Frame order : 0x0102, 0x8100, 0x1100, 0x0180, 0x0108, 0x0500, 0x2100, 0x4100
enum ControlIndex { FILTER = 0, HEATER, UP, DOWN, BUBBLE, POWER, UNIT, BEEP, STANDBY, RELEASE };

#define _NA_ 0x0000
const Control def[] = {
    {"Filter", 0x0102, 0x1000}, {"Heater", 0x8100, 0x0080}, {"Up", 0x1100, _NA_},   {"Down", 0x0180, _NA_},
    {"Bubble", 0x0108, 0x0400}, {"Power", 0x0500, 0x0001},  {"Unit", 0x2100, _NA_}, {"Beep", _NA_, 0x0100},
    {"Standby", _NA_, 0x0200},  {"Release", 0xBABE, _NA_},  // I love you babe !!
};

typedef struct {
  volatile uint8_t current_temperature;
  volatile uint8_t target_temperature;
  volatile uint8_t error;
  volatile uint16_t leds;
} SystemStatus;

namespace esphome {
namespace purespa {

class SBH20 {
 public:
  void setup();
  bool loop();

  SystemStatus get_status();
  void set_target_temperature(uint8_t temp);
  void set_state(uint8_t led, bool state);

  uint8_t getErrorValue() const;
};
}  // namespace purespa
}  // namespace esphome
#endif /* SBH20IO_H */
