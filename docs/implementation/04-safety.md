# 04. 安全設計

状態表示LED、フェイルセーフ出力極性、ウォッチドッグタイマー、二重クランプ、入力異常への対処。`shifter` 層・`.ino` の `setup()` で実装する。

---

## 1. 状態表示インジケータ（単色LED 1個）

LED の表示は **状態機械の状態（[02-architecture.md](./02-architecture.md) §ステートマシン）＋異常フラグ** から派生算出する。「ERROR 表示状態」は状態機械上に存在せず、IDLE / LOCKED に滞在しながら表示だけ ERROR となる。

| LED表示 | 表示条件 |
|---|---|
| 常時消灯 | 電源断（Arduino電源喪失） |
| 常時点灯 | state == IDLE & 異常フラグなし |
| ゆっくり点滅（1Hz） | state == LOCKED |
| 速い点滅（5Hz） | 異常フラグあり（rpm信号断） |
| 短時間点灯 | state == CUTTING（視覚的フィードバック） |

### 異常フラグ

| フラグ | セット | クリア |
|---|---|---|
| rpm信号断 | 直近 `RPM_TIMEOUT_MS` 以上パルスなし | パルス再開（getRPM が非ゼロ） |

### 優先順位

CUTTING（短時間） > LOCKED > ERROR > IDLE。例：起動直後にライダーが N を踏まずクラッチを握ったまま放置するとエンジン未始動で rpm信号断となるが、このときは LOCKED 表示を優先する（ライダーが先に N を踏むべきため）。LOCKED から ERROR への遷移ロジックは実装しない。

物理的LEDは外部に1個。`PIN_STATUS_LED`（D7）から駆動。Arduino Nano の内蔵LED（D13）にミラーすればベンチ確認も容易。

## 2. フェイルセーフ方向

**点火カット出力は アクティブHIGH**。

| Arduino 出力ピン | 動作 |
|---|---|
| LOW（電源断/リセット中/起動直後/IDLE） | 点火通常通電 |
| HIGH（CUTTING状態のみ） | 点火カット |

ハードウェア側のリレー/MOSFET駆動回路も「HIGH駆動でコイル一次断」となるよう設計する。これにより Arduino が死んでも点火系は通常動作に倒れる（走行中エンスト・転倒回避）。

起動時の不定状態対策：
- `pinMode(PIN_CUT_OUTPUT, OUTPUT)` の **前** に `digitalWrite(LOW)` を呼ぶ
- ハードウェア側に 10kΩ 外部プルダウンを設置

## 3. ウォッチドッグタイマー（WDT）

AVR内蔵WDTを **1秒** で有効化。`loop()` で `wdt_reset()` を呼び続け、ハングしたら Arduino が自動リセットされる。リセット後は §2 フェイルセーフ方向 によりカット出力は LOW（= 通常点火）に戻るため、最悪ケースでもエンジンは止まらない。

### DEBUG_MODE 時の注意

115200 bps で TXバッファ満杯 → `Serial.println` ブロッキング → 1秒以内に解消しなければ WDT リセット、という連鎖が起きうる。

対策：
- ログを1行・短く保つ（[03-sensors-io.md](./03-sensors-io.md) §デバッグ・ロギング 方針に合致済）
- ベンチテストでランダムリセットが頻発する場合、`setup()` で `#ifdef DEBUG_MODE` 分岐し WDT を 2秒に延長
- 本番ビルド（DEBUG_MODE 無効）ではこの問題は発生しない

## 4. ハードウェア・ソフトウェア間の絶対クランプ（二重防御）

ソフト側で `MAX_CUT_MS = 120 ms` を超える出力時間を絶対に発生させない。さらに CUTTING 滞在時間にも独立した上限ガードを置く：

```cpp
if (state == CUTTING && (millis() - cutStartMs) > MAX_CUT_MS) {
  forceEndCut();
  DBG_EVENT("ERROR cut_overrun");
}
```

`forceEndCut()` の責務（`shifter` モジュールが提供）：
1. 点火カット出力ピンを `LOW` に戻す（`digitalWrite(PIN_CUT_OUTPUT, LOW)`）
2. state を `COOLDOWN` に強制遷移する
3. `cooldownStartMs = millis()` をセットして COOLDOWN 期間の起点を更新する

「計算結果ではなく経過時間そのもの」を見る独立ガード。`cut_time_ms` の計算ミスや変数破壊に対する二重防御。

## 5. 入力異常への対処

| 異常 | 検知 | 対応 |
|---|---|---|
| rpm 信号断 | 直近 100ms にパルス無し | 抑制（QS不発動）、LED ERROR点滅、ログ出力 |
| シフトSW 固着 | [03-sensors-io.md](./03-sensors-io.md) §シフト検知ロジック のリリース要件で自動的に1発限り発火 | 追加ガード不要 |
| クラッチSW 断線 | 検知困難（プルアップにより常時 HIGH = リリース相当） | クラッチ抑制が効かなくなる。3000rpm 以上で握り換え操作中に意図せず QS が発動する可能性あり。`MAX_CUT_MS=120` クランプと §2 フェイルセーフ方向 が最終防衛線。断線検知は将来拡張（[05-hardware-scope-tuning.md](./05-hardware-scope-tuning.md) §将来拡張余地） |
| N SW 断線 | 検知困難（常時 HIGH = N以外） | LOCKED から脱出不能。LED 1Hz 点滅でライダーが気付ける |
