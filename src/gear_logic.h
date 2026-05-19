#pragma once
#include <stdint.h>

namespace gear_logic {

  // 初期化（sensors::init() の後に呼ぶ）
  void init();

  // Nスイッチエッジ検出・ギアカウンタ更新（毎ループ呼ぶ）
  void update();

  // 現在ギア取得（0=N, 1..6=各ギア）
  uint8_t getCurrentGear();

  // N が一度でも観測されたか（LOCKED 解除判定用）
  bool hasNeutralBeenObserved();

  // QS作動許可判定（抑制条件をすべて評価）
  bool isQsAllowed();

  // カット時間算出（ms）。isQsAllowed() が true のときのみ呼ぶこと
  uint32_t calculateCutTimeMs(uint16_t rpm);

  // シフト完了通知（shifter が CUTTING→COOLDOWN 遷移時に呼ぶ）
  void notifyShiftCompleted();

}  // namespace gear_logic
