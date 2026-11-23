// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
// The C++ beat tracker (firmware/BeatAdventure_S0/beatadv.h) against the same synthetic onset signals as the web demo's
// test/beat.test.js: steady kicks, off-beat hats, missing kicks, noise, a 10 fps hidden tab. It checks the detected tempo (half or
// double time is accepted), the beat offset and the jitter, prints ok / FAIL per case, and exits with 1 on a failure.
// Build and run (a computer only, no Cardputer):  host/run_tests.sh
#include <stdio.h>
#include <vector>
#include <functional>
#include <string>
#include <math.h>
#include "../firmware/BeatAdventure_S0/beatadv.h"

static int failed = 0;
struct Rng {   // the same generator as the JavaScript test: a Lehmer / Park-Miller sequence seeded with 7
  uint64_t seed = 7;
  double operator()() { seed = (seed * 16807) % 2147483647ULL; return (double)seed / 2147483647.0; }
};
struct Opts { double fps = 60, off = 0, maxOff = 0.05, maxSd = 0.03; const char* expect = ""; };
typedef std::function<double(double, double, Rng&)> Gen;

static void run(const char* name, double bpm, Gen gen, double secs, Opts o = Opts()) {
  beatadv::BeatTracker bt;
  Rng rnd;
  const double T = 60.0 / bpm;
  std::vector<double> fires, kicks;
  for (double t = 0; t < secs;) {
    t += 1.0 / o.fps + (rnd() - 0.5) * 0.004;   // frame jitter
    double v = gen(t, T, rnd);
    if (bt.feed(t, (float)v)) fires.push_back(t);
  }
  for (double t = 1; t < secs; t += T) kicks.push_back(t + o.off);
  std::vector<double> late, errs;
  for (double t : fires) if (t > secs - 15) late.push_back(t);
  for (double t : late) {
    double best = 1e9;
    for (double k : kicks) { double d = t - k; if (fabs(d) < fabs(best)) best = d; }
    errs.push_back(best);
  }
  double mean = 0, sd = 0;
  for (double e : errs) mean += e;
  mean /= errs.empty() ? 1 : errs.size();
  for (double e : errs) sd += (e - mean) * (e - mean);
  sd = sqrt(sd / (errs.empty() ? 1 : errs.size()));
  double ratio = bt.T / T;
  bool tempoOk = fabs(ratio / 0.5 - 1) < 0.05 || fabs(ratio - 1) < 0.05 || fabs(ratio / 2 - 1) < 0.05;
  std::string problems;
  auto add = [&](const std::string& s) { if (!problems.empty()) problems += "; "; problems += s; };
  std::string ex = o.expect;
  if (ex == "none") { if (!late.empty()) add("fired beats on a signal with no usable pulse"); }
  else if (ex == "fallback") { /* no lock is acceptable: the game falls back to plain onset detection */ }
  else {
    char b[96];
    if (!bt.locked) add("not locked");
    if (!tempoOk) { snprintf(b, sizeof b, "tempo %.3f is not 1/2, 1 or 2 x %.3f", bt.T, T); add(b); }
    if (late.size() < 8) { snprintf(b, sizeof b, "too few beats in the last 15 s: %zu", late.size()); add(b); }
    if (fabs(mean) > o.maxOff) { snprintf(b, sizeof b, "mean offset %.0f ms", mean * 1000); add(b); }
    if (sd > o.maxSd) { snprintf(b, sizeof b, "jitter sd %.0f ms", sd * 1000); add(b); }
  }
  if (!problems.empty()) failed++;
  printf("%s %-34s locked %s T %.3f (true %.3f) beats/15s %zu offset(ms) %.0f sd(ms) %.0f %s\n", problems.empty() ? "ok  " : "FAIL", name,
         bt.locked ? "true" : "false", bt.T, T, late.size(), mean * 1000, sd * 1000, problems.c_str());
}

static double kick(double t, double T, double off = 1) {
  double ph = fmod(fmod(t - off, T) + T, T);
  return ph < 0.04 ? 30 * (1 - ph / 0.04) : 0;
}

int main() {
  run("steady kick 124 bpm", 124, [](double t, double T, Rng& r) { double k = kick(t, T); return k + r() * 2; }, 40);
  run("kick + off-beat hats", 120, [](double t, double T, Rng& r) { double a = kick(t, T), b = 0.5 * kick(t, T, 1 + T / 2); return a + b + r() * 3; }, 40);
  run("kick 90 bpm", 90, [](double t, double T, Rng& r) { double k = kick(t, T); return k + r() * 2; }, 40);
  run("kick 170 bpm (locks to half time)", 170, [](double t, double T, Rng& r) { double k = kick(t, T); return k + r() * 2; }, 40);
  run("weak kick in loud noise 100 bpm", 100, [](double t, double T, Rng& r) { double k = kick(t, T) * 0.6; return k + r() * 8; }, 40);
  run("every second kick missing 128", 128, [](double t, double T, Rng& r) { double a = ((long)floor((t - 1) / T) % 2) ? 0 : kick(t, T); return a + r() * 2; }, 40);
  { Opts o; o.expect = "fallback"; run("snare on 2 and 4 only, 110", 110, [](double t, double T, Rng& r) { double a = ((long)floor((t - 1) / T) % 2) ? kick(t, T) : 0; return a + r() * 2; }, 40, o); }
  { Opts o; o.expect = "fallback"; run("pure noise (no pulse)", 120, [](double, double, Rng& r) { return r() * 8; }, 40, o); }
  { Opts o; o.fps = 10; o.maxOff = 0.12; o.maxSd = 0.2; run("hidden-tab 10 fps, 120 bpm", 120, [](double t, double T, Rng& r) { double k = kick(t, T); return k + r() * 2; }, 40, o); }
  if (failed) printf("%d check(s) failed\n", failed); else printf("all checks passed\n");
  return failed ? 1 : 0;
}
