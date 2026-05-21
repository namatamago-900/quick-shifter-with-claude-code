#pragma once

namespace shifter {

  /*
   * 点火カット出力ピン・ステータス LED を初期化し、ウォッチドッグタイマーを有効化する。
   * setup() から gear_logic::init() の後に呼び出すこと。
   */
  void init();

  /*
   * IDLE→CUTTING→COOLDOWN のステートマシンを 1 ティック進め、点火カット出力と LED を制御する。
   * loop() で毎ティック呼び出すこと。
   */
  void update();

}  // namespace shifter
