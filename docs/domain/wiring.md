# Ducati 900SS (1995) — 電装系配線リファレンス

## コンポーネント一覧

| No. | Component                             |
|-----|---------------------------------------|
| 1   | Headlight                             |
| 2   | Front, right turn indicator           |
| 3   | Front, left turn indicator            |
| 4   | Horn                                  |
| 5   | Rev counter                           |
| 6   | Speedometer (km/h)            |
| 7   | Dashboard warning lights (12V-1.2W)   |
| 8   | Regulator-Rectifier                   |
| 9   | Ignition switch                       |
| 10  | R.H. grip switch                      |
| 11  | L.H. grip switch                      |
| 12  | Front stop light switch               |
| 13  | Rear stop light switch                |
| 14  | Neutral indicator / safety bypass switch |
| 15  | Oil pressure sending unit             |
| 16  | Side stand switch                     |
| 17  | Fuel level gauge                      |
| 18  | Fuel pump                             |
| 19  | Main Relay                            |
| 20  | Turn flasher relay                    |
| 21  | Alternator                            |
| 22a | Pick-up — horizontal cylinder         |
| 22b | Pick-up — vertical cylinder           |
| 23  | Ignition module (IDS)                 |
| 24  | Tail light                            |
| 25  | Fuse holder 30A (main)                |
| 26  | Coil — horizontal cylinder            |
| 27  | Coil — vertical cylinder              |
| 28  | Spark plug — horizontal cylinder      |
| 29  | Spark plug — vertical cylinder        |
| 30  | Fuse box                              |
| 31  | Starter motor                         |
| 32  | Starter relay (remote switch)         |
| 33  | Battery (YUASA YB16AL-A2)             |
| 34  | Rear, right turn indicator            |
| 35  | Rear, left turn indicator             |

---

## 配線色コード

| Code  | Color (EN)    |
|-------|---------------|
| R     | Red           |
| G     | Green         |
| Y     | Yellow        |
| V     | Violet        |
| GR    | Grey          |
| BN    | Brown         |
| BK    | Black         |
| P     | Pink          |
| LB    | Light Blue    |
| W-R   | White-Red     |
| G-W   | Green-White   |
| GR-R  | Grey-Red      |
| R-BK  | Red-Black     |
| O-W   | Orange-White  |
| Y-G   | Yellow-Green  |
| Y-BK  | Yellow-Black  |
| GR-BK | Grey-Black    |
| R-G   | Red-Green     |
| O-BK  | Orange-Black  |
| V-BK  | Violet-Black  |
| W-B   | White-Blue    |
| R-B   | Red-Blue      |
| G-BK  | Green-Black   |
| G-B   | Green-Blue    |

---

## 回路図

### 1. 主電源・充電回路

```
ALTERNATOR (21)
     |
     | （3相交流）
     v
REGULATOR-RECTIFIER (8)
     |
     | （直流安定化 ~14V）
     v
BATTERY [33] (YUASA YB16AL-A2, 12V)
     |
     |--- [+] ---> 30A MAIN FUSE (25)  ← インライン、バッテリー側
     |                    |
     |                    v
     |              FUSE BOX (30)
     |              ┌──────────────────────────────────────┐
     |              │ F1: Headlight / Tail light           │
     |              │ F2: Turn signals / Horn              │
     |              │ F3: Ignition / IDS / Fuel pump       │
     |              │ F4: Dashboard / Warning lights       │
     |              │ F5: Starter circuit                  │
     |              └──────────────────────────────────────┘
     |                    |
     |                    v
     |            IGNITION SWITCH (9)
     |
     |--- [+] ---> STARTER RELAY (32)
                         |
                         v
                   STARTER MOTOR (31)
```

> **FUSE BOX (30) に関する注意:** 入手可能な純正ドキュメントのページには、個々のヒューズ容量が明記されていない。上記の割り当ては典型的な900SSのレイアウトを反映しているため、実際のヒューズボックスのラベルまたは完全な純正配線図で必ず確認すること。30A MAIN FUSE (25) は FUSE BOX 上流のシステム全体を保護する。ヒューズ容量の参考値（A1: 30A、B2/C3: 15A、D4/E5: 7.5A）は `docs/domain/common.md` セクション3「ヒューズ構成」を参照。

---

### 2. 点火系回路

