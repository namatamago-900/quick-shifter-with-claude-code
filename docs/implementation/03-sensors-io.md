# 03. センサー入出力・デバッグ

RPMパルス計測、シフト・クラッチ・Nスイッチの読み取り、およびイベント駆動ロギング。`sensors` 層・`debug.h` の責務範囲。

---

## 1. RPM計測

### 1.1 信号源

**タップ箇所は片側ピックアップ（22a または 22b）の1本のみ**（`docs/回路設計.md` 方針）。両ピックアップを併用しない理由：
- 90° V型2気筒のため両ピックアップ合成では**不等間隔パルス列**になり瞬時周期からの rpm 算出が不正確（`ignition.md` §2）
- 片側のみなら **クランク1回転 = 1パルス・等間隔**

| 項目 | 値 |
|---|---|
| 信号形態 | シュミットトリガ整形後のデジタルパルス（フォトカプラで絶縁） |
| パルスレート | 1パルス／クランク1回転 |
| rpm 想定範囲 | 1200（アイドル）〜 9000（レブ） |
| パルス周期 | 50 ms（1200 rpm）〜 6.67 ms（9000 rpm） |
| 結線方式 | ピックアップ → IDS 間から**並列分岐**して読み取り。元の点火経路は無改造で維持 |

### 1.2 計測アルゴリズム

直近4パルス周期の移動平均。

```cpp
// sensors.cpp
volatile uint32_t lastPulseMicros;
volatile uint32_t periodSamples[RPM_AVG_SAMPLES];
volatile uint8_t  sampleIdx;
volatile bool     firstPulseSeen;   // 初回判定。lastPulseMicros=0 は使わない
                                    // (micros() ラップで0に当たると取りこぼすため)
volatile bool     periodsValid;

void onRpmPulseISR() {
  uint32_t now = micros();
  if (firstPulseSeen) {
    periodSamples[sampleIdx] = now - lastPulseMicros;
    sampleIdx = (sampleIdx + 1) & (RPM_AVG_SAMPLES - 1);
    if (sampleIdx == 0) periodsValid = true;
  }
  firstPulseSeen = true;
  lastPulseMicros = now;
}

uint16_t getRPM() {
  uint32_t copy[RPM_AVG_SAMPLES];
  uint32_t last;
  bool valid;
  // volatile 配列を memcpy で読むのは規格上 UB。noInterrupts 下で要素ごと代入する
  noInterrupts();
  for (uint8_t i = 0; i < RPM_AVG_SAMPLES; i++) copy[i] = periodSamples[i];
  last  = lastPulseMicros;
  valid = periodsValid;
  interrupts();

  if (!valid) return 0;
  if ((micros() - last) > (RPM_TIMEOUT_MS * 1000UL)) return 0;  // 信号断

  uint32_t sum = 0;
  for (uint8_t i = 0; i < RPM_AVG_SAMPLES; i++) sum += copy[i];
  return (uint16_t)(60000000UL / (sum / RPM_AVG_SAMPLES));
}

// 信号断のみを判定（低rpm抑制との区別に使う）
bool isRpmSignalAlive() {
  noInterrupts();
  bool seen = firstPulseSeen;
  uint32_t last = lastPulseMicros;
  interrupts();
  if (!seen) return false;
  return (micros() - last) <= (RPM_TIMEOUT_MS * 1000UL);
}
```

API の役割分担：`getRPM()` は信号断時もアイドル割れ時も `0` を返すため、両者を区別したい場合は `isRpmSignalAlive()` を併用する（[04-safety.md](./04-safety.md) §異常フラグ・LED制御 がこちらを参照）。

## 2. シフト・補助スイッチ

### 2.1 物理層と極性

全入力スイッチは **アクティブLOW**（`INPUT_PULLUP` 受け、スイッチの他端を GND へ）。配線本数最小、断線時 HIGH＝リリース相当に倒れる。

