#include "gear_logic.h"
#include "config.h"
#include "sensors.h"
#include <Arduino.h>

const uint16_t REVS_REQUIRED_X10[5] PROGMEM = { 80, 70, 60, 50, 45 };

static uint8_t currentGear;
static bool nPrev; // 前回ループのニュートラルスイッチ状態フラグ
static bool neutralEverObserved; // 起動後にニュートラルスイッチが一度でも ON になったかどうかのフラグ

namespace gear_logic {

  void init() {
    currentGear = 0;
    nPrev = false;
    neutralEverObserved = false;
  }

  void update() {
    bool nNow = sensors::isNeutralPressed();
    if (nNow && !nPrev) {
      currentGear = 0;
      neutralEverObserved = true;
    } else if (!nNow && nPrev) {
      currentGear = 1;
    }
    nPrev = nNow;
  }

  uint8_t getCurrentGear() {
    return currentGear;
  }

  bool hasNeutralBeenObserved() {
    return neutralEverObserved;
  }

  bool isQsAllowed() {
    if (sensors::isClutchPressed()) return false;
    if (!sensors::isRpmSignalAlive()) return false;
    uint16_t rpm = sensors::getRPM();
    if (rpm < QS_RPM_MIN || rpm > QS_RPM_MAX) return false;
    if (currentGear == 0 || currentGear >= 6) return false;
    return true;
  }

  uint32_t calculateCutTimeMs(uint16_t rpm) {
    if (rpm == 0) return MIN_CUT_MS;
    if (currentGear < 1 || currentGear > 5) return MIN_CUT_MS;
    uint32_t cut = (uint32_t)pgm_read_word(&REVS_REQUIRED_X10[currentGear - 1]) * 6000UL / rpm;
    if (cut < MIN_CUT_MS) cut = MIN_CUT_MS;
    if (cut > MAX_CUT_MS) cut = MAX_CUT_MS;
    return cut;
  }

  void notifyShiftCompleted() {
    if (currentGear >= 1 && currentGear < 6) {
      currentGear++;
    }
  }

}  // namespace gear_logic
