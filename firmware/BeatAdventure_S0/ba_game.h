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
  long beatN = 0;
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

// ---- Stage 2: the beach (the web demo's "jungle" with land = "open"): sand road, the liner leaving, driftwood logs and sea turtles to jump.
constexpr float JD = 0.62f, JH = 28.0f;   // the jump's time and height
enum ObKind : uint8_t { OB_LOG, OB_TURTLE, OB_NUT };
struct Obs { double arr = 0, born = 0; float wx = 0, Tf = 0.7f, xw0 = 0, y0 = 0; ObKind kind = OB_LOG; bool done = false, live = false; };
struct Tree { float wx = 0; int throws = 0; double throwAt = -9; bool live = false; };   // a palm with a monkey on it
struct Beach {
  float scroll = 0, jump = -1, jd = JD, jh = JH;
  Obs obs[6];
  Tree tree;
  double lastArr = 0, nextGap = 0.78, shipStart = 0, nextTree = 0;
  uint32_t rng = 2463534242u;
};

enum Scene : uint8_t { SC_LINER, SC_BEACH };

struct Game {
  Scene scene = SC_LINER, next = SC_LINER;
  bool isQuiet = false;
  float sneak = 0, sp = 1, fade = 0, visualDark = -1, sl = 0, dt = 0.033f;
  int fadeDir = 0, day = 1;
  double t0 = 0;
  Liner cl;
  Beach bc;
};

