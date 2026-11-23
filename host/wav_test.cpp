// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
// A manual tool: runs one 16-bit mono WAV file (looped to the given length) through beatadv.h's analysis the way the sketch does
// (a ring that the source fills, update() every `frame` ms) and prints what the tracker made of it, once a second.
//   wav_test file.wav rate secs frame_ms [true_bpm]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>
#include "../firmware/BeatAdventure_S0/beatadv.h"

int main(int argc, char** argv) {
  if (argc < 5) { fprintf(stderr, "usage: wav_test file.wav rate secs frame_ms [true_bpm]\n"); return 2; }
  FILE* f = fopen(argv[1], "rb");
  if (!f) { perror("open"); return 2; }
  std::vector<unsigned char> b;
  unsigned char tmp[65536]; size_t n;
  while ((n = fread(tmp, 1, sizeof tmp, f)) > 0) b.insert(b.end(), tmp, tmp + n);
  fclose(f);
  size_t i = 12, dataOff = 0, dataLen = 0;
  while (i + 8 <= b.size()) {
    uint32_t len = b[i + 4] | b[i + 5] << 8 | b[i + 6] << 16 | (uint32_t)b[i + 7] << 24;
    if (!memcmp(&b[i], "data", 4)) { dataOff = i + 8; dataLen = len; break; }
    i += 8 + len + (len & 1);
  }
  if (!dataOff) { fprintf(stderr, "no data chunk\n"); return 2; }
  const int16_t* pcm = (const int16_t*)&b[dataOff];
  size_t ns = (dataLen > b.size() - dataOff ? b.size() - dataOff : dataLen) / 2;
  uint32_t rate = (uint32_t)atoi(argv[2]);
  double secs = atof(argv[3]), frameMs = atof(argv[4]), trueBpm = argc > 5 ? atof(argv[5]) : 0;
  beatadv::State s;
  beatadv::init(s, 0);
  static int16_t ring[1024];
  memset(ring, 0, sizeof ring);
  uint32_t pos = 0;
  long written = 0, beats = 0, plain = 0, lockedF = 0, frames = 0, lastSec = -1;
  double nextFrame = 0, lo = 1e9, hi = 0;
  for (double t = 0; t < secs;) {
    nextFrame += frameMs / 1000.0;
    long target = (long)floor(nextFrame * rate);
    for (; written < target; written++) ring[pos++ & 1023] = pcm[written % ns];
    t = nextFrame;
    beatadv::update(s, (uint32_t)lround(t * 1000), ring, 1023, pos, rate);
    frames++;
    if (s.beatNow) { beats++; if (s.plainBeat) plain++; }
    if (t > 10) { if (s.bt.locked) lockedF++; double bp = 60.0 / s.T; if (bp < lo) lo = bp; if (bp > hi) hi = bp; }
    long sec = (long)t;
    if (sec != lastSec && sec % 5 == 0) { lastSec = sec; printf("  t=%3lds  %3.0f bpm  %s  conf %.2f  level %.2f  rms %.3f  oss %.1f\n", sec, 60.0 / s.T, s.bt.locked ? "PULSE" : "onset", s.bt.conf, s.level, s.rms, s.oss); }
  }
  printf("%s: loop %.2f s, locked %ld%% after 10 s, shown %.0f..%.0f bpm, beats %ld (plain %ld) in %.0f s%s\n", argv[1], (double)ns / rate, 100 * lockedF / (frames > 0 ? frames : 1), lo, hi, beats, plain, secs, "");
  if (trueBpm > 0) printf("  (reference %.0f bpm: %.1f beats a second would be %.0f)\n", trueBpm, trueBpm / 60, trueBpm / 60 * secs);
  return 0;
}
