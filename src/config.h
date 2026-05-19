// ───── ピン定義（正本: docs/arduino/pin_assign.md） ─────
// 入力スイッチはすべてアクティブLOW（INPUT_PULLUP）
//   D5 シフトロッドSW   : 踏み込み = LOW（上方向プッシュ時のみ ON、単方向性要確認）
//   D6 クラッチSW       : 油圧上昇 = LOW (DRC F5945)
//   D4 ニュートラルSW   : N位置   = LOW
// 出力はアクティブHIGH
//   D8 点火カット出力   : HIGH = カット
//   D7 状態LED          : HIGH = 点灯
#define PIN_RPM_PULSE     3   // INT1 (Nano)
#define PIN_SHIFT_SW      5
#define PIN_CLUTCH_SW     6
#define PIN_NEUTRAL_SW    4
#define PIN_CUT_OUTPUT    8
#define PIN_STATUS_LED    7

// rpm
#define QS_RPM_MIN        3000
#define QS_RPM_MAX        8500
#define RPM_AVG_SAMPLES   4    // 2のべき乗
#define RPM_TIMEOUT_MS    100

// カット時間
#define MIN_CUT_MS        40
#define MAX_CUT_MS        120
// 1→2, 2→3, 3→4, 4→5, 5→6 の遷移に対応。gear (1..5) から REVS_REQUIRED_X10[gear-1]。
// revs_required を ×10 して整数化（AVR の浮動小数を回避）。
const uint16_t REVS_REQUIRED_X10[5] = { 80, 70, 60, 50, 45 };

// タイミング
#define SHIFT_DEBOUNCE_MS    5
#define SWITCH_DEBOUNCE_MS   20    // クラッチ・N用
#define COOLDOWN_MS          300

// デバッグ
#define DEBUG_MODE             // 本番ビルドではコメントアウト