```
IGNITION SWITCH (9)
     |
     | （電源供給）
     v
IGNITION MODULE / IDS (23) <-------- PICK-UP — Horizontal cylinder (22a)
     |                          <---- PICK-UP — Vertical cylinder   (22b)
     |                                  [タイミング信号（各気筒独立）]
     |
     |--- COIL — Horizontal cylinder (26) ---> SPARK PLUG Horizontal (28)
     |
     `--- COIL — Vertical cylinder   (27) ---> SPARK PLUG Vertical   (29)
```

> **クイックシフター接続ポイント:**
> - **RPM信号タップ:** PICK-UP (22a または 22b) → IGNITION MODULE 間の信号線に割り込む。または IGNITION MODULE → REV COUNTER (5) 間のタコメーター信号線から取得する方が安全（信号レベルが安定している）。いずれもArduinoの5V入力に対してレベル変換回路が必要（`docs/domain/common.md` セクション4参照）。
> - **点火カット介入点:** COIL 水平 (26) および 垂直 (27) の一次側（LV側, 0.34 Ω）への電流遮断で実現する。ArduinoのOUTPUT端子でNチャンネルMOSFETをスイッチング、またはリレーを制御してコイルの一次電源ラインを断続する。`ignition.md` のカット時間設計（30〜120 ms）を参照。

---

### 2a. キルスイッチ回路（R.H. GRIP SWITCH — エンジン停止）

キルスイッチは **R.H. GRIP SWITCH (10)** にスターターボタンと並んで内蔵されており、**RUN** と **OFF (STOP)** の2ポジションを持つ。

**動作方式 — Ignition Module グラウンドキル方式:**  
キルワイヤー（R/BK — 赤/黒）が両 Ignition Module (23) のキル入力に接続されている。

```
R.H. GRIP SWITCH (10)
コネクターピン: [4][6][2][5][3][1]
                                |
              .-----------------+  （ピン1または3 — キルワイヤー R/BK）
              |
              v
         [RUN ポジション]                 [OFF / STOP ポジション]
              |                                      |
         R/BK フロート                   R/BK -----> GND (BK)
    （Ignition Module キル入力: オープン）（Ignition Module キル入力: グラウンド）
              |                                      |
              v                                      v
     Ignition Module (23) 正常点火       Ignition Module (23) 点火停止
     → エンジン動作                      → エンジン即時停止
```

**キルスイッチ経路（全体）:**

```
R.H. GRIP SWITCH (10)
  [STOP ポジション]
       |
       | ワイヤー: R/BK (Red/Black)
       v
Ignition Module — Horizontal cylinder (23, upper) — キル入力
Ignition Module — Vertical   cylinder (23, lower) — キル入力
       |
       v
      GND [車体アース / BK ワイヤー]
```

**スイッチ状態表:**

| キルスイッチポジション | R/BK ワイヤー状態 | Ignition Module 状態 | エンジン      |
|----------------------|-----------------|----------------------|---------------|
| RUN                  | オープン / フロート | 正常点火中           | 動作          |
| OFF (STOP)           | GND にショート  | キル入力有効          | 即時停止      |

> ⚠️ **トラブルシューティング:** 燃料・バッテリーに問題がないのにエンジンが始動しない場合、R/BK ワイヤーのショートや被覆損傷により Ignition Module キル入力が常時グラウンドに落ちている可能性を疑う。R.H. GRIP SWITCH のコネクターを外してエンジンが始動するか確認し、始動すればキルスイッチまたはその配線が原因。また、IGNITION SWITCH (9) が ON ポジションにあるか確認すること。LOCK / PARK / OFF はいずれも Ignition Module 上流の電源供給経路を遮断する。


---

### 3. サイドスタンド安全ロジック

> ⚠️ **1995年式 900SS 固有の仕様:** SIDE STAND SWITCH はエンジンを直接カットするのではなく、**MAIN RELAY (19)** を経由して **Ignition Module (23) への電源供給をカット**する。カットロジックは **NEUTRAL SWITCH (14)** の状態で条件分岐する：
> - スタンド **UP** → MAIN RELAY (19) が閉じたまま → Ignition Module (23) に電源供給 → エンジン正常動作
> - スタンド **DOWN** + **ニュートラル** → NEUTRAL SWITCH (14) がリレーをバイパス → エンジンは動作継続可（または始動可）
> - スタンド **DOWN** + **ギア入り** → MAIN RELAY (19) が開放 → Ignition Module (23) 電源カット → **点火停止**

```
IGNITION SWITCH (9)
     |
     v
