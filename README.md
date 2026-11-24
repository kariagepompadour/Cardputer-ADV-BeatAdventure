# BeatAdventure

A music-synced adventure for the M5Stack Cardputer ADV (240 x 135), and a sister of
[KeySpeaker](https://github.com/kariagepompadour/Cardputer-ADV-KeySpeaker). It is the KeySpeaker firmware (USB speaker, USB and BLE
keyboard, air mouse, jog dial, built-in microphone, Pico 2 AUX unit, MP3 from a microSD card) with the game screen replaced by the
BeatAdventure game: an explorer who runs, climbs, swims and falls through a journey that follows the music's level, spectrum and
beats. The explorer plays by himself; you watch.

**Try it in the browser first: https://kariagepompadour.github.io/BeatAdventure-WEB-DEMO/** (the web demo is the reference this firmware was
ported from.)

[日本語](README_JA.md)

## The journey

One long run that loops, in the order of the web demo:

1. **The liner**: he climbs the left ladder (a rung on every beat), looks out to sea from the bridge roof, crosses it, climbs down the right
   ladder and walks down the gangway.
2. **The beach**: a departing ship, wave crests, a monkey in a palm throwing coconuts, turtles and logs to jump, a masked hunter that
   chases him now and then, and a bush to hide behind when the music goes quiet.
3. **The forest** (hills, tiger, bear, apples) and **the wall**: a landslide he climbs, with falling rocks and condors.
4. **The vine ravine** and **the boat and the falls**: three vines across a ravine, then a boat over a waterfall.
5. **The river** (crocodiles, piranhas, water snakes) and **the flat forest** (ponds, a vine over the wide ponds, a golden mask on a
   pedestal, a hidden shaft).
6. **The brick tunnel** (arrow traps, a rolling boulder, a mine cart over broken rails, guardians who chase him) and **the treasury**: ten
   floors of ledges, ladders, gaps, ropes and gold, guardians with spears, the golden mask on the top floor. A missed jump drops him into the
   river, and **the waterway** carries him to a **water jet**, a **fall** and **the pyramid**, where three boulders roll down the steps and he
   crouches under each one. Then the forest again.
7. **The camp**: after more than a minute without music he sits by a fire under falling code, eats, and sleeps until sound returns; then
   the same scene goes on, one day later.

Quiet music darkens the world and nothing comes (suspense); loud parts brighten it and bring more.
There is no text on the game screen while it plays (as in KeySpeaker), except two short titles in the treasury ("THE TREASURY" on
arrival and "GOLDEN MASK" when he takes the mask). The `d` key shows the frames a second, the analysis and drawing times, the beats a
second, the stage, the day and the tempo, for checking on the device; with it shown, `n` and `p` jump to the next and previous stage.

## Status

All the stages of the web demo are ported. Each was run on one Cardputer ADV over USB audio as it was added: about 28 to 30 frames a
second, 4 to 16 ms of drawing a frame (the busiest place is the treasury), about 87 KB of heap left. Two things from development are worth
knowing:

- An early build crashed at start-up because the game's functions run on the 8 KB loop-task stack and one of them built a large state on it.
  That is fixed, and `firmware/build.sh` now refuses a build in which a game function uses more than 2.5 KB of stack.
- Once, in an early treasury build, the screen froze while the music kept playing; it did not happen again in about 20 minutes of watching
  and could not be reproduced on a computer. The cause is unknown. Since then the loop watchdog is on and the serial log records where the
  loop was; what the device does when it fires has not been tested.

The microphone input, BLE, the Pico 2 AUX unit and MP3 playback come from KeySpeaker and have not been re-tested with the game.

`firmware/BeatAdventure_S0/beatadv.h` is a C++ port of the web demo's beat tracker (`beat.js`) and its sound analysis; on a computer it gives
the same results as the web demo's own test. The game itself is `ba_game.h` (the stages), `ba_hero.h` (the explorer, a skeleton drawn as thick
lines) and `ba_gfx.h` (the drawing layer), plain C++ with no Arduino headers.

## Keys

The keys are KeySpeaker's (see its README for the full list). The ones used most: Space plays and pauses, `,` and `/` go to the previous and
next track (or jump in an MP3), `[` and `]` change the volume, `\` mutes, `Fn` + `I` switches the input (USB, microphone, AUX, MP3),
`Fn` + `M` switches the screen, `h` shows the help. Only in this game: `d` shows the numbers, and while they are shown `n` and `p` jump to the
next and previous stage.

## Build

```bash
firmware/build.sh BeatAdventure_S0
```

Needs `arduino-cli`, the `m5stack:esp32` core 3.3.10 and the libraries M5Cardputer, M5Unified and M5GFX; the settings and the
linker options are those of KeySpeaker (see `firmware/build.sh`). The app-only `BeatAdventure_S0.ino.bin` is about 1.2 MB: install it
through a launcher; do not use the Arduino IDE's **Upload**, which also writes the bootloader and the partition table.

Note: on the one device it was tried on, M5Launcher's web upload failed with "Update Fail: 3". The likely cause (not proven) was an app
slot of about 1.18 MB, smaller than the image. If you meet this, check the size of your launcher's app slot.

## Tests (a computer only)

```bash
host/run_tests.sh
```

Runs the beat tracker against the same synthetic signals as the web demo's `test/beat.test.js`, the sound analysis, the game's rules stage
by stage (the treasury, the waterway, the pyramid, the camp and others), and a whole run of one hour with random music, quiet stretches and
long silences under the address and undefined-behaviour checkers. Needs only a C++ compiler. `host/preview.cpp` draws the game on a computer,
for comparing with the web demo.

## Similar tools by the same author

- [KeySpeaker WEB DEMO](https://github.com/kariagepompadour/KeySpeaker-WEB-DEMO)
- [Cardputer-ADV-KeySpeaker](https://github.com/kariagepompadour/Cardputer-ADV-KeySpeaker): the firmware this one grew from.
- [BeatAdventure WEB DEMO](https://github.com/kariagepompadour/BeatAdventure-WEB-DEMO): the reference for the game
  ([play it in the browser](https://kariagepompadour.github.io/BeatAdventure-WEB-DEMO/)).

## License

MIT License. Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. See [`LICENSE`](LICENSE).
`firmware/BeatAdventure_S0/minimp3.h` is minimp3 (CC0, see `minimp3_LICENSE.txt`).
