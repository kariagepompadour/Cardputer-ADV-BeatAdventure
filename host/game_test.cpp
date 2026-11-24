// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
// Game rules checked on a computer: passing a tree in either forest takes its apple (a bug in B11 left the flat forest without it).
#include <stdio.h>
#include <initializer_list>
#include "../firmware/BeatAdventure_S0/ba_game.h"
using namespace beatadv;
int main() {
  int bad = 0;
  for (int sc : {(int)SC_FOREST, (int)SC_FOREST3}) {
    Game G; gameInit(G, 0); setScene(G, (Scene)sc, 0);
    static float eq[30]; AudioIn A; A.sounding = true; A.T = 0.5f; A.eqH = eq; A.level = 0.7f; A.rms = 0.1f;
    double t = 0; const float dt = 1.0f / 30;
    for (int f = 0; f < 30 * 40; f++, t += dt) { A.beatNow = (f % 15) == 0; gameUpdate(G, A, dt, t); }
    const bool ok = G.bc.nApples >= 10;
    printf("%s  scene %d: apples taken %d in 40 s\n", ok ? "ok  " : "FAIL", sc, G.bc.nApples);
    if (!ok) bad++;
  }
  {   // the treasure: in the flat forest the pedestal comes after about 80 s of music; he takes the mask and falls into the hidden shaft
    Game G; gameInit(G, 0); setScene(G, SC_FOREST3, 0);
    static float eq[30]; AudioIn A; A.sounding = true; A.T = 0.5f; A.eqH = eq; A.level = 0.7f; A.rms = 0.1f;
    double t = 0; const float dt = 1.0f / 30; int relics = 0; bool left = false;
    for (int f = 0; f < 30 * 130; f++, t += dt) { A.beatNow = (f % 15) == 0; gameUpdate(G, A, dt, t); if (G.scene == SC_FOREST3) relics = G.bc.relics; else left = true; if (left) break; }
    const bool ok = relics >= 1 && left;
    printf("%s  treasure: mask taken %d, left the forest %d (at %.0f s)\n", ok ? "ok  " : "FAIL", relics, (int)left, t);
    if (!ok) bad++;
  }
  {   // the tunnel: a hole down, a ladder up, then the golden doorway (not before about 86 s); then he leaves the tunnel
    Game G; gameInit(G, 0); setScene(G, SC_CAVE, 0);
    static float eq[30]; AudioIn A; A.sounding = true; A.T = 0.5f; A.eqH = eq; A.level = 0.7f; A.rms = 0.1f;
    double t = 0; const float dt = 1.0f / 30; int maxDepth = 0, gate = 0, boulder = 0, arrow = 0, ride = 0, chase = 0; double leftAt = 0;
    for (int f = 0; f < 30 * 140; f++, t += dt) { A.beatNow = (f % 15) == 0; gameUpdate(G, A, dt, t); if (G.scene != SC_CAVE) { leftAt = t; break; } if (G.cv.depth > maxDepth) maxDepth = G.cv.depth; if (G.cv.mv.live && G.cv.mv.kind == MV_GATE) gate++; if (G.cv.boulder.live && G.cv.boulder.roll) boulder++; for (const auto& a : G.cv.arrows) if (a.live) arrow++; if (G.cv.cart) ride++; if (G.cv.chase.live) chase++; }
    const bool ok = maxDepth == 1 && gate > 0 && leftAt >= 86 && boulder > 0 && arrow > 0 && ride > 0 && chase > 0;
    printf("%s  tunnel: deepest level %d, doorway reached %d, boulder frames %d, arrow frames %d, cart frames %d, guardian frames %d, left at %.0f s\n", ok ? "ok  " : "FAIL", maxDepth, gate > 0, boulder, arrow, ride, chase, leftAt);
    if (!ok) bad++;
  }
  {   // the waterway: a very quiet song (volume under 0.35) makes him miss the first gap of the treasury and drop into the river; the splash leads straight into the channel, which ends at the water source
    Game G; gameInit(G, 0); setScene(G, SC_CAVERN, 0);
    static float eq[30]; AudioIn A; A.sounding = true; A.T = 0.5f; A.eqH = eq; A.level = 0.3f; A.rms = 0.1f;
    double t = 0; const float dt = 1.0f / 30; double enteredAt = 0, leftAt = 0; bool entry = false, exitSeen = false; float maxScroll = 0; int objs = 0;
    for (int f = 0; f < 30 * 120; f++, t += dt) { A.beatNow = (f % 15) == 0; gameUpdate(G, A, dt, t);
      if (G.scene == SC_WATERWAY) { if (!enteredAt) enteredAt = t; if (G.rv.entryLive) entry = true; if (G.rv.exitLive) exitSeen = true; if (G.rv.scroll > maxScroll) maxScroll = G.rv.scroll; for (const RvObs& o : G.rv.obs) if (o.live) objs++; }
      else if (enteredAt) { leftAt = t; break; } }
    const bool ok = enteredAt > 0 && entry && exitSeen && objs == 0 && maxScroll >= WATERWAY_LEN - 7 && leftAt > enteredAt + 10;
    printf("%s  waterway: entered at %.0f s, splash-in %d, reached the source %d, river life %d, scrolled %.0f of %.0f, left at %.0f s\n", ok ? "ok  " : "FAIL", enteredAt, (int)entry, (int)exitSeen, objs, maxScroll, (double)WATERWAY_LEN, leftAt);
    if (!ok) bad++;
  }
  {   // after the waterway: the geyser (4.9 s), then the fall over the pyramid (3.3 s)
    Game G; gameInit(G, 0); setScene(G, SC_WATERWAY, 0);
    static float eq[30]; AudioIn A; A.sounding = true; A.T = 0.5f; A.eqH = eq; A.level = 0.7f; A.rms = 0.1f;
    double t = 0; const float dt = 1.0f / 30; double fountainAt = 0, skyAt = 0, leftAt = 0;
    for (int f = 0; f < 30 * 60; f++, t += dt) { A.beatNow = (f % 15) == 0; gameUpdate(G, A, dt, t);
      if (G.scene == SC_FOUNTAIN && !fountainAt) fountainAt = t; if (G.scene == SC_SKYFALL && !skyAt) skyAt = t; if (G.scene != SC_WATERWAY && G.scene != SC_FOUNTAIN && G.scene != SC_SKYFALL) { leftAt = t; break; } }
    const bool ok = fountainAt > 0 && skyAt > 0 && fabs((skyAt - fountainAt) - 4.9) < 0.2 && (leftAt - skyAt) > 3.3 && (leftAt - skyAt) < 4.3;   // the black fade out of it (until the pyramid is ported) comes after 3.3 s
    printf("%s  geyser and skyfall: fountain at %.1f s, sky at %.1f s (%.1f s later), left at %.1f s (%.1f s later)\n", ok ? "ok  " : "FAIL", fountainAt, skyAt, skyAt - fountainAt, leftAt, leftAt - skyAt);
    if (!ok) bad++;
  }
  {   // the treasury: from the bottom floor up through ladders, gaps and ropes to the pit on the top floor; gold on the way
    Game G; gameInit(G, 0); setScene(G, SC_CAVERN, 0);
    static float eq[30]; AudioIn A; A.sounding = true; A.T = 0.5f; A.eqH = eq; A.level = 0.7f; A.rms = 0.1f;
    double t = 0; const float dt = 1.0f / 30; int top = 9, bars = 0, chaseF = 0, jabF = 0, duckF = 0, stuckF = 0; double leftAt = 0;
    for (int f = 0; f < 30 * 200; f++, t += dt) { A.beatNow = (f % 15) == 0; gameUpdate(G, A, dt, t); if (G.scene != SC_CAVERN) { leftAt = t; break; } if (G.cn.lvl < top) top = G.cn.lvl; bars = G.cn.bars; if (G.cn.chase.live) { chaseF++; if (G.cn.chase.attack > 0 && G.cn.chase.attack < 0.28f) jabF++; if (G.cn.chase.stuck) stuckF++; } if (G.cn.duck) duckF++; }
    const bool ok = top == 0 && bars >= 15 && leftAt > 60 && chaseF > 0 && jabF > 0 && duckF > 0;
    printf("%s  treasury: reached floor %d, gold bars %d, left at %.0f s; guardians behind him %d frames (spear jabs %d, ducking %d, stopped at a rope %d)\n", ok ? "ok  " : "FAIL", top, bars, leftAt, chaseF, jabF, duckF, stuckF);
    if (!ok) bad++;
  }
  if (bad) return 1;
  printf("game rules passed\n");
  return 0;
}
