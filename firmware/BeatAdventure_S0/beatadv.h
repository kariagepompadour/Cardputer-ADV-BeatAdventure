// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
/*
  beatadv.h — BeatAdventure: the game screen (stage B0-1: the skeleton).

  This replaces KeySpeaker's FlightViz.h. It has the same shape (init / update / render over a 240 x 135 RGB565 frame
  buffer laid out like M5Canvas's: 16-bit pixels, high byte first), so the sketch changes only where it calls it, and the same
  code also runs on a computer for tests (host/beat_test.cpp). No Arduino headers here.

  What is in it now:
    - BeatTracker: a C++ port of the web demo's beat.js (the web demo is the reference). Every 20 ms tick it takes an "onset
      strength", finds the tempo with an autocorrelation of the last 6 s (70 to 200 bpm, with a prior toward 120), finds the
      phase with a comb of beats, and runs a clock (a phase-locked loop) that fires a beat every period even when a kick is
      missing. When there is no clear pulse it says so (locked = false) and the caller falls back to plain onset detection.
    - The analysis the web demo's readAudio() does, from the newest 512 mono samples: the level, "sounding", a 512-point FFT, the
      30-band equalizer (each band against its own 2.5 s average), the onset strength and the plain bass onset.
      Web Audio's numbers are copied (Blackman window, magnitude / N in dB, -100 to -30 dB onto 0..255) so the constants tuned
      in the browser keep their meaning. Bands are given in Hz, so they follow the sample rate (48 kHz from USB, AUX and MP3,
      16 kHz from the built-in microphone, which has nothing above 8 kHz).
    - A stub screen (render): the equalizer, a flash on every beat, and a ring of four dots that count the beats.
  The game itself comes stage by stage and will live in this namespace.
*/
#pragma once
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "ba_gfx.h"
#include "ba_game.h"

namespace beatadv {

constexpr int DOT = 3;
constexpr int SHIP_DW = 9, SHIP_DH = 4, SHIP_W = SHIP_DW * DOT, SHIP_H = SHIP_DH * DOT;
// Placeholder for the hero in the input-switch effect (Fn+I): a 9 x 4 dot ship, W hull, B shade, M trim, C canopy.
static const char* const SHIP[SHIP_DH] = {
  "M........",
  "MWWCC....",
  "WWWWWWWWW",
  ".BMMMMB..",
};

// ---------------------------------------------------------------------------------------------------------------------
// The beat tracker (a port of web/beat.js). Times are in seconds; time scalars are double, the arrays float.
struct BeatTracker {
  static constexpr double DT = 0.02;                      // tick length
  static constexpr int N = 512, MINL = 15, MAXL = 50, WIN = 300;   // ring size, lags 0.30 .. 1.0 s, a 6 s window
  float buf[N];
  long cur;
  int count;
  float acc, avg, conf;
  double T, clk, nextB, lastFire, lastAn, cand;
  int candN, jumpN;
  bool locked;

