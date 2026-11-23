# BeatAdventure

M5Stack Cardputer ADV(240 x 135)用の、音楽同期の冒険ゲームです。
[KeySpeaker](https://github.com/kariagepompadour/Cardputer-ADV-KeySpeaker) の姉妹品で、KeySpeaker のファームウェア(USB スピーカー、
USB と BLE のキーボード、エアマウス、ジョグダイヤル、内蔵マイク、Pico 2 の AUX ユニット、microSD の MP3)のゲーム画面を、
BeatAdventure のゲームに差し替えたものです。探検家は、音の大きさ、スペクトラム、拍に合わせて、走り、登り、泳ぎ、落ちながら旅をします。
探検家は自分で動くので、あなたは眺めるだけです。

[English](README.md)

## 現在の状態: 段階 B1(最初のステージ「客船」)。実機ではまだ試していません

- ゲーム画面は、WEB 版の最初のステージ **「客船」** です。探検家は左のはしごを登り(拍ごとに1段、曲が静かになるとしがみついて待ちます)、
  艦橋の屋根から海を眺め、屋根を渡り、右のはしごを降りて、タラップを下って岸に着きます。カモメ、雲、そして下には30帯域のイコライザが
  波しぶきになって動きます。絵は WEB 版に合わせてあります。岸に着いたあとは、次のステージ(浜辺)ができるまで、客船の最初に戻ります。
  遊んでいる間、画面に文字は出ません(KeySpeaker と同じです)。`d` キーで、1秒あたりのコマ数、解析と描画の時間、拍の数、ステージ名、日数、テンポが出ます(実機での確認用です)。
- `firmware/BeatAdventure_S0/beatadv.h` は、WEB 版の拍の追跡(`beat.js`)と音の解析を C++ に移したものです。コンピューター上では、
  WEB 版のテストと同じ結果になります(`host/run_tests.sh` の出力は、`node test/beat.test.js` と行ごとに一致します)。
  **Cardputer ADV の実機では、まだ何も試していません。** 1コマの時間、残りのメモリ、マイクの反応は、これから実機で測ります。
- 残りのステージは、1〜2個ずつ移します。`host/preview.cpp` で、コンピューター上でもゲームの絵を描けます(WEB 版との見比べ用)。

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
