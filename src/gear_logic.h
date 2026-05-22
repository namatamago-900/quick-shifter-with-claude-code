#pragma once
#include <stdint.h>

namespace gear_logic {

  /*
   * ギアカウンタ・ニュートラル観測フラグ・内部状態を初期値にリセットする。
   * sensors::init() 完了後に setup() から呼び出すこと。
   */
  void init();

  /*
   * ニュートラルスイッチのエッジを検出し、ギアポジションカウンタを更新する。
   * loop() で毎ティック呼び出すこと。
   */
  void update();

  /*
   * 推定中の現在ギアポジションを返す（0=ニュートラル、1〜6=各ギア）。
   * ギアポジションセンサー非搭載のため、ニュートラルスイッチとシフト回数の積算で推定する。
   */
  uint8_t getCurrentGear();

  /*
   * 起動後にニュートラルスイッチが一度でも ON になったかどうかを返す。
   * false の間はギア位置不明（LOCKED 状態）として QS 作動を禁止する。
   */
  bool hasNeutralBeenObserved();

  /*
   * RPM 範囲・ギア位置確定・クラッチ状態など全抑制条件を評価して QS 作動可否を返す。
   * true のときのみ calculateCutTimeMs() を呼ぶこと。
   */
  bool isQsAllowed();

  /*
   * 現在のギアと rpm に基づいて適切な点火カット時間（ms）を算出して返す。
   * REVS_REQUIRED_X10 によるギア比変化率から必要な荷重抜き時間を計算する。
   */
  uint32_t calculateCutTimeMs(uint16_t rpm);

  /*
   * 点火カット完了後に shifter から呼ばれ、ギアカウンタをインクリメントして次ギアへ遷移する。
   * shifter の CUTTING→COOLDOWN 遷移タイミングでのみ呼び出すこと。
   */
  void notifyShiftCompleted();

}  // namespace gear_logic
