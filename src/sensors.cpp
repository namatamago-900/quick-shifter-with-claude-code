#include "sensors.h"
#include "config.h"
#include <Arduino.h>

// ── RPM ISR ───────────────────────────────────────────────────────────────────
// volatile 変数の読み取りは必ず noInterrupts() ブロック内で行う

static volatile uint32_t lastPulseMicros;
static volatile uint32_t periodSamples[RPM_AVG_SAMPLES];
static volatile uint8_t  sampleIdx;
static volatile bool     firstPulseSeen;
static volatile bool     periodsValid;

static void onRpmPulseISR() {
  uint32_t now = micros();
  if (firstPulseSeen) {
    periodSamples[sampleIdx] = now - lastPulseMicros;
    sampleIdx = (sampleIdx + 1) & (RPM_AVG_SAMPLES - 1);
    if (sampleIdx == 0) periodsValid = true;
  }
  firstPulseSeen = true;
  lastPulseMicros = now;
}

// ── スイッチデバウンス ─────────────────────────────────────────────────────────

struct SwState {
  bool confirmed;
  bool pending;
  uint32_t pendingSince;
};

static SwState swShift;
static SwState swClutch;
static SwState swNeutral;

static void updateSw(SwState& sw, bool rawPressed, uint16_t debounceMs) {
  if (rawPressed != sw.pending) {
    sw.pending = rawPressed;
    sw.pendingSince = millis();
  }
  if ((millis() - sw.pendingSince) >= debounceMs) {
    sw.confirmed = sw.pending;
  }
}

// ── 公開 API ──────────────────────────────────────────────────────────────────

namespace sensors {

  void init() {
    pinMode(PIN_RPM_PULSE,  INPUT);
    pinMode(PIN_SHIFT_SW,   INPUT_PULLUP);
    pinMode(PIN_CLUTCH_SW,  INPUT_PULLUP);
    pinMode(PIN_NEUTRAL_SW, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_RPM_PULSE), onRpmPulseISR, RISING);
  }

  void update() {
    updateSw(swShift,   digitalRead(PIN_SHIFT_SW)   == LOW, SHIFT_DEBOUNCE_MS);
    updateSw(swClutch,  digitalRead(PIN_CLUTCH_SW)  == LOW, SWITCH_DEBOUNCE_MS);
    updateSw(swNeutral, digitalRead(PIN_NEUTRAL_SW) == LOW, SWITCH_DEBOUNCE_MS);
  }

  uint16_t getRPM() {
    uint32_t copy[RPM_AVG_SAMPLES];
    uint32_t last;
    bool valid;

    // volatile 配列を安全に取り出す（memcpy は規格上 UB のため要素ごと代入）
    noInterrupts();
    for (uint8_t i = 0; i < RPM_AVG_SAMPLES; i++) copy[i] = periodSamples[i];
    last  = lastPulseMicros;
    valid = periodsValid;
    interrupts();

    if (!valid) return 0;
    if ((micros() - last) > (RPM_TIMEOUT_MS * 1000UL)) return 0;  // 信号断

    uint32_t sum = 0;
    for (uint8_t i = 0; i < RPM_AVG_SAMPLES; i++) sum += copy[i];
    return (uint16_t)(60000000UL / (sum / RPM_AVG_SAMPLES));
  }

  bool isRpmSignalAlive() {
    noInterrupts();
    bool seen     = firstPulseSeen;
    uint32_t last = lastPulseMicros;
    interrupts();
    if (!seen) return false;
    return (micros() - last) <= (RPM_TIMEOUT_MS * 1000UL);
  }

  bool isShiftPressed()  { return swShift.confirmed; }
  bool isShiftReleased() { return !swShift.confirmed; }
  bool isClutchPressed() { return swClutch.confirmed; }
  bool isNeutralPressed(){ return swNeutral.confirmed; }

}  // namespace sensors
