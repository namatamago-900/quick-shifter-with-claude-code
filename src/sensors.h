#pragma once
#include <stdint.h>

namespace sensors {

  /*
   * RPMパルス割り込み・各スイッチピンを初期化し、外部割り込み（INT0）をアタッチする。
   * setup() の最初に呼び出すこと。
   */
  void init();

  /*
   * シフトロッド・クラッチ・ニュートラルの各スイッチ状態を読み取り、デバウンスタイマーを更新する。
   * loop() で毎ティック呼び出すこと。
   */
  void update();

  /*
   * 直近 RPM_AVG_SAMPLES パルスの間隔をもとに移動平均で RPM を算出して返す。
   * 信号断（RPM_TIMEOUT_MS 超過）またはサンプル数不足の場合は 0 を返す。
   */
  uint16_t getRPM();

  /*
   * 最後にパルスを受信してからの経過時間が RPM_TIMEOUT_MS 以内かどうかを返す。
   * getRPM() が 0 を返した際に、信号断か低回転かを区別するために使用する。
   */
  bool isRpmSignalAlive();

  /*
   * シフトロッドスイッチが SHIFT_DEBOUNCE_MS（5ms）以上継続して LOW の場合に true を返す。
   * 誤検知を防ぐためにデバウンス処理を経た確定値を返す。
   */
  bool isShiftPressed();

  /*
   * シフトロッドスイッチが SHIFT_DEBOUNCE_MS 以上継続して HIGH の場合に true を返す。
   * クールダウン後のシフト完了確認など、リリース状態の検出に使用する。
   */
  bool isShiftReleased();

  /*
   * クラッチスイッチが SWITCH_DEBOUNCE_MS（20ms）以上継続して LOW の場合に true を返す。
   * クラッチ操作中は QS 作動を抑制するための安全判定に使用する。
   */
  bool isClutchPressed();

  /*
   * ニュートラルスイッチが SWITCH_DEBOUNCE_MS（20ms）以上継続して LOW の場合に true を返す。
   * ニュートラル検出による誤シフト防止のためにギアカウンタ更新で使用する。
   */
  bool isNeutralPressed();

}  // namespace sensors
