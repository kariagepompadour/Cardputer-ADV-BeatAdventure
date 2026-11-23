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
  {   // the treasury: from the bottom floor up through ladders, gaps and ropes to the pit on the top floor; gold on the way
    Game G; gameInit(G, 0); setScene(G, SC_CAVERN, 0);
    static float eq[30]; AudioIn A; A.sounding = true; A.T = 0.5f; A.eqH = eq; A.level = 0.7f; A.rms = 0.1f;
    double t = 0; const float dt = 1.0f / 30; int top = 9, bars = 0; double leftAt = 0;
    for (int f = 0; f < 30 * 200; f++, t += dt) { A.beatNow = (f % 15) == 0; gameUpdate(G, A, dt, t); if (G.scene != SC_CAVERN) { leftAt = t; break; } if (G.cn.lvl < top) top = G.cn.lvl; bars = G.cn.bars; }
    const bool ok = top == 0 && bars >= 15 && leftAt > 60;
    printf("%s  treasury: reached floor %d, gold bars %d, left at %.0f s\n", ok ? "ok  " : "FAIL", top, bars, leftAt);
    if (!ok) bad++;
  }
  if (bad) return 1;
  printf("game rules passed\n");
  return 0;
}
