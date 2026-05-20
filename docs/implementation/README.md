# 実装方針 — Ducati 900SS クイックシフター（Arduino Nano）

本ディレクトリは `docs/domain/` のドメイン情報および `docs/回路設計.md` のハードウェア方針を踏まえた **プログラム実装（C++）の方針** を機能ごとに分割したものである。実装着手前に各書の決定を `config.h` に反映する。

対象車両の詳細は `CLAUDE.md` および `docs/domain/*.md` を参照。

---

## ファイル索引

| # | ファイル | 内容 |
|---|---------|------|
| 1 | [01-control-strategy.md](./01-control-strategy.md) | 制御方針：カット時間決定式、`REVS_REQUIRED_X10`、許可rpmレンジ、ギア状態管理、クラッチ抑制、クールダウン |
| 2 | [02-architecture.md](./02-architecture.md) | ソフトウェア構成（4層）、`config.h` 全体像、ステートマシン定義 |
| 3 | [03-sensors-io.md](./03-sensors-io.md) | RPM計測（割込・移動平均）、シフト/クラッチ/Nスイッチ、デバッグロギング |
| 4 | [04-safety.md](./04-safety.md) | 状態表示LED、フェイルセーフ方向、WDT、二重クランプ、入力異常対処 |
| 5 | [05-hardware-scope-tuning.md](./05-hardware-scope-tuning.md) | ピン要件差分、スコープ外・将来拡張、実装後チューニング手順 |

---

## 0. エグゼクティブサマリ

| # | 領域 | 決定内容 |
|---|---|---|
| 1 | カット時間戦略 | rpm × ギア比の動的計算（整数演算） |
| 2 | ギア推定 | Nスイッチ基点 + シフトカウント（N離脱エッジで gear=1） |
| 3 | 計算モデル | `REVS_REQUIRED_X10[gear-1] × 6000 / rpm` |
| 4 | クラッチ役割 | 操作中はQS抑制（インヒビット） |
| 5 | 作動rpmレンジ | 3000 – 8500 rpm |
| 6 | クールダウン | 300 ms |
| 7 | 起動時フォールバック | N観測までロックアウト |
| 8 | ステートマシン | 5状態（ERROR は表示派生） |
| 9 | ファイル構成 | 4層（config / sensors / gear_logic / shifter）+ debug.h |
| 10 | rpm計測 | 直近4パルス周期の移動平均、片側ピックアップ単独タップ |
| 11 | 状態表示 | 単色LED 1個 × 点滅パターン |
| 12 | カット上下限 | 40 – 120 ms（チューニング初期は 80 まで絞る） |
| 13 | デバッグ | DEBUG_MODE + `snprintf` ベースイベント駆動ログ |
| 14 | 出力極性 | アクティブHIGH + WDT 1秒 |
| 15 | `REVS_REQUIRED_X10` 初期値 | `{ 80, 70, 60, 50, 45 }` |

---

## 1. 目的・スコープ

### 1.1 目的
- ライダーがクラッチ操作なしでシフトアップできる電子制御を実装する
- Arduino Nano 単体で、エンジンrpmと現在ギアに応じて**動的にカット時間を計算**する
- 安全性（誤作動防止・フェイルセーフ）と保守性（チューニング容易性）を両立する

### 1.2 スコープ

| 含む | 含まない |
|---|---|
| シフトアップ時の点火カット制御 | シフトダウン補助（オートブリッパー） |
| rpm計測・ギア状態管理 | 自動ギア学習・rpm低下率からの逆推定 |
| クラッチ抑制・ロックアウト等の安全機構 | 車速連動カット時間補正（センサーなし） |
| 状態表示LED | OLEDダッシュ表示・SDロガー |

---

## 実装着手前のドキュメント追対応チェックリスト

[05-hardware-scope-tuning.md §1.5](./05-hardware-scope-tuning.md#15-本実装着手前のドキュメント追対応チェックリスト) を参照。

---

## 実装の進め方（推奨順序）

1. **`config.h` 作成**：[02-architecture.md](./02-architecture.md) §config.h 全体像 を写経
2. **`sensors` 層**：[03-sensors-io.md](./03-sensors-io.md) §RPM計測 / §シフト・補助スイッチ を実装
3. **`gear_logic` 層**：[01-control-strategy.md](./01-control-strategy.md) §ギア状態管理 / §カット時間決定式 を実装
4. **`shifter` 層**：[02-architecture.md](./02-architecture.md) §ステートマシン に従い、[04-safety.md](./04-safety.md) の安全機構を組み込む
5. **`debug.h`**：[03-sensors-io.md](./03-sensors-io.md) §デバッグ・ロギング を実装
6. **`.ino`**：`setup()` で各層を初期化、`loop()` で `sensors::update()` → `gear_logic::update()` → `shifter::update()` → `wdt_reset()` を呼ぶ
7. **チューニング**：[05-hardware-scope-tuning.md](./05-hardware-scope-tuning.md) §チューニング手順 に従う
