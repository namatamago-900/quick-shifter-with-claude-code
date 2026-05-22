#pragma once
// ピン定義
#define PIN_RPM_PULSE 3 // D3 rpm信号入力
#define PIN_SHIFT_SW 5 // D5 シフトロッドSW
#define PIN_CLUTCH_SW 6 // D6 クラッチSW
#define PIN_NEUTRAL_SW 4 // D4 ニュートラルSW
#define PIN_CUT_OUTPUT 8 // D8 点火カット出力
#define PIN_STATUS_LED 7 // D7 状態LED

// rpm
#define QS_RPM_MIN 3000
#define QS_RPM_MAX 8500
#define RPM_AVG_SAMPLES 4
#define RPM_TIMEOUT_MS 100

// カット時間
#define MIN_CUT_MS 40
#define MAX_CUT_MS 120
// 1→2, 2→3, 3→4, 4→5, 5→6 の遷移に対応。gear (1..5) から REVS_REQUIRED_X10[gear-1]。
// revs_required を ×10 して整数化（AVR の浮動小数を回避）。
const uint16_t REVS_REQUIRED_X10[5] = { 80, 70, 60, 50, 45 };

// タイミング
#define SHIFT_DEBOUNCE_MS 5
#define SWITCH_DEBOUNCE_MS 20 // クラッチ・N用
#define COOLDOWN_MS 300

// デバッグ
#define DEBUG_MODE// 本番ビルドではコメントアウト
