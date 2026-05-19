#pragma once
#include <stdint.h>

namespace sensors {

// ピン初期化・割込アタッチ
void init();

// スイッチデバウンス更新（毎ループ呼ぶ）
void update();

// RPM 計測（直近4パルス移動平均。信号断またはサンプル未満は 0 を返す）
uint16_t getRPM();

// RPM 信号の生死判定（低rpm抑制との区別用）
bool isRpmSignalAlive();

// シフトロッドSW（5ms 安定 LOW で確定）
bool isShiftPressed();
bool isShiftReleased();  // リリース要件確認用（確定 HIGH）

// クラッチSW（20ms 安定 LOW で確定）
bool isClutchPressed();

// ニュートラルSW（20ms 安定 LOW で確定）
bool isNeutralPressed();

}  // namespace sensors
