// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
/*
  ba_game.h — BeatAdventure: the game (a port of the web demo's game.js, stage by stage).

  The web demo is the reference. Here: the state the stages share (the quiet detector, the sneak, the stride, the fade between stages,
  the dark overlay), and the stages themselves. Stage 1 is the liner (the web demo's "climb" scene): the explorer climbs the left
  ladder to the bridge roof, looks out to sea, crosses the roof, climbs down the right ladder, and walks down the gangway to the
  shore; a ladder rung on every beat, and he clings and waits when the music goes quiet.

  The game reads the sound through AudioIn, which beatadv.h fills every frame; nothing here touches the sound itself.
*/
#pragma once
#include "ba_gfx.h"
#include "ba_hero.h"

namespace beatadv {

// What the game needs to know about the sound (the web demo's M).
struct AudioIn {
  bool beatNow = false, sounding = false;
  float T = 0.5f, level = 0, pulse = 0;
  const float* eqH = nullptr;   // the 30 equalizer bars, 0..8
};

constexpr float V = 90.0f;   // scroll px/s
constexpr int PITCH = 8, SEG = 4, SEG_H = 3;   // the equalizer blocks: 6 px wide (8 px pitch), 3 px tall (4 px pitch)

static inline float tsin(double x) { return sinf((float)fmod(x, 6.283185307179586)); }   // sin of a time-based phase without losing float precision

// ---- Stage 1: the liner.
constexpr int UPX = 318, UPW = 324, DNX = 436, DNW = 446, CEND = 600, UP_TOP = 88;   // the last rung is at 88: the top of the cliff (100) is reached by pulling up over the edge
static float T_deck(float w) { return w < 324 ? 40.0f : w < 440 ? 100.0f : w < 460 ? 40.0f : w < 540 ? 40.0f - (w - 460) * 0.5f : 0.0f; }   // lower deck, bridge roof, lower deck and gangway to shore

enum ClPh : uint8_t { CL_WALK, CL_UP, CL_DOWN, CL_MOUNT, CL_LOOKWALK, CL_LOOK, CL_CROUCH, CL_LOWER, CL_TURN };
struct Liner { float w = 0, z = 40, zT = 40, stepT = 0, mt = 0, q = 0; ClPh ph = CL_WALK; uint8_t stage = 0; };

enum Scene : uint8_t { SC_LINER };

struct Game {
  Scene scene = SC_LINER, next = SC_LINER;
  bool isQuiet = false;
  float sneak = 0, sp = 1, fade = 0, visualDark = -1, sl = 0, dt = 0.033f;
  int fadeDir = 0, day = 1;
  double t0 = 0;
  Liner cl;
};

static void setScene(Game& G, Scene s, double now) {
  G.scene = s; G.t0 = now;
  if (s == SC_LINER) G.cl = Liner();
}
static void switchTo(Game& G, Scene s) { if (G.fadeDir) return; G.next = s; G.fadeDir = 1; G.fade = 0; }
static void gameInit(Game& G, double now) { G = Game(); HC = HeroCtx(); setScene(G, SC_LINER, now); }

static void updateLiner(Game& G, const AudioIn& A, float dt, double now) {
  Liner& c = G.cl;
  c.stepT += dt;
  bool stepNow = (A.beatNow && !G.isQuiet) || (!A.sounding && c.stepT >= 0.7f);
  if (stepNow) c.stepT = 0;
  const float T = A.T;
  switch (c.ph) {
    case CL_WALK: {
      float sl = c.w > 460 && c.w < 540 ? -1.0f : 0.0f;   // the only slope is the gangway down to shore
      c.w += V * G.sp * (sl < 0 ? 1.3f : 1.0f) * dt; c.z = T_deck(c.w);
      if (c.stage == 0 && c.w >= UPX) { c.stage = 1; c.ph = CL_UP; c.w = UPX; c.zT = c.z; }
      else if (c.stage == 1 && c.w >= DNX) { c.mt = 0; c.w = 438; c.z = 100; c.stage = 2; c.ph = CL_CROUCH; }
      else if (c.w >= CEND && !G.fadeDir) switchTo(G, SC_LINER);   // until the next stage is ported, the liner starts again
      break;
    }
    case CL_LOOKWALK: { c.mt += dt; float q = sm01(cl01(c.mt / 0.35f)); c.w = UPX + 8 + 18 * q; if (q >= 1) { c.ph = CL_LOOK; c.mt = 0; } break; }
    case CL_LOOK: { c.mt += dt; if (c.mt >= 1.6f) { c.ph = CL_WALK; c.mt = 0; } break; }
    case CL_UP: case CL_DOWN: {
      bool up = c.ph == CL_UP;
      if (stepNow) c.zT = up ? fminf(UP_TOP, c.zT + 6) : fmaxf(40, c.zT - 6);   // one rung on every beat; in a quiet part he clings and waits
      float st = 6 / (0.55f * T) * dt, d = c.zT - c.z, ad = d < 0 ? -d : d;
      c.z += (d > 0 ? 1 : d < 0 ? -1 : 0) * (ad < st ? ad : st);
      if (up && c.z >= UP_TOP - 0.1f && c.zT >= UP_TOP) { c.ph = CL_MOUNT; c.mt = 0; c.z = UP_TOP; }
      else if (!up && c.z <= 40.1f) { c.ph = CL_TURN; c.mt = 0; c.z = 40; c.stage = 3; }
      break;
    }
    default: {   // the moves at the ends of a ladder take a few beats: pull over the edge, crouch and turn at the top, lower himself onto the ladder, let go at the bottom
      c.mt += dt / T;
      if (c.ph == CL_MOUNT) { float q = cl01(c.mt / 2.4f); c.q = q; c.z = UP_TOP + 12 * sm01(q); c.w = UPX + 8 * sm01(cl01((q - 0.5f) / 0.5f)); if (q >= 1) { c.ph = CL_LOOKWALK; c.mt = 0; c.w = UPX + 8; c.z = 100; } }
      else if (c.ph == CL_CROUCH) { if (c.mt >= 0.5f) { c.ph = CL_LOWER; c.mt = 0; } }
      else if (c.ph == CL_LOWER) { float q = 1 - cl01(c.mt / 2.4f); c.q = q; c.z = UP_TOP + 12 * sm01(q); c.w = 440 + 6 - 8 * sm01(cl01((q - 0.5f) / 0.5f)); if (c.mt >= 2.4f) { c.ph = CL_DOWN; c.z = UP_TOP; c.zT = UP_TOP; c.w = DNW; } }
      else if (c.ph == CL_TURN) { if (c.mt >= 0.8f) { c.ph = CL_WALK; c.w = DNW + 3; c.z = 40; } }
      break;
    }
  }
  (void)now;
}

// One frame of the game: the quiet detector, the sneak, the stride, then the stage and the fade between stages.
static void gameUpdate(Game& G, const AudioIn& A, float dt, double now) {
  G.dt = dt;
  G.isQuiet = A.sounding && (G.isQuiet ? A.level < 0.38f : A.level < 0.27f);
  G.sneak += (((G.isQuiet || !A.sounding) ? 1.0f : 0.0f) - G.sneak) * fminf(1.0f, dt * 2);
  G.sp = 1 - 0.4f * G.sneak;
  HC.sneak = G.sneak;
  HC.ph += dt * G.sp / (A.sounding ? A.T : 0.6f);   // the stride follows the beat: the left foot lands on it
  if (A.beatNow && G.sneak < 0.3f) { float f = (HC.ph - 0.25f) - floorf(HC.ph - 0.25f), e = f < 0.5f ? -f : 1 - f; HC.ph += e * 0.35f; }
  HC.ph -= floorf(HC.ph);
  if (G.scene == SC_LINER) updateLiner(G, A, dt, now);
  if (G.fadeDir) {
    G.fade += G.fadeDir * dt * 1.8f;
    if (G.fadeDir > 0 && G.fade >= 1) { setScene(G, G.next, now); G.fadeDir = -1; }
    else if (G.fadeDir < 0 && G.fade <= 0) { G.fade = 0; G.fadeDir = 0; }
  }
}

// ---- Drawing.
static const uint16_t SURF_EQ[6] = {rgb(255, 255, 255), rgb(241, 252, 255), rgb(216, 246, 255), rgb(183, 232, 249), rgb(156, 219, 243), rgb(132, 206, 237)};
static void drawSurfEQ(const AudioIn& A, int base, int maxSeg) {   // the 30 live bands as foam: white at the foot, pale blue at the tip
  for (int j = 0; j < 30; j++) {
    int n = rnd(A.eqH[j]); if (n > maxSeg) n = maxSeg;
    int x = j * PITCH + 1;
    for (int k = 0; k < n; k++) R(x, base - SEG * (k + 1) + 1, 6, SEG_H, SURF_EQ[k < 5 ? k : 5]);
  }
}

static void drawLiner(Game& G, const AudioIn& A, double now) {   // the opening: lower deck, left ladder, bridge roof, right ladder, gangway to shore
  const Liner& c = G.cl;
  const float w = c.w, z = c.z;
  auto ix = [&](float wr) { return rnd(HX + wr - w); };
  auto iy = [&](float h) { return rnd(GY - (h - z)); };
  const uint16_t WHITE = rgb(245, 246, 238), SHADE = rgb(194, 213, 225), GLASS = rgb(80, 198, 225), SEA = rgb(7, 39, 112), BLUE = rgb(21, 75, 151), RAIL = rgb(75, 91, 105), WOOD = rgb(139, 94, 54);
  const uint16_t SAND = rgb(190, 149, 101), SANDL = rgb(241, 211, 161), RUNG = rgb(207, 157, 99);
  Ri(0, 0, W, H, rgb(105, 183, 231)); Ri(0, 0, W, 38, rgb(55, 136, 208));
  for (int i = 0; i < 3; i++) { int x = ((i * 91 - rnd((float)fmod(now * 2, 300.0))) % 300 + 300) % 300 - 25; Ri(x, 18 + i * 9, 22, 4, WHITE); Ri(x + 5, 16 + i * 9, 12, 2, WHITE); }   // clouds
  for (int i = 0; i < 4; i++) {   // gulls
    int x = ((i * 67 + rnd((float)fmod(now * (8 + i * 2), 290.0))) % 290 + 290) % 290 - 24;
    int y = 34 + (i % 3) * 12 + rnd(tsin(now * 2.2 + i) * 2), wing = rnd(tsin(now * 8 + i * 2) * 3);
    seg(x - 7, y - 3 + wing, x, y, WHITE, 1); seg(x, y, x + 7, y - 3 + wing, WHITE, 1); Ri(x - 1, y, 3, 1, SHADE);
  }
  Ri(0, 77, W, H - 77, SEA); Ri(0, 77, W, 3, rgb(44, 120, 184));
  for (int x = 0; x < W; x += 4) {   // the hull and the gangway's end, column by column
    float wr = w + x - HX; int sy = iy(T_deck(wr));
    if (wr < 460) { Ri(x, sy + 3, 4, H - sy - 3, WHITE); Ri(x, sy, 4, 3, SHADE); }
    else if (wr >= 540) { Ri(x, sy, 4, H - sy, SAND); Ri(x, sy, 4, 3, SANDL); }
  }
  for (int wx = -40; wx < 460; wx += 18) { int x = ix((float)wx), y = iy(24); if (x < -10 || x > W + 10) continue; Ri(x, y, 8, 7, RAIL); Ri(x + 1, y + 1, 6, 5, GLASS); }   // portholes
  for (int wx = 333; wx < 435; wx += 17) { int x = ix((float)wx), y = iy(78); if (x < -10 || x > W + 10) continue; Ri(x, y, 10, 9, RAIL); Ri(x + 1, y + 1, 8, 7, GLASS); Ri(x + 2, y + 2, 3, 1, rgb(199, 245, 250)); }   // the bridge windows
  Ri(ix(324), iy(103), ix(440) - ix(324), 3, WHITE); Ri(ix(324), iy(100), ix(440) - ix(324), 2, RAIL);
  Ri(ix(351), iy(110), 12, 10, SHADE); Ri(ix(351), iy(110), 12, 2, WHITE);   // bridge-roof fitting for the raised boot
  Ri(ix(375), iy(119), 2, 16, SHADE); Ri(ix(369), iy(120), 16, 3, WHITE); Ri(ix(372), iy(124), 10, 2, RAIL);   // the mast
  for (int wx = 346; wx < 432; wx += 18) Ri(ix((float)wx), iy(101), 1, 5, RAIL);
  auto rail = [&](int wx0, int wx1, int h) { Ri(ix((float)wx0), iy((float)h + 4), ix((float)wx1) - ix((float)wx0), 1, RAIL); for (int wx = wx0; wx <= wx1; wx += 15) Ri(ix((float)wx), iy((float)h + 4), 1, 5, RAIL); };
  rail(-40, 318, 40); rail(442, 460, 40);
  int shoreX = ix(540); shoreX = shoreX < 0 ? 0 : shoreX > W ? W : shoreX;
  Ri(0, H - 18, shoreX, 18, SEA); Ri(shoreX, H - 18, W - shoreX, 18, SAND);
  Ri(0, H - 18, shoreX, 2, BLUE); drawSurfEQ(A, H - 2, 6);
  if (shoreX < W) Ri(shoreX, H - 18, W - shoreX, 3, SANDL);
  seg((float)ix(460), (float)iy(40), (float)ix(540), (float)iy(0), WOOD, 5); seg((float)ix(460), (float)iy(48), (float)ix(540), (float)iy(8), RAIL, 1);   // the gangway
  for (int wx = 460; wx <= 540; wx += 12) { float h = T_deck((float)wx); seg((float)ix((float)wx), (float)iy(h + 8), (float)ix((float)wx), (float)iy(h), RAIL, 1); }
  auto rung = [&](int lxw, int h0, int h1) { int X = ix((float)lxw); Ri(X - 1, iy((float)h1), 1, iy((float)h0) - iy((float)h1), WOOD); Ri(X + 3, iy((float)h1), 1, iy((float)h0) - iy((float)h1), WOOD); for (int h = h0; h <= h1; h += 6) Ri(X - 1, iy((float)h), 5, 1, RUNG); };
  rung(UPX + 5, 40, 104); rung(DNW - 5, 40, 104);
  HC.wob = 0;
  switch (c.ph) {
    case CL_MOUNT: drawLedgeHero(c.q, z, 1); break;
    case CL_LOOK: drawLookoutHero(HX, GY); break;
    case CL_LOOKWALK: drawHero(HX, GY, now, HM_RUN); break;
    case CL_LOWER: drawLedgeHero(c.q, z, -1); break;
    case CL_CROUCH: case CL_TURN: mirrorOn(HX); drawHero(HX, GY, now, HM_IDLE); mirrorOff(); break;   // at the edge he turns to face the cliff (back to the drop) and goes straight over it
    case CL_WALK: {
      float tg = c.w > 460 && c.w < 540 ? -1.0f : 0.0f;
      G.sl += (tg - G.sl) * (1.0f - powf(0.88f, 60.0f * G.dt));
      HC.slope = 0.5f * G.sl; HC.wob = G.sl > 0 ? 4 * G.sl : 12 * G.sl;
      drawHero(HX, GY, now, HM_RUN); HC.wob = 0; HC.slope = 0;
      break;
    }
    default: drawLadderHero(HX, z, c.ph == CL_DOWN ? -1 : 1); break;
  }
}

// One frame of the picture: the stage, the dark of a quiet part, the fade, and the level dots.
static void gameRender(Game& G, const AudioIn& A, uint16_t* fb, double now) {
  FB = fb; gMirror = false;
  drawLiner(G, A, now);
  float dark = (A.sounding ? fmaxf(0.0f, 0.5f - A.level * 0.9f) : 0.55f) * 0.45f;   // the quieter it gets, the darker (less so at sea)
  G.visualDark = G.visualDark < 0 ? dark : G.visualDark + (dark - G.visualDark) * (1.0f - powf(1.0f - 0.018f, 60.0f * G.dt));
  if (G.visualDark > 0) fillA(0, 0, W, H, 0, 0, 12, G.visualDark);
  if (G.fade > 0) fillA(0, 0, W, H, 0, 0, 0, G.fade < 1 ? G.fade : 1);
  for (int i = 0; i < 6; i++) Ri(W - 10 - i * 5, 125, 3, 3, (A.pulse > 0.5f && i == 0) ? rgb(255, 210, 60) : (A.level * 6 > i ? rgb(40, 170, 50) : rgb(40, 50, 40)));
}

// The name of the stage for the status line.
static const char* gameStageName(const Game& G) { (void)G; return "LINER"; }

}  // namespace beatadv