| ピン | センサー | 押下時 | リリース時 | デバウンス | 備考 |
|---|---|---|---|---|---|
| D5 | シフトロッドSW (Yamaha 13S-82470-00 等) | LOW | HIGH | 5ms | 上方向プッシュ時のみ ON（単方向性要確認） |
| D6 | クラッチSW (DRC F5945) | LOW | HIGH | 20ms | 油圧上昇＝握り |
| D4 | ニュートラルSW (純正) | LOW | HIGH | 20ms | 既存ハーネスから並列分岐。状態変化エッジで `currentGear` を更新（[01-control-strategy.md](./01-control-strategy.md) §ギアカウンタの更新ルール） |

### 2.2 シフト検知ロジック

- **エッジトリガ**：LOW を 5ms 連続観測で「押下確定」
- 確定後 SHIFT_PENDING → CUTTING へ遷移
- カット完了＆クールダウン後、スイッチが一度 HIGH に戻ってから再受付（**リリース要件**）

リリース要件により以下も自動的に成立する：
- 押しっぱなしでの連続再発火を防止
- シフトSW固着（ONのまま）時は最初の1発で発火した後、自動的に無発火状態が続く（追加のタイムアウトガード不要）

## 3. デバッグ・ロギング

`DEBUG_MODE` マクロでコンパイル時切替。本番ビルドではシリアル出力コードが完全に消える。

`Serial.printf` は **AVR Arduino コアでは未サポート**（`HardwareSerial` に `printf` メンバなし）。`snprintf` で一時バッファに整形し `Serial.println` に渡す。

```cpp
// src/debug.h
#ifdef DEBUG_MODE
  #define DBG_INIT()  do { Serial.begin(115200); } while(0)
  #define DBG_EVENT(fmt, ...) do {                                \
    char _dbgbuf[64];                                             \
    snprintf(_dbgbuf, sizeof(_dbgbuf), fmt, ##__VA_ARGS__);       \
    Serial.print(millis()); Serial.print(F(": "));                \
    Serial.println(_dbgbuf);                                      \
  } while(0)
#else
  #define DBG_INIT()
  #define DBG_EVENT(fmt, ...)
#endif
```

- `snprintf` でバッファオーバーラン防止
- 文字列リテラルは `F()` で PROGMEM 配置し SRAM を節約
- **AVR-libc の `snprintf` はデフォルトで `%f` 非対応**（float サポートはリンカオプション `-Wl,-u,vfprintf -lprintf_flt` 等が別途必要）。本実装は整数のみで完結させること
- ログはイベント発生時のみ。連続ストリーム（rpm 毎パルスなど）は出さない

### ログ出力項目

ギア推定ずれ解析（[01-control-strategy.md](./01-control-strategy.md) §ギア推定ずれの観測）のため、シフトイベント時は **シフト前/後の rpm と比率** を残す：

```
0:        SYSTEM_BOOT
1284:     LOCK_RELEASE gear=0
12340:    SHIFT_BEGIN  gear=2  rpm_pre=6800  cut=70ms
12410:    SHIFT_END    gear=3  rpm_post=5210  ratio_x1000=766
12710:    COOLDOWN_END
15022:    INHIBIT      reason=clutch
20011:    ERROR        rpm_signal_timeout
```

`ratio_x1000` は `(rpm_post * 1000) / rpm_pre`（整数）。`gear.md` のギア比ステップ（1→2: 715, ..., 5→6: 894）から大きく乖離していればギア推定ずれを疑う。

`gear` フィールドの値タイミング：
- `SHIFT_BEGIN` の `gear` は**インクリメント前**（シフト元）の値。例の `gear=2` は「2速から3速にシフト開始」を意味する
- `SHIFT_END` の `gear` は**インクリメント後**（シフト後）の値。`currentGear++` は CUTTING→COOLDOWN 遷移時に実行されるため、SHIFT_END 時点では既に新ギアの値になっている
