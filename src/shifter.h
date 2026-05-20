#pragma once

namespace shifter {

  // ピン初期化・WDT有効化・状態初期化
  void init();

  // ステートマシン1ティック（毎ループ呼ぶ）
  void update();

}  // namespace shifter
