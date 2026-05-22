# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

Ducati 900SS（1995年式）に Arduino Nano を使ったクイックシフターを実装するプロジェクト。コード生成は Claude Code がすべて担当する。

## ビルド・アップロード

Arduino IDE または VS Code Arduino 拡張 (`vsciot-engineering.vscode-arduino`) を使用する。エントリーポイントは `quick-shifter-with-claude-code.ino`。

```sh
# Arduino CLI でのコンパイル・アップロード（インストール済みの場合）
arduino-cli compile --fqbn arduino:avr:nano .
arduino-cli upload  --fqbn arduino:avr:nano --port <COMx> .
```

本番ビルド前に `src/config.h` の `#define DEBUG_MODE` をコメントアウトすること（シリアル出力・WDTタイムアウト時間が変わる）。

## アーキテクチャ

4層の namespace 分離構成。`delay()` 禁止・すべてのタイミングは `millis()` ベース。

```
quick-shifter-with-claude-code.ino   ← setup()/loop() のみ。3層の init()/update() を順に呼ぶ
src/
  config.h          ← 全定数（ピン番号・RPM閾値・カット時間・デバウンス時間）。調整はここだけ
  sensors.h/.cpp    ← 入力層: RPMパルス割り込み（INT1/D3）・3スイッチのデバウンス
  gear_logic.h/.cpp ← 計算層: ギア推定・QS作動可否判定・点火カット時間算出
  shifter.h/.cpp    ← 出力層: ステートマシン（LOCKED→IDLE→SHIFT_PENDING→CUTTING→COOLDOWN）
  debug.h           ← DBG_INIT()/DBG_EVENT() マクロ。DEBUG_MODE 未定義時は完全除去
```

### 制御フロー

`shifter::update()` のステートマシンが制御の中心：

1. **LOCKED**: 起動後ニュートラルスイッチ未観測。`gear_logic::hasNeutralBeenObserved()` が true になるまで QS 禁止
2. **IDLE**: シフトスイッチ待機。`gear_logic::isQsAllowed()` でクラッチ・RPM範囲・ギア位置を評価
3. **SHIFT_PENDING**: シフトスイッチ ON 確定待ち（二重デバウンス）
4. **CUTTING**: `PIN_CUT_OUTPUT`（D8）を HIGH にして点火カット。`gear_logic::calculateCutTimeMs()` が算出した時間経過後に LOW
5. **COOLDOWN**: `COOLDOWN_MS`（300ms）待機後 IDLE へ戻る。この間は再シフト禁止

### 点火カット時間算出（`gear_logic.cpp`）

```
cut_ms = REVS_REQUIRED_X10[gear-1] × 6000 / rpm
```

`REVS_REQUIRED_X10` は Flash（PROGMEM）に配置し、AVRの浮動小数演算を回避。結果は `MIN_CUT_MS`〜`MAX_CUT_MS`（40〜120ms）にクランプ。

### 重要な設計上の制約

- `volatile` 変数は `sensors.cpp` に閉じ込め、外部からはアクセサ関数経由でのみ読む
- `volatile` 配列の安全なコピーには `noInterrupts()` ブロックを使う（`memcpy` は UB）
- ギアポジションセンサー非搭載のため `gear_logic` の推定は安全クリティカル。ニュートラルスイッチの立ち上がり/立下りエッジでのみギアカウンタを更新する
- 二重クランプガード: `shifter::update()` 内でカット時間超過を独立監視し、`forceEndCut()` でフェイルセーフ終了
- `shiftMustRelease` フラグ: カット完了後にシフトスイッチが一度 HIGH に戻るまで次のシフトを受け付けない

## ピンアサイン

| ピン | 役割 | 方向 |
|------|------|------|
| D3 | RPMパルス入力 | INPUT / INT1 |
| D4 | ニュートラルSW | INPUT_PULLUP |
| D5 | シフトロッドSW | INPUT_PULLUP |
| D6 | クラッチSW | INPUT_PULLUP |
| D7 | 状態LED | OUTPUT |
| D8 | 点火カット出力 | OUTPUT |

## ドキュメント構成

| ファイル | 内容 |
|----------|------|
| @docs/domain/quick-shifter.md | クイックシフターの仕組み・制御フロー・注意事項 |
| @docs/domain/sensors.md | シフトセンサー・クラッチセンサーの選定と接続方法 |
| @docs/domain/gear.md | ギア比・減速比・シフトアップ時の回転変化率 |
| @docs/domain/ignition.md | 点火系仕様・推奨点火カット時間の設計根拠 |
| @docs/domain/wiring.md | 電装系配線図・コンポーネント一覧・接続ポイント |
| @docs/domain/common.md | クラッチ仕様・電装系スペック・キャブレター車固有の制約 |
| @docs/arduino/pin_assign.md | Arduino Nano のピンアサイン一覧 |
