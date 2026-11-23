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
  if (bad) return 1;
  printf("game rules passed\n");
  return 0;
}
