# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

Ducati 900SS（1995年式）に Arduino Nano を使ったクイックシフターを実装するプロジェクト。コード生成は Claude Code がすべて担当する。実装はまだ未定。

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
