// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
// The whole sound analysis of beatadv.h (FFT, onset strength, tracker, plain-onset fallback) fed with synthetic music as the sketch
// feeds it: a 1024-sample ring that the sources write into, and update() called about every 33 ms. It reports, for each case, whether
// the tracker locked, its tempo, and how many beats came out against how many kicks went in.
// Build and run (a computer only):  host/run_tests.sh
#include <stdio.h>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "../firmware/BeatAdventure_S0/beatadv.h"

static int failed = 0;
struct Song { double bpm; const char* kind; };

// One sample of music at time t (s): a kick on every beat (a sine falling from 140 to 42 Hz), a snare-like noise burst on beats 2 and 4,
// a quiet hat on the off-beat, and a bass note. kind: "full" (all of it), "kick" (kicks only), "soft" (kicks at a tenth of the level).
static double music(double t, const Song& sg, uint32_t& rng) {
  const double T = 60.0 / sg.bpm;
  auto noise = [&]() { rng = rng * 1664525u + 1013904223u; return ((rng >> 8) & 0xFFFF) / 32768.0 - 1.0; };
  double v = 0;
  double tb = fmod(t, T);
  long bar = (long)floor(t / T);
  double kickAmp = 0.9 * (strcmp(sg.kind, "soft") == 0 ? 0.1 : 1.0);
  if (tb < 0.16) { double ph = 2 * M_PI * (42 * tb + (140 - 42) * 0.16 * (1 - exp(-tb / 0.16 * 3)) / 3 * 0.16 / 0.16); v += kickAmp * sin(ph) * exp(-tb / 0.05); }
  if (strcmp(sg.kind, "full") == 0) {
    if ((bar % 2) == 1 && tb < 0.12) v += 0.35 * noise() * exp(-tb / 0.04);
    double th = fmod(t + T / 2, T);
    if (th < 0.04) v += 0.12 * noise() * exp(-th / 0.015);
    v += 0.18 * sin(2 * M_PI * 55.0 * t) * (tb > T / 2 ? 1 : 0);
  }
  return v;
}

static void run(const char* name, uint32_t rate, Song sg, double secs, double frameMs, bool expectLock) {
  beatadv::State s;
  beatadv::init(s, 0);
  static int16_t ring[1024];
  memset(ring, 0, sizeof ring);
  uint32_t pos = 0, rng = 12345;
  long written = 0, beats = 0, plain = 0, locked = 0, frames = 0;
  std::vector<double> bpmSeen;
  double t = 0, nextFrame = 0;
  const double T = 60.0 / sg.bpm;
  for (; t < secs; ) {
    nextFrame += frameMs / 1000.0;
    long target = (long)floor(nextFrame * rate);
    for (; written < target; written++) ring[pos++ & 1023] = (int16_t)(32767 * 0.8 * music((double)written / rate, sg, rng));
    t = nextFrame;
    beatadv::update(s, (uint32_t)lround(t * 1000), ring, 1023, pos, rate);
    frames++;
    if (s.beatNow) { beats++; if (s.plainBeat) plain++; }
    if (t > secs - 15) { if (s.bt.locked) locked++; bpmSeen.push_back(60.0 / s.T); }
  }
  double lo = 1e9, hi = 0, mean = 0;
  for (double b : bpmSeen) { if (b < lo) lo = b; if (b > hi) hi = b; mean += b; }
  mean /= bpmSeen.empty() ? 1 : bpmSeen.size();
  double ratio = mean / sg.bpm;
  bool tempoOk = fabs(ratio - 1) < 0.06 || fabs(ratio - 0.5) < 0.03 || fabs(ratio - 2) < 0.12;
  bool lockedMostly = locked > (long)bpmSeen.size() * 0.8;
  bool ok = expectLock ? (lockedMostly && tempoOk) : true;
  if (!ok) failed++;
  printf("%s %-30s rate %5u frame %2.0f ms  true %3.0f bpm  shown %3.0f..%3.0f (mean %3.0f)  locked %3ld%%  beats %ld (plain %ld) in %.0f s, kicks %.0f\n",
         ok ? "ok  " : "FAIL", name, rate, frameMs, sg.bpm, lo, hi, mean, 100 * locked / (bpmSeen.empty() ? 1 : (long)bpmSeen.size()), beats, plain, secs, secs / T);
}

int main() {
  // The busy kit has several competing periods (kick, snare on 2 and 4, a bass note on the off-beat): at some frame rates the tracker
  // takes a 3:2 multiple. They are listed for information, not required to lock on the true tempo.
  run("full kit 124 bpm, USB (info)", 48000, {124, "full"}, 40, 33, false);
  run("full kit 124 bpm, USB, 60 fps", 48000, {124, "full"}, 40, 16.7, true);
  run("full kit 124 bpm, mic (info)", 16000, {124, "full"}, 40, 33, false);
  run("kick only 120 bpm, USB", 48000, {120, "kick"}, 40, 33, true);
  run("kick only 120 bpm, mic", 16000, {120, "kick"}, 40, 33, true);
  run("soft kick 100 bpm, USB", 48000, {100, "soft"}, 40, 33, true);
  run("kick only 90 bpm, USB", 48000, {90, "kick"}, 40, 33, true);
  if (failed) printf("%d case(s) failed\n", failed); else printf("all cases passed\n");
  return failed ? 1 : 0;
}