MAIN RELAY (19) --------> IGNITION MODULE / IDS (23)
     ^                       [リレーで制御される電源供給]
     |
     +<-------- SIDE STAND SWITCH (16)
     |              [スタンド DOWN + ギア入り時にリレー接点を開放]
     |
     +<-------- NEUTRAL SWITCH (14)
                    [ニュートラル時はカットロジックをバイパスし、
                     スタンド位置に関わらずエンジン動作を許可]
```

**トラブルシューティングガイド:**

| 症状 | 考えられる原因 |
|---|---|
| スタンド展開 + ギア入り時にエンジンカット | 正常動作 |
| スタンドUP + ニュートラルでもエンジンカット | MAIN RELAY (19) または配線の不良 |
| スタンドDOWN + ギア入りでエンジンが動作 | SIDE STAND SWITCH (16) の断線（オープン故障） |
| スターターが回らない（スタンドUP、ニュートラル） | MAIN RELAY (19) が閉じない |

---

### 4. REV COUNTER & SPEEDOMETER

```
IGNITION MODULE (23)
     |
     | （タコメーター信号）
     v
REV COUNTER (5)

SPEEDOMETER (6) [機械式またはケーブル駆動]

ダッシュボードクラスターに収容されるメーター類：
  - ニュートラルインジケーター
  - 油圧警告灯
  - DASHBOARD WARNING LIGHTS (7) [12V-1.2W バルブ]
```

> ⚠️ **RPMタップ時の注意（クイックシフター実装者向け）:** IGNITION MODULE → REV COUNTER 間のタコメーター信号を Arduino でRPM計測に利用する場合、パルスは**クランク1回転あたり2パルスが不等間隔**で出力される（90° Lツイン構造により、2パルスは約270°と90°の非対称間隔）。単純に「直前のパルス間隔からRPMを計算」すると、間隔の長短で2倍近い誤差が生じる。詳細は `docs/domain/ignition.md` セクション2を参照。

---

## 注意事項

- **30A MAIN FUSE (25)** はバッテリープラス端子と FUSE BOX の間にインラインで設置されており、完全な電装系の不具合を診断する際は必ずここを最初に確認すること。FUSE BOX (30) 内の個別ヒューズは各サブ回路を保護している（典型的な割り当てはセクション1を参照）。
- **REGULATOR-RECTIFIER (8)** はオルタネーターからの3相交流を整流・安定化して充電用直流に変換する。不良品はアイドル時に過充電（>15V）または充電不足（<13V）を引き起こす。
- **SIDE STAND SWITCH (16)** は **MAIN RELAY (19)** を経由して Ignition Module (23) 電源をカットするが、トランスミッションが **ギア入り** の場合のみ動作する。**NEUTRAL SWITCH (14)** はニュートラル時にこのカットをバイパスする。これはイグニッションカットであり、スターター抑止ではない。完全なロジックとトラブルシューティング表はセクション3を参照。
- **IGNITION MODULE / IDS (23)** はエンジンケースに取り付けられた **PICK-UP (22a/22b)** からタイミング信号を受信する。PICK-UP のエアギャップと状態は正確な進角制御に不可欠である。
- **水平シリンダー用 SPARK PLUG (28)** と **垂直シリンダー用 SPARK PLUG (29)** にはそれぞれ専用のコイルが割り当てられている。一方のシリンダーで失火が発生した場合、通常は Ignition Module よりも該当するコイルまたはその高圧リードを疑う。
- **SPEEDOMETER (6)** は900SSにおいて機械式ケーブル駆動であり、電子センサーは存在しない。イタリア語の純正呼称は *Tachimetro*。項目5（REV COUNTER / *Contagiri*）や項目7（DASHBOARD WARNING LIGHTS / *Spia*）と混同しないこと。
- **MAIN RELAY (19)**（*Relè principale*）は点火系への電源経路を制御する。車体には複数のリレー（スターターリレー等）が存在するが、項目19は特に FUSE BOX 付近に設置されているこの点火系電源リレーを指す。
