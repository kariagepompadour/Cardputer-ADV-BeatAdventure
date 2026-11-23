# BeatAdventure

A music-synced adventure for the M5Stack Cardputer ADV (240 x 135), and a sister of
[KeySpeaker](https://github.com/kariagepompadour/Cardputer-ADV-KeySpeaker). It is the KeySpeaker firmware (USB speaker, USB and BLE
keyboard, air mouse, jog dial, built-in microphone, Pico 2 AUX unit, MP3 from a microSD card) with the game screen replaced by the
BeatAdventure game: an explorer who runs, climbs, swims and falls through a journey that follows the music's level, spectrum and
beats. The explorer plays by himself; you watch.

[日本語](README_JA.md)

## Status: stage B0-1, the skeleton (not tried on a device)

- The game screen is still a stub that shows the beat tracker at work: the 30-band equalizer, a flash on every beat, a ring of four
  dots that count the beats, and the tempo, whether it holds a clear pulse, its confidence and the level.
- `firmware/BeatAdventure_S0/beatadv.h` is a C++ port of the web demo's beat tracker (`beat.js`) and of its sound analysis. On a
  computer it gives the same results as the web demo's own test (`host/run_tests.sh`; the output matches `node test/beat.test.js`
  line for line). **Nothing has been tried on a Cardputer ADV yet**: the frame time, the memory left and the microphone's response are
  still to be measured there.
- The game itself (the stages of the web demo) is ported stage by stage.

## Build

```bash
firmware/build.sh BeatAdventure_S0
```

Needs `arduino-cli`, the `m5stack:esp32` core 3.3.10 and the libraries M5Cardputer, M5Unified and M5GFX; the settings and the
linker options are those of KeySpeaker (see `firmware/build.sh`). Install the app-only `BeatAdventure_S0.ino.bin` through a launcher;
do not use the Arduino IDE's **Upload**, which also writes the bootloader and the partition table.

## Tests (a computer only)

```bash
host/run_tests.sh
```

Feeds the beat tracker the same synthetic onset signals as the web demo's `test/beat.test.js` (steady kicks, off-beat hats, missing
kicks, noise, a 10 fps hidden tab) and checks the tempo, the beat offset and the jitter. Needs only a C++ compiler.

## Similar tools by the same author

- [KeySpeaker WEB DEMO](https://github.com/kariagepompadour/KeySpeaker-WEB-DEMO)
- [Cardputer-ADV-KeySpeaker](https://github.com/kariagepompadour/Cardputer-ADV-KeySpeaker): the firmware this one grew from.
- BeatAdventure WEB DEMO: the reference for the game (its address will be added after it is published).

## License

MIT License. Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. See [`LICENSE`](LICENSE).
`firmware/BeatAdventure_S0/minimp3.h` is minimp3 (CC0, see `minimp3_LICENSE.txt`).