static void setScene(Game& G, Scene s, double now) {
  G.scene = s; G.t0 = now;
  if (s == SC_LINER) G.cl = Liner();
  if (s == SC_BEACH) { uint32_t r = G.bc.rng; G.bc = Beach(); G.bc.rng = r; G.bc.lastArr = now; G.bc.shipStart = now; G.bc.nextTree = now + 3; G.sl = 0; }
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
      else if (c.w >= CEND && !G.fadeDir) switchTo(G, SC_BEACH);   // the gangway ends on the beach
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

static inline float rand01(Game& G) { uint32_t x = G.bc.rng; x ^= x << 13; x ^= x >> 17; x ^= x << 5; G.bc.rng = x; return (x >> 8) * (1.0f / 16777216.0f); }
static inline float obsX(const Beach& b, const Obs& o) { return HX + o.wx - b.scroll; }        // an obstacle stands at a place in the world and the ground carries it past
static inline float obsTtl(const Beach& b, const Obs& o) { return (o.wx - b.scroll) / V; }     // seconds until it reaches the hero at full speed

static void spawnBeach(Game& G, const AudioIn& A, double now) {   // called on a beat: put an obstacle that reaches the hero k beats from now
  Beach& b = G.bc;
  if (G.fadeDir || G.isQuiet || A.level < 0.06f) return;   // very quiet: nothing comes, the beach only waits (suspense)
  const float T = A.T, need = (W + 18 - HX + 64) / V;
  int k = (int)ceilf(need / T); if (k < 2) k = 2;
  const double arr = now + k * T;
  if (arr - b.lastArr < b.nextGap || ((A.beatN & 1) && A.level < 0.55f)) return;   // room between obstacles
  b.lastArr = arr; b.nextGap = 3.5 + rand01(G) * 5.5;   // rare enough to matter: every 3.5 to 9 s
  for (Obs& o : b.obs) if (!o.live) {
    o.live = true; o.done = false; o.arr = arr; o.wx = b.scroll + (float)(arr - now) * V;
    o.kind = rand01(G) < 0.25f ? OB_LOG : OB_TURTLE;
    break;
  }
}

static inline float monkeyY(const Tree& t, double now) { return (float)rnd(42 + 16 * sinf((float)(now * 0.9 + t.wx * 0.013))); }   // he shuttles up and down the trunk

static void updateBeach(Game& G, const AudioIn& A, float dt, double now) {
  Beach& b = G.bc;
  if (A.beatNow) spawnBeach(G, A, now);
  // a palm with a monkey now and then; he throws a coconut ahead of the hero, and it lands and lies there like a log
  Tree& t = b.tree;
  if (!t.live && A.sounding && !G.fadeDir && now > b.nextTree) { t = Tree(); t.live = true; t.wx = b.scroll + W - HX + 24; b.nextTree = now + 7 + rand01(G) * 5; }
  if (t.live) {
    float sx = HX + t.wx - b.scroll;
    if (A.beatNow && t.throws < 2 && sx < 214 && sx > 100 && !G.isQuiet && G.sp > 0.7f && now - t.throwAt > 1.2) {
      const float Tf = 0.7f, wx = b.scroll + V * Tf + 100;
      bool room = true; for (const Obs& o : b.obs) if (o.live && fabsf(o.wx - wx) <= 150) room = false;
      if (room) for (Obs& o : b.obs) if (!o.live) {
        o.live = true; o.done = false; o.kind = OB_NUT; o.wx = wx; o.arr = now + (wx - b.scroll) / V; o.born = now; o.Tf = Tf; o.xw0 = t.wx - 8; o.y0 = monkeyY(t, now) - 4;
        if (o.arr > b.lastArr) b.lastArr = o.arr; t.throws++; t.throwAt = now; break;
      }
    }
    if (sx < -30) t.live = false;
  }
  b.scroll += V * G.sp * dt;
  for (Obs& o : b.obs) if (o.live && obsX(b, o) < -40) o.live = false;
  if (b.jump >= 0) { b.jump += dt; if (b.jump > b.jd) b.jump = -1; }
  if (b.jump < 0) {   // hop what comes: he leaves the ground half a jump before it arrives, so he lands on the beat
    for (Obs& o : b.obs) if (o.live && !o.done) { float t = obsTtl(b, o); if (t < JD / 2 && t > -0.1f) { b.jd = JD; b.jh = JH; b.jump = fmaxf(0, b.jd / 2 - t); o.done = true; break; } }
  }
  if (!G.fadeDir && now - b.shipStart >= 48) switchTo(G, SC_LINER);   // the liner has gone: until the forest is ported, the story starts again
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
  else if (G.scene == SC_BEACH) updateBeach(G, A, dt, now);
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

static inline double dmod(double v, double m) { return fmod(fmod(v, m) + m, m); }   // a modulo that stays positive (JavaScript's ((v % m) + m) % m)

static void PA(float x, float y, int w, int h, int r, int g, int b, float a) { float keep = gAlpha; gAlpha = a * keep; R(x, y, w, h, rgb(r, g, b)); gAlpha = keep; }

static void drawDepartingShip(const Beach& bc, double now, int horizon) {   // a large passenger ship leaves the hero on the beach, then slowly shrinks toward the horizon
  const float elapsed = fmaxf(0, (float)(now - bc.shipStart)), away = cl01(elapsed / 48);
  if (away >= 1) return;
  const int x = rnd(92 + 105 * sm01(cl01(elapsed / 26)) + 28 * sm01(cl01((elapsed - 26) / 22)));
  const float s = 1 - 0.94f * away;
  gAlpha = 1 - cl01((away - 0.82f) / 0.18f);
  auto P = [&](float dx, float dy, float w, float h, uint16_t col) { int pw = rnd(w * s), ph = rnd(h * s); R(x + dx * s, horizon + dy * s, pw < 1 ? 1 : pw, ph < 1 ? 1 : ph, col); };
  auto Pa = [&](float dx, float dy, float w, float h, int r, int g, int b, float a) { float keep = gAlpha; gAlpha = a * keep; P(dx, dy, w, h, rgb(r, g, b)); gAlpha = keep; };
  Pa(-47, 3, 12, 1, 255, 244, 220, 0.8f); Pa(-51, 5, 10, 1, 255, 244, 220, 0.55f);   // wake trailing behind the stern
  P(-34, -5, 68, 7, rgb(45, 53, 82)); P(-30, 1, 58, 2, rgb(152, 51, 48)); P(30, -4, 4, 4, rgb(45, 53, 82));   // deep hull, red waterline and rising bow
  P(-29, -16, 53, 11, rgb(248, 243, 223)); P(-23, -22, 36, 6, rgb(255, 250, 234)); P(9, -20, 15, 4, rgb(255, 250, 234));   // two passenger decks and bridge
  P(-13, -28, 9, 6, rgb(210, 60, 40)); P(-12, -29, 7, 2, rgb(75, 58, 56)); P(3, -26, 7, 4, rgb(210, 60, 40));   // red funnels
  for (int i = 0; i < 10; i++) P(-25 + i * 5, -12, 3, 3, rgb(39, 74, 100));
  for (int i = 0; i < 6; i++) P(-18 + i * 5, -20, 3, 2, rgb(39, 74, 100));
  for (int i = 0; i < 9; i++) P(-26 + i * 6, -2, 2, 1, rgb(255, 215, 125));   // cabin windows and lit portholes
  gAlpha = 1;
}

static void drawOpenSky(const Beach& bc, const AudioIn& A, double now) {   // the beach: a sunset sky, departing liner, drifting wave crests, and far palms on the sand
  const float sc = bc.scroll;
  static const uint16_t bands[6] = {rgb(30, 94, 174), rgb(52, 128, 205), rgb(78, 161, 222), rgb(112, 191, 235), rgb(153, 211, 242), rgb(192, 228, 247)};
  for (int i = 0; i < 6; i++) Ri(0, rnd((float)i * GY / 6), W, (int)ceilf(GY / 6.0f) + 1, bands[i]);
  const int sr = 9 + rnd(A.pulse * 2);
  fillA(150 - sr - 3, 56 - sr - 3, (sr + 3) * 2, (sr + 3) * 2, 255, 240, 170, 0.35f);   // the sun's glow
  Ri(150 - sr + 2, 56 - sr + 2, (sr - 2) * 2, (sr - 2) * 2, rgb(255, 236, 160)); Ri(150 - sr + 4, 56 - sr, (sr - 4) * 2, sr * 2, rgb(255, 236, 160));
  for (int i = 0; i < 5; i++) {   // clouds
    int cx = rnd((float)(dmod(i * 71 - sc * 0.06 - now * 3, 330) - 40)), cy = 14 + (i * 23) % 40;
    Ri(cx, cy, 26, 5, rgb(255, 255, 255)); Ri(cx + 5, cy - 3, 14, 3, rgb(255, 255, 255)); Ri(cx + 3, cy + 5, 20, 2, rgb(183, 215, 231));
  }
  const int SY = GY - 36;   // the horizon
  static const uint16_t seaB[4] = {rgb(100, 178, 220), rgb(62, 143, 204), rgb(36, 107, 180), rgb(18, 75, 145)};
  for (int i = 0; i < 4; i++) Ri(0, SY + i * 7, W, 8, seaB[i]);   // the sea, deeper toward the beach
  for (int r = 0; r < 4; r++) for (int k = -1; k < 12; k++) {   // wave crests drift along, bigger in loud music
    int x = rnd((float)(dmod(k * 31 + r * 17 - sc * (0.05 + r * 0.03) - now * (4 + r * 2), 372) - 30));
    int y = SY + 3 + r * 7 + rnd(tsin(now * 2 + k + r) * (1 + 2 * A.level));
    Ri(x, y, 7 + r * 2, 1, rgb(212, 241, 255));
  }
  if (A.pulse > 0.4f) for (int i = 0; i < 5; i++) Ri(120 + i * 17, SY + 2 + (i * 5) % 9, 1, 1, WHITE565);   // the sea glitters on the beat
  drawDepartingShip(bc, now, SY);
  Ri(0, GY - 8, W, 8, rgb(214, 170, 120)); Ri(0, GY - 8, W, 1, rgb(240, 200, 160));
  for (int k = -1; k < 14; k++) {   // wet sand and the foam at the water's edge
    int x = rnd((float)(dmod(k * 29 - sc * 0.3, 400) - 20)); float f = (tsin(now * 1.6 + k) + 1) * 1.2f;
    PA((float)x, (float)(GY - 9 - rnd(f)), 12, 1, 255, 226, 200, 0.8f);
  }
  drawSurfEQ(A, GY - 7, 6);   // white-to-aqua sound bars rise from the shoreline like spray
  for (int i = -1; i < 5; i++) {   // far palms on the sand, leaning a little, swaying with the bass
    int x = rnd((float)(dmod(i * 83 - sc * 0.3, 330) - 40)), sw = rnd(tsin(now * 1.3 + i) * 1 + (A.eqH[0] + A.eqH[1] + A.eqH[2]) / 3 * 0.35f);
    Ri(x + 8, GY - 30, 3, 28, rgb(70, 50, 40)); Ri(x + 9, GY - 22, 1, 1, rgb(90, 62, 36)); Ri(x + 9, GY - 14, 1, 1, rgb(90, 62, 36));
    for (int d = -1; d <= 1; d += 2) { Ri(x + 9 + d * 4 + sw, GY - 33, 6, 2, rgb(20, 86, 58)); Ri(x + 9 + d * 9 + sw, GY - 31, 5, 2, rgb(40, 120, 70)); Ri(x + 9 + d * 12 + sw, GY - 29, 3, 2, rgb(20, 86, 58)); }
    Ri(x + 5 + sw, GY - 36, 9, 3, rgb(40, 120, 70)); Ri(x + 8, GY - 29, 3, 3, rgb(110, 70, 30));   // coconuts under the crown
  }
}

static void drawLog(float x, int y) {   // a fallen tree trunk lying along the path, seen from the side: rounded bark with streaks, a moss patch, a knot, and the sawn end toward him with its growth rings
  const uint16_t bark = rgb(130, 80, 30), dk = rgb(60, 36, 18), tan = rgb(218, 176, 116), ring = rgb(150, 100, 56), moss = rgb(20, 130, 40);
  R(x - 8, (float)y - 10, 17, 10, bark); R(x - 9, (float)y - 9, 19, 8, bark);   // a rounded body
  R(x - 7, (float)y - 10, 15, 1, rgb(140, 92, 48)); R(x - 6, (float)y - 7, 12, 1, dk); R(x - 4, (float)y - 4, 13, 1, dk); R(x - 8, (float)y - 2, 11, 1, dk); R(x - 1, (float)y - 9, 1, 3, dk);   // bark streaks
  R(x + 2, (float)y - 6, 3, 3, dk); R(x + 3, (float)y - 5, 1, 1, bark); R(x + 3, (float)y - 11, 5, 2, moss);   // a knot and moss
  R(x - 12, (float)y - 9, 4, 8, tan); R(x - 13, (float)y - 8, 1, 6, tan); R(x - 11, (float)y - 7, 2, 4, ring); R(x - 10, (float)y - 6, 1, 2, tan);   // the sawn end, with its rings
}
static void drawBeachTurtle(float x, double now) {   // a small sea turtle crawling across the sand, head toward the approaching hero
  const uint16_t shell = rgb(68, 102, 55), rim = rgb(42, 73, 47), plate = rgb(132, 150, 75), skin = rgb(87, 128, 77);
  const int step = (int)((long)floor(now * 5 + x * 0.03) & 1);
  PA(x - 13, GY - 1, 27, 1, 94, 68, 47, 0.35f);
  R(x + 7, (float)(GY - 3 + step), 6, 3, skin); R(x - 8, (float)(GY - 3 + (1 - step)), 6, 3, skin); R(x + 11, GY - 5, 4, 2, skin);
  R(x - 14, GY - 7, 7, 5, skin); R(x - 15, GY - 5, 3, 2, skin); R(x - 11, GY - 6, 1, 1, rgb(20, 30, 24));
  R(x - 8, GY - 7, 18, 6, rim); R(x - 6, GY - 9, 14, 7, shell); R(x - 3, GY - 11, 8, 3, shell);
  R(x - 4, GY - 8, 6, 3, plate); R(x + 4, GY - 7, 3, 2, plate); R(x - 7, GY - 5, 2, 2, plate);
}

static void drawTree(const Tree& t, const Beach& b, const AudioIn& A, double now) {   // a palm with a monkey on it: he climbs by hands and feet in time with the beat, and throws a coconut
  const int sx = rnd(HX + t.wx - b.scroll); if (sx < -20 || sx > W + 24) return;
  const int base = GY, my = (int)monkeyY(t, now), ph = (int)floor(now / (A.T / 2)) & 1; const bool th = now - t.throwAt < 0.35;
  const uint16_t TK = rgb(150, 100, 52), TKD = rgb(104, 66, 30), LF = rgb(40, 170, 60), LF2 = rgb(90, 210, 80), CT = rgb(80, 50, 24);
  const uint16_t MK = rgb(120, 70, 30), MKD = rgb(78, 44, 20), MKF = rgb(226, 176, 128);
  Ri(sx - 3, 14, 7, base - 14, TK); Ri(sx + 2, 14, 2, base - 14, TKD); for (int y = 22; y < base; y += 5) Ri(sx - 3, y, 7, 1, TKD);
  Ri(sx - 20, 15, 12, 3, LF); Ri(sx + 9, 15, 12, 3, LF); Ri(sx - 12, 11, 24, 3, LF2); Ri(sx - 7, 7, 14, 3, LF); Ri(sx - 24, 18, 6, 3, LF); Ri(sx + 19, 18, 6, 3, LF); Ri(sx - 26, 21, 3, 3, LF); Ri(sx + 24, 21, 3, 3, LF);
  Ri(sx - 3, 15, 3, 3, CT); Ri(sx + 1, 15, 3, 3, CT);
  seg((float)(sx - 6), (float)(my + 5), (float)(sx - 10), (float)(my + 8), MKD, 1); seg((float)(sx - 10), (float)(my + 8), (float)(sx - 8), (float)(my + 11), MKD, 1);
  Ri(sx - 6, my, 4, 6, MK); Ri(sx - 7, my - 4, 4, 4, MK); Ri(sx - 7, my - 3, 2, 2, MKF); Ri(sx - 8, my - 5, 2, 2, MK);
  Ri(sx - 3, my + (ph ? 0 : 3), 2, 1, MKD); Ri(sx - 3, my + (ph ? 4 : 1), 2, 1, MKD);
  if (th) { Ri(sx - 9, my - 7, 2, 4, MK); Ri(sx - 10, my - 9, 3, 3, CT); }
}
static void drawNut(const Obs& o, float x, const Beach& b, double now) {
  const uint16_t CT = rgb(80, 50, 24), D2 = rgb(70, 44, 20);
  const float q = cl01((float)((now - o.born) / o.Tf));
  if (q < 1) { float xw = lp(o.xw0, o.wx, q), y = lp(o.y0, GY - 3, q) - 10 * 4 * q * (1 - q); int sx = rnd(HX + xw - b.scroll), sp = (int)floor(now * 18) & 1; Ri(sx - 2, rnd(y) - 2, 5, 5, CT); Ri(sx - 2 + sp * 2, rnd(y) - 2, 2, 2, D2); }
  else { R(x - 3, GY - 6, 6, 6, CT); R(x - 2, GY - 5, 2, 2, D2); R(x - 1, GY - 7, 2, 1, CT); }
}

static void drawBeach(Game& G, const AudioIn& A, double now) {
  const Beach& b = G.bc;
  const float sc = b.scroll;
  drawOpenSky(b, A, now);
  Ri(0, GY, W, 4, rgb(226, 184, 130)); Ri(0, GY + 4, W, H - GY - 4, rgb(190, 146, 100)); Ri(0, GY + 14, W, 12, rgb(162, 120, 82));   // on the beach the road is sand
  for (int i = -1; i < 28; i++) {   // grass tufts, shells and darker patches, each fixed to the world, so that the ground is seen to move
    int k = (int)floorf((sc - HX) / 9) + i; float x = HX + k * 9 - sc;
    uint32_t hh = (uint32_t)k * 2654435761u; int a = (hh >> 8) % 5, bb = (hh >> 16) % 7;
    if (a < 2) { uint16_t gc = rgb(150, 160, 70); R(x, GY - 2, 1, 2, gc); R(x + 2, GY - 3, 1, 3, gc); R(x + 4, GY - 2, 1, 2, gc); }
    if (bb == 0) { R(x, GY + 7, 3, 2, rgb(250, 240, 230)); R(x + 1, GY + 7, 1, 1, rgb(240, 170, 160)); } else if (bb == 3) R(x + 1, GY + 11, 3, 2, rgb(170, 140, 90)); else if (bb == 5) R(x, GY + 5, 2, 1, rgb(190, 160, 108));
  }
  if (b.tree.live) drawTree(b.tree, b, A, now);
  for (const Obs& o : b.obs) if (o.live) { float x = obsX(b, o); if (x > W + 20) continue; if (o.kind == OB_LOG) drawLog(x, GY); else if (o.kind == OB_NUT) drawNut(o, x, b, now); else drawBeachTurtle(x, now); }
  float y = GY; HeroMode mode = HM_RUN;
  if (b.jump >= 0) { float u = b.jump / b.jd; y = GY - 4 * b.jh * u * (1 - u); mode = HM_AIR; AIR_U = u; }
  HC.slope = 0; HC.wob = 0;
  drawHero(HX, (float)rnd(y), now, mode);
}

// One frame of the picture: the stage, the dark of a quiet part, the fade, and the level dots.
static void gameRender(Game& G, const AudioIn& A, uint16_t* fb, double now) {
  FB = fb; gMirror = false;
  if (G.scene == SC_LINER) drawLiner(G, A, now); else drawBeach(G, A, now);
  float dark = (A.sounding ? fmaxf(0.0f, 0.5f - A.level * 0.9f) : 0.55f) * 0.45f;   // the quieter it gets, the darker (less so at sea)
  G.visualDark = G.visualDark < 0 ? dark : G.visualDark + (dark - G.visualDark) * (1.0f - powf(1.0f - 0.018f, 60.0f * G.dt));
  if (G.visualDark > 0) fillA(0, 0, W, H, 0, 0, 12, G.visualDark);
  if (G.fade > 0) fillA(0, 0, W, H, 0, 0, 0, G.fade < 1 ? G.fade : 1);
  for (int i = 0; i < 6; i++) Ri(W - 10 - i * 5, 125, 3, 3, (A.pulse > 0.5f && i == 0) ? rgb(255, 210, 60) : (A.level * 6 > i ? rgb(40, 170, 50) : rgb(40, 50, 40)));
}

// The name of the stage for the status line.
static const char* gameStageName(const Game& G) { return G.scene == SC_BEACH ? "BEACH" : "LINER"; }

}  // namespace beatadv
