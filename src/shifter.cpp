#include "shifter.h"
#include "config.h"
#include "sensors.h"
#include "gear_logic.h"
#include "debug.h"
#include <Arduino.h>
#include <avr/wdt.h>

// ── ステートマシン ────────────────────────────────────────────────────────────

enum State : uint8_t {
  LOCKED,
  IDLE,
  SHIFT_PENDING,
  CUTTING,
  COOLDOWN
};

static State    state;
static uint32_t stateStartMs;
static uint32_t cutStartMs;
static uint32_t cutTimeMs;
static uint32_t cooldownStartMs;
static uint16_t rpmPreCut;
static bool shiftMustRelease;  // リリース要件: カット後は一度HIGHに戻るまで次のシフトを受け付けない
static bool inhibitLogged;     // INHIBIT ログのスパム防止
static bool rpmWasAlive;       // rpm信号断の遷移検出（LOSTエッジで1回だけログ）

// ── LED制御 ──────────────────────────────────────────────────────────────────

static uint32_t ledToggleMs;
static bool ledOn;

static void setLed(bool on) {
  uint8_t v = on ? HIGH : LOW;
  digitalWrite(PIN_STATUS_LED, v);
  digitalWrite(LED_BUILTIN, v);
}

static void updateLed() {
  uint32_t now = millis();

  if (state == CUTTING) {
    setLed(true);
    return;
  }

  if (state == LOCKED) {
    // 1Hz点滅
    if ((now - ledToggleMs) >= 500UL) {
      ledOn = !ledOn;
      ledToggleMs = now;
    }
    setLed(ledOn);
    return;
  }

  if (!sensors::isRpmSignalAlive()) {
    // 5Hz点滅（rpm信号断）
    if ((now - ledToggleMs) >= 100UL) {
      ledOn = !ledOn;
      ledToggleMs = now;
    }
    setLed(ledOn);
    return;
  }

  // IDLE / SHIFT_PENDING / COOLDOWN & 異常なし: 常時点灯
  setLed(true);
}

// ── フェイルセーフ強制終了 ────────────────────────────────────────────────────

static void forceEndCut() {
  digitalWrite(PIN_CUT_OUTPUT, LOW);
  cooldownStartMs = millis();
  state = COOLDOWN;
  DBG_EVENT("ERROR cut_overrun");
}

// ── INHIBIT理由文字列 ─────────────────────────────────────────────────────────

#ifdef DEBUG_MODE
static const char* inhibitReason() {
  if (!sensors::isRpmSignalAlive()) return "rpm_timeout";
  if (sensors::isClutchPressed()) return "clutch";
  uint16_t rpm = sensors::getRPM();
  if (rpm < QS_RPM_MIN || rpm > QS_RPM_MAX) return "rpm_range";
  if (gear_logic::getCurrentGear() == 0) return "gear_n";
  return "gear_max";
}
#endif

// ── 公開 API ──────────────────────────────────────────────────────────────────

namespace shifter {

  void init() {
    // フェイルセーフ: OUTPUTモード設定前にLOWを確定させる（起動時の不定状態防止）
    digitalWrite(PIN_CUT_OUTPUT, LOW);
    pinMode(PIN_CUT_OUTPUT, OUTPUT);

    pinMode(PIN_STATUS_LED, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    state = LOCKED;
    stateStartMs = 0;
    cutStartMs = 0;
    cutTimeMs = 0;
    cooldownStartMs = 0;
    rpmPreCut = 0;
    shiftMustRelease = false;
    inhibitLogged = false;
    rpmWasAlive = false;
    ledToggleMs = 0;
    ledOn = false;

    DBG_INIT();
    DBG_EVENT("SYSTEM_BOOT");

    #ifdef DEBUG_MODE
    wdt_enable(WDTO_2S);  // シリアル出力によるWDTリセットを防ぐため2秒に延長
    #else
    wdt_enable(WDTO_1S);
    #endif
  }

  void update() {
    uint32_t now = millis();

    // rpm信号断の遷移検出（alive→dead の立下りエッジで1回だけログ）
    bool rpmAlive = sensors::isRpmSignalAlive();
    if (rpmWasAlive && !rpmAlive) {
      DBG_EVENT("ERROR rpm_signal_timeout");
    }
    rpmWasAlive = rpmAlive;

    // リリース要件の追跡（全状態で毎ループ評価）
    if (shiftMustRelease && sensors::isShiftReleased()) {
      shiftMustRelease = false;
    }

    // 二重クランプガード: cut_time_ms の計算ミスや変数破壊への独立防衛
    if (state == CUTTING && (now - cutStartMs) > (uint32_t)MAX_CUT_MS) {
      forceEndCut();
    }

    switch (state) {

      case LOCKED:
        if (gear_logic::hasNeutralBeenObserved()) {
          DBG_EVENT("LOCK_RELEASE gear=%u", (unsigned)gear_logic::getCurrentGear());
          state = IDLE;
        }
        break;

      case IDLE: {
        if (shiftMustRelease || !sensors::isShiftPressed()) {
          inhibitLogged = false;
          break;
        }
        if (!gear_logic::isQsAllowed()) {
          if (!inhibitLogged) {
            DBG_EVENT("INHIBIT reason=%s", inhibitReason());
            inhibitLogged = true;
          }
          break;
        }
        inhibitLogged = false;
        state = SHIFT_PENDING;
        stateStartMs = now;
        break;
      }

      case SHIFT_PENDING:
        if (!sensors::isShiftPressed()) {
          state = IDLE;
          break;
        }
        if ((now - stateStartMs) >= (uint32_t)SHIFT_DEBOUNCE_MS) {
          // デバウンス期間中に抑制条件が成立した可能性があるため再評価
          if (!gear_logic::isQsAllowed()) {
            state = IDLE;
            break;
          }
          rpmPreCut = sensors::getRPM();
          cutTimeMs = gear_logic::calculateCutTimeMs(rpmPreCut);
          cutStartMs = now;
          shiftMustRelease = true;
          digitalWrite(PIN_CUT_OUTPUT, HIGH);
          state = CUTTING;
          DBG_EVENT("SHIFT_BEGIN gear=%u rpm_pre=%u cut=%ums",
                    (unsigned)gear_logic::getCurrentGear(),
                    (unsigned)rpmPreCut,
                    (unsigned)cutTimeMs);
        }
        break;

      case CUTTING:
        if ((now - cutStartMs) >= cutTimeMs) {
          digitalWrite(PIN_CUT_OUTPUT, LOW);
          gear_logic::notifyShiftCompleted();
          cooldownStartMs = now;
          state = COOLDOWN;
          uint16_t rpmPost = sensors::getRPM();
          uint16_t ratio = (rpmPreCut > 0)
            ? (uint16_t)((uint32_t)rpmPost * 1000UL / rpmPreCut)
            : 0;
          DBG_EVENT("SHIFT_END gear=%u rpm_post=%u ratio_x1000=%u",
                    (unsigned)gear_logic::getCurrentGear(),
                    (unsigned)rpmPost,
                    (unsigned)ratio);
        }
        break;

      case COOLDOWN:
        if ((now - cooldownStartMs) >= (uint32_t)COOLDOWN_MS) {
          state = IDLE;
          DBG_EVENT("COOLDOWN_END");
        }
        break;
    }

    updateLed();
  }

}  // namespace shifter