  BeatTracker() { reset(); }
  void reset() {
    memset(buf, 0, sizeof buf);
    cur = -1; count = 0; acc = 0; avg = 1; T = 0.5; conf = 0; locked = false;
    clk = 0; nextB = 0; lastFire = -9; lastAn = -9; cand = 0; candN = 0; jumpN = 0;
  }
  float get(long i) const { return buf[((i % N) + N) % N]; }
  void push(float raw) {   // close one tick: store its onset strength relative to the running average
    cur++;
    avg += (raw - avg) * 0.02f;
    buf[((cur % N) + N) % N] = raw / (avg + 1e-3f);
    if (count < N) count++;
  }
  // Call with the time (s) and the onset strength; returns true when a beat falls on this call.
  bool feed(double now, float oss) {
    long ci = (long)floor(now / DT);
    if (cur < 0) cur = ci - 1;
    if (ci - cur > 150) { float a = avg; reset(); avg = a; cur = ci - 1; }   // a long pause: start again
    if (ci > cur + 1) { push(acc); acc = 0; while (cur < ci - 1) push(0); }   // ticks with no call count as silence
    if (oss > acc) acc = oss;
    if (now - lastAn >= 0.4 && count >= 150) { lastAn = now; analyse(now); }
    if (!locked) return false;
    if (now >= nextB) {   // the clock rings: a beat, and the next one is set on the grid at least 0.3 T away
      lastFire = now;
      double a = clk + T * ceil((now + 0.3 * T - clk) / T), b = now + 0.45 * T;
      nextB = a > b ? a : b;
      return true;
    }
    return false;
  }
  void analyse(double now) {
    int w = count - 1 < WIN ? count - 1 : WIN;
    long end = cur;   // the newest finished tick
    float x[WIN];
    float m = 0;
    for (int k = 0; k < w; k++) { x[k] = get(end - k); m += x[k]; }
    m /= w;
    float v0 = 1e-6f;
    for (int k = 0; k < w; k++) { x[k] -= m; v0 += x[k] * x[k]; }
    v0 /= w;
    float ac[2 * MAXL + 2];
    for (int i = 0; i < 2 * MAXL + 2; i++) ac[i] = 0;
    for (int L = MINL; L <= 2 * MAXL; L++) {
      float s = 0;
      for (int k = 0; k + L < w; k++) s += x[k] * x[k + L];
      ac[L] = s / (w - L) / v0;
    }
    float score[MAXL + 2];
    for (int i = 0; i < MAXL + 2; i++) score[i] = 0;
    int bL = -1;
    float bS = -1e9f;
    for (int L = MINL; L <= MAXL; L++) {
      float lg = log2f((float)(L * DT / 0.5)) / 0.8f, prior = expf(-0.5f * lg * lg);   // a prior toward 120 bpm (0.5 s)
      score[L] = (ac[L] + 0.5f * ac[2 * L]) * (0.4f + 0.6f * prior);
      if (score[L] > bS) { bS = score[L]; bL = L; }
    }
    float Lr = (float)bL;
    if (bL > MINL && bL < MAXL) {   // a parabola through the peak: finer than a tick
      float a = score[bL - 1], b = score[bL], c = score[bL + 1], d = a - 2 * b + c;
      if (d < 0) Lr = bL + 0.5f * (a - c) / d;
    }
    conf = ac[bL];
    if (locked ? conf < 0.1f : conf < 0.2f) { locked = false; return; }
    double Tn = Lr * DT;
    if (!locked) { T = Tn; locked = true; candN = 0; clk = NAN; }
    else if (fabs(Tn - T) / T < 0.06) { T += (Tn - T) * 0.35; candN = 0; }
    else {   // a new tempo has to be seen three times in a row
      if (candN && fabs(Tn - cand) / cand < 0.06) candN++; else { cand = Tn; candN = 1; }
      if (candN >= 3) { T = Tn; candN = 0; clk = NAN; }
    }
    // the phase: slide a comb of beats at this tempo over the window and keep the offset where the onsets pile up
    double Tt = T;
    int steps = (int)lround(Tt / DT), bo = 0;
    float bp = -1e9f;
    for (int o = 0; o < steps; o++) {
      float s = 0;
      for (double t = o; t < w; t += Tt / DT) {
        int k = (int)round(t);
        s += (k - 1 >= 0 && k - 1 < w ? x[k - 1] : 0) + 2 * (k >= 0 && k < w ? x[k] : 0) + (k + 1 >= 0 && k + 1 < w ? x[k + 1] : 0);
      }
      if (s > bp) { bp = s; bo = o; }
    }
    double est = (end - bo) * DT + DT / 2;   // the time of the most recent beat in the data
    if (isnan(clk)) { clk = est; nextB = clk + Tt * ceil((now - clk) / Tt); jumpN = 0; return; }
    double err = fmod(fmod(est - clk, Tt) + Tt, Tt);
    if (err > Tt / 2) err -= Tt;
    if (fabs(err) < 0.25 * Tt) { clk += err * 0.3; nextB += err * 0.3; jumpN = 0; }   // small disagreement: nudge the clock
    else if (++jumpN >= 3) {   // large and consistent: move the clock
      clk = est; jumpN = 0;
      double a = clk + Tt * ceil((now + 0.3 * Tt - clk) / Tt), b = lastFire + 0.45 * Tt;
      nextB = a > b ? a : b;
    }
  }
};

// ---------------------------------------------------------------------------------------------------------------------
// The analysis of the sound (the web demo's readAudio()).
constexpr int FFT_N = 512, FFT_LOG2 = 9, EQ_BANDS = 30;

struct State {
  BeatTracker bt;
  bool ready = false;
  double lastNow = 0, soundAt = -9, lastBeat = -9;
  float rms = 0, peak = 0.05f, level = 0, pulse = 0;   // rms 0..1 of full scale; level 0..1 against the recent peak
  bool sounding = false, beatNow = false, plainBeat = false;   // plainBeat: this beat came from the bass onset (no clear pulse)
  long beatN = 0;
  float T = 0.5f, ivs[8];
  int nIv = 0;
  float prevE = 0, avgFlux = 2;
  float pl[FFT_N / 2];                 // last frame's linear spectrum, for the onset strength
  float eqAvg[EQ_BANDS], eqH[EQ_BANDS];   // each band's own 2.5 s average (dB); the bar heights (0..8)
  bool eqSet = false;
  uint32_t rate = 0;
  uint16_t bandBin[EQ_BANDS + 1];      // band edges in bins
  uint16_t ossBin[4];                  // the onset strength's three bands in bins: [0,1) [1,2) [2,3)
  uint16_t flat[2];                    // the plain onset detector's bins
  float oss = 0;
  Game game;                           // the adventure
};

// log2 of a positive float, within 0.005: the exponent plus a cubic through the mantissa (a Cardputer frame has 255 of them).
static inline float fastLog2(float x) {
  union { float f; uint32_t i; } u = {x};
  float e = (float)((int)((u.i >> 23) & 255) - 127);
  u.i = (u.i & 0x007FFFFFu) | 0x3F800000u;
  float m = u.f;
  return e + ((-0.34484843f * m + 2.02466578f) * m - 0.67487759f);
}

static float fftRe[FFT_N], fftIm[FFT_N], win[FFT_N], twC[FFT_N / 2], twS[FFT_N / 2];
static bool fftReady = false;

static void setupRate(State& s, uint32_t rate) {
  float binHz = (float)rate / FFT_N;
  auto bin = [&](float hz) { int b = (int)lrintf(hz / binHz); return b < 1 ? 1 : b > FFT_N / 2 - 1 ? FFT_N / 2 - 1 : b; };
  // The web demo's bands are in bins of a 1024-point FFT at 44.1 / 48 kHz (about 45 Hz): bins 1..6, 7..24, 25..90 are about 45-290 Hz,
  // 300-1100 Hz and 1.1-4.1 kHz. The same ranges in Hz here.
  s.ossBin[0] = (uint16_t)bin(45); s.ossBin[1] = (uint16_t)(bin(295) + 1); s.ossBin[2] = (uint16_t)(bin(1100) + 1); s.ossBin[3] = (uint16_t)(bin(4100) + 1);
  if (s.ossBin[1] <= s.ossBin[0]) s.ossBin[1] = s.ossBin[0] + 1;
  if (s.ossBin[2] <= s.ossBin[1]) s.ossBin[2] = s.ossBin[1] + 1;
  if (s.ossBin[3] <= s.ossBin[2]) s.ossBin[3] = s.ossBin[2] + 1;
  int lo = bin(94), hi = bin(fminf(16000.0f, rate * 0.47f)), last = lo - 1;   // 30 bands, even in pitch, from 94 Hz up
  for (int b = 0; b <= EQ_BANDS; b++) {
    int e = (int)lrintf(lo * powf((float)hi / lo, (float)b / EQ_BANDS));
    if (e <= last) e = last + 1;
    if (e > FFT_N / 2) e = FFT_N / 2;
    s.bandBin[b] = (uint16_t)e;
    last = e;
  }
  s.rate = rate;
}

static void init(State& s, uint32_t nowMs) {
  s = State();
  s.lastNow = nowMs / 1000.0;
  s.ready = true;
  gameInit(s.game, s.lastNow);
}

static AudioIn audioIn(const State& s) {
  AudioIn a; a.beatNow = s.beatNow; a.sounding = s.sounding; a.T = s.T; a.level = s.level; a.pulse = s.pulse; a.eqH = s.eqH; a.beatN = s.beatN;
  return a;
}

// One frame of the audio side: the newest FFT_N samples of ring (mono, mask + 1 samples, pos = the next write index) at `rate` Hz.
static void update(State& s, uint32_t nowMs, const int16_t* ring, uint32_t mask, uint32_t pos, uint32_t rate) {
  const double now = nowMs / 1000.0;
  float dt = (float)(now - s.lastNow);
  if (dt < 0.001f) dt = 0.001f;
  if (dt > 0.1f) dt = 0.1f;
  const double prevNow = s.lastNow;
  s.lastNow = now;
  if (!fftReady) {
    for (int i = 0; i < FFT_N; i++) {   // Web Audio's Blackman window (alpha 0.16)
      float a = 2.0f * (float)M_PI * i / FFT_N;
      win[i] = 0.42f - 0.5f * cosf(a) + 0.08f * cosf(2 * a);
    }
    for (int i = 0; i < FFT_N / 2; i++) twC[i] = cosf(2.0f * (float)M_PI * i / FFT_N), twS[i] = -sinf(2.0f * (float)M_PI * i / FFT_N);
    fftReady = true;
  }
  if (s.rate != rate) { setupRate(s, rate); memset(s.pl, 0, sizeof s.pl); }
  s.beatNow = false; s.plainBeat = false;

  float sq = 0;
  for (int i = 0; i < FFT_N; i++) {   // windowed, in bit-reversed order
    float v = ring[(pos - FFT_N + i) & mask] * (1.0f / 32768.0f);
    sq += v * v;
    int r = 0;
    for (int b = 0; b < FFT_LOG2; b++) r |= ((i >> b) & 1) << (FFT_LOG2 - 1 - b);
    fftRe[r] = v * win[i];
    fftIm[r] = 0;
  }
  s.rms = sqrtf(sq / FFT_N);
  if (s.rms > 0.002f) s.soundAt = now;
  s.sounding = now - s.soundAt < 0.6;   // a 0.6 s hold: the gaps between kicks are not silence
  s.peak *= powf(0.9996f, 60.0f * dt);   // the web demo's per-frame constants, at 60 frames a second
  if (s.rms > s.peak) s.peak = s.rms;
  if (s.peak < 0.03f) s.peak = 0.03f;
  float lv = s.rms / s.peak;
  if (lv > 1) lv = 1;
  if (lv > s.level) s.level += (lv - s.level) * (1.0f - powf(0.5f, 60.0f * dt));
  else s.level *= powf(0.985f, 60.0f * dt);

  for (int len = 2; len <= FFT_N; len <<= 1) {
    int half = len / 2, step = FFT_N / len;
    for (int i = 0; i < FFT_N; i += len)
      for (int k = 0; k < half; k++) {
        float c = twC[k * step], sn = twS[k * step];
        int a = i + k, b = a + half;
        float xr = fftRe[b] * c - fftIm[b] * sn, xi = fftRe[b] * sn + fftIm[b] * c;
        fftRe[b] = fftRe[a] - xr, fftIm[b] = fftIm[a] - xi;
        fftRe[a] += xr, fftIm[a] += xi;
      }
  }
  // Per bin: Web Audio's byte value (magnitude / N in dB, -100 .. -30 dB onto 0 .. 255) and its cube, the "lin" of the web demo
  // (cubing brings back the contrast, so a loud kick counts for more than a soft bass note).
  static float lin[FFT_N / 2], byteV[FFT_N / 2], pw[FFT_N / 2];
  for (int k = 1; k < FFT_N / 2; k++) {
    float p2 = (fftRe[k] * fftRe[k] + fftIm[k] * fftIm[k]) * (1.0f / ((float)FFT_N * FFT_N));   // power: (magnitude / N) squared
    pw[k] = p2;
    float d = 3.0102999566f * fastLog2(p2 + 1e-24f);   // 20 log10(magnitude / N) = 10 log10(power) = 3.0103 log2(power)
    float bv = (d + 100.0f) / 70.0f * 255.0f;
    bv = bv < 0 ? 0 : bv > 255 ? 255 : bv;
    byteV[k] = bv;
    float a = bv / 255.0f;
    lin[k] = a * a * a;
  }
  // The equalizer: each band's loudness in dB against ITS OWN average of the last 2.5 s, so every band moves with the music.
  {
    float bdb[EQ_BANDS];
    for (int b = 0; b < EQ_BANDS; b++) {
      float p = 0;
      for (int k = s.bandBin[b]; k < s.bandBin[b + 1] && k < FFT_N / 2; k++) p += pw[k];
      bdb[b] = 3.0102999566f * fastLog2(p + 1e-12f);
    }
    bool silent = s.rms < 0.001f;
    if (!silent && !s.eqSet) { for (int b = 0; b < EQ_BANDS; b++) s.eqAvg[b] = bdb[b]; s.eqSet = true; }
    float ek = 1.0f - expf(-dt / 2.5f);
    for (int b = 0; b < EQ_BANDS; b++) {
      float t = 0;
      if (!silent) { t = 0.4f + (bdb[b] - s.eqAvg[b]) / 12.0f; s.eqAvg[b] += (bdb[b] - s.eqAvg[b]) * ek; }
      t = t < 0 ? 0 : t > 1 ? 1 : t;
      if (t * 8 > s.eqH[b]) s.eqH[b] = t * 8;
      s.eqH[b] -= 28.0f * dt;
      if (s.eqH[b] < 0) s.eqH[b] = 0;
    }
  }
  // The plain bass onset (the fallback), and the onset strength for the tracker.
  float e = 0;
  int ne = s.ossBin[1] - s.ossBin[0];
  for (int k = s.ossBin[0]; k < s.ossBin[1]; k++) e += byteV[k];
  e /= ne;
  float flux = e - s.prevE;
  s.prevE = e;
  s.avgFlux += ((flux > 0 ? flux : 0) - s.avgFlux) * 0.03f;
  auto band = [&](int lo, int hi) { float t = 0; for (int k = lo; k < hi; k++) { float d = lin[k] - s.pl[k]; t += d > 0 ? d : 0; } return t / (hi - lo) * 100.0f; };
  s.oss = band(s.ossBin[0], s.ossBin[1]) + 0.7f * band(s.ossBin[1], s.ossBin[2]) + 0.4f * band(s.ossBin[2], s.ossBin[3]);
  for (int k = 1; k < s.ossBin[3] && k < FFT_N / 2; k++) s.pl[k] = lin[k];

  // Feed the tracker once for every 20 ms tick since the last call (a frame slower than a tick holds its onset strength).
  bool clock = false;
  const float o = s.sounding ? s.oss : 0;
  long n0 = (long)floor(prevNow / BeatTracker::DT), n1 = (long)floor(now / BeatTracker::DT);
  for (long n = n0 + 1; n < n1; n++) clock |= s.bt.feed((n + 0.5) * BeatTracker::DT, o);
  clock |= s.bt.feed(now, o);
  bool beat = false;
  if (s.bt.locked) { s.T = (float)s.bt.T; beat = clock && s.sounding; }   // a clear pulse: the clock rings on time, even through a missing kick
  else if (flux > fmaxf(s.avgFlux * 2.2f, 8.0f) && now - s.lastBeat > 0.3 && s.sounding) {   // no clear pulse: a bass hit counts as a beat
    double iv = now - s.lastBeat;
    if (iv > 0.3 && iv < 0.9) {
      if (s.nIv == 8) { for (int i = 1; i < 8; i++) s.ivs[i - 1] = s.ivs[i]; s.nIv = 7; }
      s.ivs[s.nIv++] = (float)iv;
      float t[8]; memcpy(t, s.ivs, sizeof(float) * s.nIv);
      for (int i = 1; i < s.nIv; i++) for (int j = i; j > 0 && t[j] < t[j - 1]; j--) { float x = t[j]; t[j] = t[j - 1]; t[j - 1] = x; }
      s.T = t[s.nIv >> 1];
    }
    beat = true; s.plainBeat = true;
  }
  if (beat) { s.lastBeat = now; s.beatNow = true; s.beatN++; s.pulse = 1; }
  s.pulse *= powf(0.9f, 60.0f * dt);
  gameUpdate(s.game, audioIn(s), dt, now);
}

// ---------------------------------------------------------------------------------------------------------------------
// Drawing: the game's picture into the frame buffer (byte-swapped RGB565, as in M5Canvas's buffer).
static void render(State& s, uint16_t* fb, uint32_t nowMs) {
  gameRender(s.game, audioIn(s), fb, nowMs / 1000.0);
}
static const char* stageName(const State& s) { return gameStageName(s.game); }
static int dayNumber(const State& s) { return s.game.day; }

}  // namespace beatadv
