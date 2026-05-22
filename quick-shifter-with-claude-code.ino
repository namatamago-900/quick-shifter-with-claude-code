#include "src/sensors.h"
#include "src/gear_logic.h"
#include "src/shifter.h"
#include <avr/wdt.h>

void setup() {
  sensors::init();
  gear_logic::init();
  shifter::init();
}

void loop() {
  sensors::update();
  gear_logic::update();
  shifter::update();
  wdt_reset();
}
