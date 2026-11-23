# BeatAdventure

M5Stack Cardputer ADV(240 x 135)用の、音楽同期の冒険ゲームです。
[KeySpeaker](https://github.com/kariagepompadour/Cardputer-ADV-KeySpeaker) の姉妹品で、KeySpeaker のファームウェア(USB スピーカー、
USB と BLE のキーボード、エアマウス、ジョグダイヤル、内蔵マイク、Pico 2 の AUX ユニット、microSD の MP3)のゲーム画面を、
BeatAdventure のゲームに差し替えたものです。探検家は、音の大きさ、スペクトラム、拍に合わせて、走り、登り、泳ぎ、落ちながら旅をします。
探検家は自分で動くので、あなたは眺めるだけです。

[English](README.md)

## 現在の状態: 段階 B0-1(骨組み)。実機ではまだ試していません

- ゲーム画面は、まだ仮のものです。拍の追跡のようすを見せます。30帯域のイコライザ、拍ごとの点滅、拍を数える4つの点、そしてテンポ、
  はっきりした拍があるか、その確からしさ、音の大きさを表示します。
- `firmware/BeatAdventure_S0/beatadv.h` は、WEB 版の拍の追跡(`beat.js`)と音の解析を C++ に移したものです。コンピューター上では、
  WEB 版のテストと同じ結果になります(`host/run_tests.sh` の出力は、`node test/beat.test.js` と行ごとに一致します)。
  **Cardputer ADV の実機では、まだ何も試していません。** 1コマの時間、残りのメモリ、マイクの反応は、これから実機で測ります。
- ゲーム本体(WEB 版の各ステージ)は、1つずつ移します。

## ビルド

```bash
firmware/build.sh BeatAdventure_S0
```

`arduino-cli`、`m5stack:esp32` コア 3.3.10、ライブラリ M5Cardputer・M5Unified・M5GFX が必要です。設定とリンカーのオプションは
KeySpeaker と同じです(`firmware/build.sh` を見てください)。できた `BeatAdventure_S0.ino.bin` を、ランチャーから入れてください。
Arduino IDE の **Upload** は使わないでください。ブートローダーとパーティションテーブルも書き換えてしまいます。

## テスト(コンピューターだけで動きます)

```bash
host/run_tests.sh
```

WEB 版の `test/beat.test.js` と同じ人工の信号(一定のキック、裏拍のハイハット、抜けたキック、ノイズ、10fps の隠れたタブ)を
拍の追跡に入れて、テンポ、拍のずれ、ばらつきを確かめます。C++ のコンパイラーだけで動きます。

## 同じ作者の似たツール

- [KeySpeaker WEB DEMO](https://github.com/kariagepompadour/KeySpeaker-WEB-DEMO)
- [Cardputer-ADV-KeySpeaker](https://github.com/kariagepompadour/Cardputer-ADV-KeySpeaker): この作品の元になったファームウェア。
- BeatAdventure WEB DEMO: ゲームの手本です(公開後にアドレスを追記します)。

## ライセンス

MIT License。Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation。[`LICENSE`](LICENSE) を見てください。
`firmware/BeatAdventure_S0/minimp3.h` は minimp3(CC0、`minimp3_LICENSE.txt` を参照)です。
