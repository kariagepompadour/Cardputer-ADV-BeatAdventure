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
#include <string.h>
#include <new>
#include "ba_gfx.h"
#include "ba_hero.h"

namespace beatadv {

// What the game needs to know about the sound (the web demo's M).
struct AudioIn {
  bool beatNow = false, sounding = false;
  float T = 0.5f, level = 0, pulse = 0, rms = 0;
  long beatN = 0;
  const float* eqH = nullptr;   // the 30 equalizer bars, 0..8
};

constexpr float V = 90.0f;   // scroll px/s
constexpr int PITCH = 8, SEG = 4, SEG_H = 3;   // the equalizer blocks: 6 px wide (8 px pitch), 3 px tall (4 px pitch)


// ---- Stage 1: the liner.
constexpr int UPX = 318, UPW = 324, DNX = 436, DNW = 446, CEND = 600, UP_TOP = 88;   // the last rung is at 88: the top of the cliff (100) is reached by pulling up over the edge
static float T_deck(float w) { return w < 324 ? 40.0f : w < 440 ? 100.0f : w < 460 ? 40.0f : w < 540 ? 40.0f - (w - 460) * 0.5f : 0.0f; }   // lower deck, bridge roof, lower deck and gangway to shore

enum ClPh : uint8_t { CL_WALK, CL_UP, CL_DOWN, CL_MOUNT, CL_LOOKWALK, CL_LOOK, CL_CROUCH, CL_LOWER, CL_TURN };
struct Liner { float w = 0, z = 40, zT = 40, stepT = 0, mt = 0, q = 0; ClPh ph = CL_WALK; uint8_t stage = 0; };

// ---- Stage 2: the beach (the web demo's "jungle" with land = "open"): sand road, the liner leaving, driftwood logs and sea turtles to jump.
constexpr float JD = 0.62f, JH = 28.0f;   // the jump's time and height
enum ObKind : uint8_t { OB_LOG, OB_TURTLE, OB_NUT, OB_TIGER, OB_BEAR, OB_POND };
struct Obs { double arr = 0, born = 0; float wx = 0, Tf = 0.7f, xw0 = 0, y0 = 0, hw = 14; ObKind kind = OB_LOG; bool done = false, live = false, wide = false, pulse = false, hasCroc = false; };
struct Hunter { float t = 0; bool live = false, front = false, dodged = false; };   // a masked spear-carrier that appears ahead or behind, then loses the faster, leaping hero
struct Bat { float x = 0, y = 0, y0 = 0, t = 0; bool live = false; };   // the bat of every old game
struct Relic { bool live = false, taken = false, clue = false; float wx = 0, shift = 0; };   // the pedestal with the golden mask, in the flat forest
struct Tree { float wx = 0; int throws = 0; double throwAt = -9; bool live = false; };   // a palm with a monkey on it
struct Beach {
  float scroll = 0, jump = -1, jd = JD, jh = JH;
  Obs obs[6];
  bool hills = false;                   // the first forest has hills; the second is flat, with ponds
  struct { bool live = false; int o = 0; float pt = 0; } swing;   // a vine over a wide pond: he jumps up to it, swings across and lets go on the far side
  bool forest = false;                  // the forest (the web demo's land = "forest", hills): the ground is the volume
  int8_t tr[128] = {}; int trN = 0, trPrev = 0;   // the terrain: block heights per column, the newest 80 kept
  int apples[24] = {}; int nApples = 0;           // trunks whose apple has been taken
  Relic rel; struct { bool live = false; float t = 0; } find, fall; double nextRelic = -1; int relics = 0;
  Tree tree;
  Bat bat;
  Hunter hunter; int hunterN = 0; double nextHunter = 0;
  double lastArr = 0, nextGap = 0.78, shipStart = 0, nextTree = 0;
  uint32_t rng = 2463534242u;
};

// The far mountains and the canopy: a picture of the last few seconds of volume, as in KeySpeaker (the web demo's M.far, M.canopy).
struct Mtn { uint8_t far[34] = {}; int canopy[34]; int canopyNext = 10; double farT = 0; float farAcc = 0; int farN = 0, farOff = 0; float avg = -30; bool set = false; Mtn() { for (int i = 0; i < 34; i++) canopy[i] = 10; } };

// ---- Stage 4: the wall (the web demo's "wall"): a landslide blocks the trail; he climbs the rock face hand and foot in turn, one grip on every beat.
enum WlPh : uint8_t { WP_APPROACH, WP_PAUSE, WP_UP, WP_TOP, WP_EXIT };
constexpr int WALL_STEPS = 52, WALL_HEIGHT = WALL_STEPS * 3;   // high enough that the forest floor disappears before the vine-ravine ledge
struct Limb4 { float x, h, from, tgt, t; };
struct Wall {
  int steps = 0; WlPh phase = WP_APPROACH; float tt = 0, stepT = 0, x = -12, z = 0, dodge = 0, grip = 0;
  struct { bool live = false, pass = false; float x = 0, t = 0; } condor; int nextCondor = 0;
  struct Rk { bool live = false; float t = 0, x = 0, y = 0; } rocks[3]; int nextRock = 0;
  Limb4 L[4] = {{-7, 15, 15, 15, 1}, {7, 9, 9, 9, 1}, {-4, -10, -10, -10, 1}, {4, -4, -4, -4, 1}};   // LH, RH, LF, RF
};
enum { LH_ = 0, RH_ = 1, LF_ = 2, RF_ = 3 };

// ---- Stage 5: the vines (the web demo's "vines"): the trail ends at a ravine and hanging vines carry him across.
constexpr float TARZAN_STEP = 5, TARZAN_TOTAL = TARZAN_STEP * 3;
struct Vines { float t = 0; };

// ---- Stage 6: the boat and the falls (the web demo's "boatfall"): the vines land beside a brook; a moored boat carries him over a waterfall.
enum BtPh : uint8_t { BT_SHORE, BT_BOARD, BT_CRUISE, BT_LIP, BT_DROP, BT_SPLASH };
struct Boat { float t = 0; BtPh phase = BT_SHORE; };

// ---- Stage 7: the river (the web demo's "river", the open one): he swims in three lanes and changes lane to keep clear of crocodiles, piranhas, water snakes and drifting wood.
constexpr int LANES[3] = {58, 84, 110};
enum RvKind : uint8_t { RV_CROC, RV_PIRANHA, RV_SNAKE, RV_WOOD };
struct RvObs { bool live = false, sine = false; RvKind kind = RV_WOOD; int lane = 1; float wx = 0; };
struct RvBubble { bool live = false; float x = 0, y = 0, v = 0; };
struct River {
  float scroll = 0, laneY = LANES[1]; int lane = 1, lifeN = 0;
  RvObs obs[10]; RvBubble bub[24];
  double lastArr = 0, nextGap = 0.78, nextLife = 2;
  bool under = false, entryLive = false, exitLive = false; float entryT = 0, exitT = 0;   // the waterway under the treasury: a brick channel with no river life, the splash in from the treasury, the water source at the end
};
constexpr float WATERWAY_LEN = 760;

// ---- Stage 8: the tunnel (the web demo's "cave" with the labyrinth): an old brick tunnel under the forest. A hole drops him a level, a ladder lifts him back up, and at the end a golden doorway leads to the treasure store.
constexpr float LAB_TOP = 160;   // the surface seen from the labyrinth ladder: height of the lip
enum CvKind : uint8_t { CV_ROCK, CV_STAL, CV_LOW, CV_SCORP, CV_SNAKE };
struct CvObs { bool live = false, done = false; CvKind kind = CV_ROCK; float wx = 0; };
enum CvMk : uint8_t { MV_DOWN, MV_UP, MV_GATE };
struct CvMv { bool live = false; CvMk kind = MV_DOWN; bool mount = false; float t = 0, d = 0, z = 40, mt = 0, q = 0; };
struct CvDrip { bool live = false; float x = 0, y = 0, v = 0; };
struct CvSplash { bool live = false; float x = 0, t = 0; };
struct Cave {
  float scroll = 0, wx = 560, pw = 0, ld = -0.45f, jump = -1, jd = JD; int depth = 0, stage = 0;   // stage 0: a hole; 1: a ladder up; 2: the golden doorway
  bool duck = false; double lastArr = 0, nextGap = 0.78, t = 0; float mvT = 0;
  struct { bool live = false; float t = 0; } trap; struct { bool live = false; float x = 0; } arrows[4]; double nextArrow = 6;   // an arrow trap: a slit in the right wall flashes, then shoots
  struct { bool live = false, roll = false, stop = false, gone = false; float t = 0, nx = 0, bw = 0, endT = -1; } boulder; bool bd = false;   // a huge boulder rolls down the tunnel behind him; there is a niche in the wall
  int cartS = 0; bool cart = false, chased = false; float cartX = 0, bt = 0, bs = 0, rideT = 0, vm = 1; float gaps[2] = {0, 0}; int nGaps = 0;   // the mine cart: 1 parked ahead, 2 he hops in, 3 riding; two broken places in the track
  struct { bool live = false; float t = 0; } chase;   // the guardians who come after him
  CvMv mv; CvObs obs[8]; CvDrip drips[16]; CvSplash splash[16]; Bat bats[2];
  uint8_t roof[128] = {}; int roofN = -2, roofPrev = 28; int cv = 28;   // the ceiling, one column every 8 px, each keeping its height while it scrolls left
};

// ---- Stage 9: the treasury (the web demo's "cavern"): a Pitfall II cavern behind the golden door. Ten floors joined by ladders, gaps to jump, ropes over the wide gaps, gold bars on the run, a crowd of guardians at home, and at the top a pit to the river.
constexpr int NF = 10, CWATER = 96 + 24;   // ten floors, 46 dots apart: floor 0 is the top, floor 9 the bottom; the river is below the bottom floor
static inline int CFL(int l) { return 96 - 46 * (9 - l); }
enum StKind : uint8_t { ST_GAP, ST_ROPE, ST_LADUP, ST_PIT };
struct CavSt { int xs; StKind kind; int lvl; };
struct CavGoldP { int x, lvl; };
struct CavTab {   // the route: on each floor from 9 up to 1 an obstacle (a gap, or on floors 6, 3 and 1 a rope over a wide gap) and then a ladder up; on the top floor a pit down to the river
  CavSt st[19]; CavGoldP gold[19]; int nst = 0, ng = 0;
  CavTab() { int i = 0, start = 130; const int SP = 350;
    for (int f = 9; f >= 1; f--) { const int xa = 420 + (i++) * SP, xb = 420 + (i++) * SP; st[nst++] = CavSt{xa, (f == 6 || f == 3 || f == 1) ? ST_ROPE : ST_GAP, f}; st[nst++] = CavSt{xb, ST_LADUP, f};
      gold[ng++] = CavGoldP{(int)floorf((start + xa) / 2.0f + 0.5f), f}; gold[ng++] = CavGoldP{(int)floorf((xa + xb) / 2.0f + 0.5f), f}; start = xb + 5; }
    const int xp = 420 + i * SP; st[nst++] = CavSt{xp, ST_PIT, 0}; gold[ng++] = CavGoldP{(int)floorf((start + xp) / 2.0f + 0.5f), 0}; }
};
static const CavTab& cavTab() { static const CavTab t; return t; }
static bool cavSolid(int lvl, float wx) {
  const CavTab& T = cavTab();
  for (int i = 0; i < T.nst; i++) { const CavSt& st = T.st[i]; if (fabsf(wx - st.xs) > 40) continue;
    if (st.kind == ST_GAP && st.lvl == lvl && fabsf(wx - st.xs) < 18) return false;
    if (st.kind == ST_ROPE && st.lvl == lvl && fabsf(wx - st.xs) < 35) return false;   // a wide gap: only a rope gets across
    if (st.kind == ST_LADUP && lvl == st.lvl - 1 && fabsf(wx - st.xs - 5) < 9) return false;   // the ladder comes up through a hole in the floor above
    if (st.kind == ST_PIT && fabsf(wx - st.xs) < 26) return false; }   // the pit is a shaft through every floor
  return true;
}
enum CvnMk : uint8_t { CM_UP, CM_SWING, CM_DROP, CM_MISS };
struct Spark { bool live = false; float x = 0, y = 0, vx = 0, vy = 0, t = 0; };
struct Cavern {
  float scroll = 0, t = 0, jump = -1, jd = JD, jh = JH; int lvl = 9, bars = 0;
  struct { bool live = false; CvnMk kind = CM_UP; float t = 0, xs = 0, from = 0; } mv;
  bool done[19] = {}, taken[19] = {}; Spark sparks[24]; Bat bats[2];
  struct { bool live = false, stuck = false; float t = 0, nextAttack = 1.5f, attack = 0, wx0 = 0; } chase;   // the temple guardians who come after him on floors 6, 3 and 1; stuck: they stop dead at the edge when he takes a rope
  bool duck = false;   // he ducks the spear thrust
};

enum Scene : uint8_t { SC_LINER, SC_BEACH, SC_FOREST, SC_WALL, SC_VINES, SC_BOAT, SC_RIVER, SC_FOREST3, SC_CAVE, SC_CAVERN, SC_WATERWAY, SC_FOUNTAIN, SC_SKYFALL, SC_PYRAMID, SC_COUNT };

constexpr int PYRAMID_X = 120;
constexpr float PYRAMID_ROCK_TIMES[3] = {3.2f, 6.2f, 9.2f};
struct Pyramid {   // the sunlit outer wall: three boulders bounce down the steps, he ducks under each one
  struct Rock { bool live = false; float x = 0, speed = 0; };
  Rock rocks[3]; int rockN = 0, passed = 0; float scroll = 0; bool look = false, duck = false;
};

struct Game {
  Scene scene = SC_LINER, next = SC_LINER;
  bool isQuiet = false;
  float quietT = 0, hideT = 0; bool hide = false;
  float sneak = 0, sp = 1, fade = 0, visualDark = -1, sl = 0, dt = 0.033f;
  int fadeDir = 0, day = 1;
  double t0 = 0;
  Liner cl;
  Beach bc;
  Mtn mt;
  Wall wl;
  Vines vn;
  Boat bt;
  River rv;
  Cave cv;
  Cavern cn;
  Pyramid py;
  float treble = 0;
};

static void setScene(Game& G, Scene s, double now) {
  G.scene = s; G.t0 = now;
  if (s == SC_FOUNTAIN || s == SC_SKYFALL || s == SC_PYRAMID) G.visualDark = 0;   // bright and clear, no dimming
  if (s == SC_LINER) new (&G.cl) Liner();
  if (s == SC_PYRAMID) new (&G.py) Pyramid();
  if (s == SC_WALL) new (&G.wl) Wall();
  if (s == SC_VINES) new (&G.vn) Vines();
  if (s == SC_BOAT) new (&G.bt) Boat();
  if (s == SC_CAVERN) { uint32_t r = G.bc.rng; new (&G.cn) Cavern(); G.bc.rng = r; }
  if (s == SC_CAVE) { uint32_t r = G.bc.rng; new (&G.cv) Cave(); G.bc.rng = r; G.cv.lastArr = now; G.cv.nextArrow = now + 6; G.cv.roofN = -2; }
  if (s == SC_RIVER || s == SC_WATERWAY) { uint32_t r = G.bc.rng; new (&G.rv) River(); G.bc.rng = r; G.rv.lastArr = now; G.rv.nextLife = now + 2; G.rv.under = s == SC_WATERWAY; G.rv.entryLive = G.rv.under; }
  if (s == SC_BEACH || s == SC_FOREST || s == SC_FOREST3) { uint32_t r = G.bc.rng; new (&G.bc) Beach(); G.bc.rng = r; G.bc.forest = (s != SC_BEACH); G.bc.hills = (s == SC_FOREST); G.bc.lastArr = now; G.bc.shipStart = now; G.bc.nextTree = now + 3; G.bc.nextHunter = now + 8; G.sl = 0; G.hide = false; G.quietT = 0; }
}
static inline bool onLand(const Game& G) { return G.scene == SC_BEACH || G.scene == SC_FOREST || G.scene == SC_FOREST3; }
static void switchTo(Game& G, Scene s) { if (G.fadeDir) return; G.next = s; G.fadeDir = 1; G.fade = 0; }
static void gameInit(Game& G, double now) { new (&G) Game();   // built in place (a Game is over 3 KB: no temporary on the small stack)
  HC = HeroCtx(); setScene(G, SC_LINER, now); }

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

static int terrSegs(Mtn& m, float rms, int ns) {   // how many blocks tall a column is: this moment's loudness against the recent average
  const float rate = 0.08f; const int TSPAN = 8; const float TMID = 0.45f;
  float db = rms > 3e-5f ? 20 * log10f(rms) : -60.0f; if (db <= -60) return 0;
  if (!m.set) { m.avg = db; m.set = true; }
  float v = cl01(TMID + (db - m.avg) / TSPAN); m.avg += (db - m.avg) * rate; return rnd(v * (ns - 1));
}
static void updateMountains(Game& G, const AudioIn& A, double now) {   // the web demo's pushFar(): a new column every 0.267 s
  Mtn& m = G.mt; m.farAcc += A.rms * A.rms; m.farN++;
  if (now - m.farT >= 0.267) {
    float r = m.farN ? sqrtf(m.farAcc / m.farN) : 0; m.farAcc = 0; m.farN = 0;
    int steps = (int)floor((now - m.farT) / 0.267); m.farT += 0.267 * (steps < 1 ? 1 : steps);
    int n = terrSegs(m, A.sounding ? r : 0, 10);
    memmove(m.far + 1, m.far, 33); m.far[0] = (uint8_t)n; memmove(m.canopy + 1, m.canopy, 33 * sizeof(int)); m.canopy[0] = m.canopyNext;
    int target = 10 + 3 * n, d = target - m.canopy[0]; d = d < -5 ? -5 : d > 5 ? 5 : d; m.canopyNext = m.canopy[0] + d;   // leaf height is set before each column enters
  }
  double e = now - m.farT; if (e > 0.267) e = 0.267; m.farOff = (int)floor(e * PITCH / 0.267);
}
static inline int trAt(const Beach& b, int i) { return (i >= b.trN || i < b.trN - 80) ? 0 : b.tr[i & 127]; }
static float gh(const Beach& b, float w) {   // the height (px) of the ground at world x: the block terrain, joined by slopes between the column centres so that he can walk it
  if (!b.forest || !b.hills) return 0;
  float f = (w - 3) / PITCH; int i = (int)floorf(f); return lp((float)trAt(b, i), (float)trAt(b, i + 1), f - i) * SEG;
}
static inline int ghr(const Beach& b, float w) { return rnd(gh(b, w)); }

static void spawnBeach(Game& G, const AudioIn& A, double now) {   // called on a beat: put an obstacle that reaches the hero k beats from now
  Beach& b = G.bc;
  if (G.fadeDir || G.isQuiet || b.rel.live || b.find.live || b.fall.live || A.level < 0.06f) return;   // very quiet: nothing comes, the beach only waits (suspense)
  const float T = A.T, need = (W + 18 - HX + 64) / V;
  int k = (int)ceilf(need / T); if (k < 2) k = 2;
  const double arr = now + k * T;
  if (arr - b.lastArr < b.nextGap || ((A.beatN & 1) && A.level < 0.55f)) return;   // room between obstacles
  const double gap = arr - b.lastArr; b.lastArr = arr; b.nextGap = 3.5 + rand01(G) * 5.5;   // rare enough to matter: every 3.5 to 9 s
  for (Obs& o : b.obs) if (!o.live) {
    o.live = true; o.done = false; o.arr = arr; o.wx = b.scroll + (float)(arr - now) * V;
    { float r = rand01(G);   // the hills: logs, tigers, bears; the flat forest: also ponds; the beach: logs and turtles
      o.kind = b.hills ? (r < 0.4f ? OB_LOG : r < 0.7f ? OB_TIGER : OB_BEAR) : b.forest ? (r < 0.2f ? OB_LOG : r < 0.4f ? OB_TIGER : r < 0.6f ? OB_BEAR : OB_POND) : (r < 0.25f ? OB_LOG : OB_TURTLE);
      o.wide = o.pulse = o.hasCroc = false;
      if (o.kind == OB_POND) { o.wide = rand01(G) < 0.3f && gap >= 1.5; o.pulse = !o.wide && rand01(G) < 0.4f; o.hasCroc = o.wide || rand01(G) < 0.55f; }   // a wide pond (two crocodiles) is crossed on a vine; some ponds breathe with the volume
      o.hw = o.kind == OB_LOG ? 8 : o.kind == OB_TIGER ? 14 : o.kind == OB_BEAR ? 12 : o.wide ? 28 : o.pulse ? 12 : 14;
      if (o.wide) b.lastArr = arr + 0.9;
    }
    break;
  }
}

static inline float monkeyY(const Tree& t, double now) { return (float)rnd(42 + 16 * sinf((float)(now * 0.9 + t.wx * 0.013))); }   // he shuttles up and down the trunk

static void updateBeach(Game& G, const AudioIn& A, float dt, double now) {
  Beach& b = G.bc;
  if (A.beatNow) spawnBeach(G, A, now);
  // a palm with a monkey now and then; he throws a coconut ahead of the hero, and it lands and lies there like a log
  Tree& t = b.tree;
  Bat& bt = b.bat;
  if (A.beatNow && A.sounding && !G.fadeDir && !bt.live && A.level > 0.25f && rand01(G) < 0.1f) { bt = Bat(); bt.live = true; bt.x = W + 8; bt.y = bt.y0 = 38; }
  if (bt.live) { bt.t += dt; bt.x -= (62 + V * G.sp * 0.3f) * dt; float u = bt.t * 1.7f; bt.y = bt.y0 + (fabsf(u - floorf(u) - 0.5f) * 4 - 1) * 12; if (bt.x <= -10) bt.live = false; }   // a triangle wave: straight zigs and zags
  if (!b.forest && !t.live && !G.hide && A.sounding && !G.fadeDir && now > b.nextTree) { t = Tree(); t.live = true; t.wx = b.scroll + W - HX + 24; b.nextTree = now + 7 + rand01(G) * 5; }
  if (t.live) {
    float sx = HX + t.wx - b.scroll;
    if (A.beatNow && t.throws < 2 && sx < 214 && sx > 100 && !G.isQuiet && !G.hide && G.sp > 0.7f && now - t.throwAt > 1.2) {
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
  if (b.hills) {   // the ground is built like KeySpeaker's terrain: each new column at the right edge; at most two blocks of step, so that he can run it
    int eC = (int)floorf((b.scroll - HX) / PITCH);
    if (b.trN < eC - 2) { b.trN = eC; b.trPrev = 0; }
    while (b.trN * PITCH < b.scroll + W - HX + 16) {
      int c = b.trN, n = rnd(3 + 2.5f * sinf(c * 0.095f) + 1.4f * sinf(c * 0.036f) + A.level); n = n < 0 ? 0 : n > 8 ? 8 : n;
      n = n < b.trPrev - 2 ? b.trPrev - 2 : n > b.trPrev + 2 ? b.trPrev + 2 : n;
      b.tr[c & 127] = (int8_t)n; b.trPrev = n; b.trN++;
    }
    G.sl += (fmaxf(-1.0f, fminf(1.0f, (gh(b, b.scroll + 4) - gh(b, b.scroll - 4)) / 8 / 0.5f)) - G.sl) * 0.2f;   // uphill he leans in and shortens his stride, downhill he leans back
  }
  if (b.forest) {   // passing a tree collects its apple like a Pac-Man dot (in both forests)
    int k = rnd((HX + 27 + b.scroll * 0.25f) / 47); float ax = k * 47 - b.scroll * 0.25f - 27;   // passing the tree collects its apple like a Pac-Man dot
    if (fabsf(ax - HX) < 3) { bool have = false; for (int i = 0; i < b.nApples; i++) if (b.apples[i] == k) have = true; if (!have) { if (b.nApples == 24) { memmove(b.apples, b.apples + 1, 23 * sizeof(int)); b.nApples--; } b.apples[b.nApples++] = k; } }
  }
  if (b.forest && !b.hills) {   // treasure: now and then a pedestal with a golden idol stands in the road; he stops, finds it, lifts it high, and the music celebrates; the pedestal slides away and shows a hidden shaft
    if (b.nextRelic < 0) b.nextRelic = now + 12 + rand01(G) * 18;
    if (((A.beatNow && A.sounding && !G.isQuiet && A.level > 0.3f) || (!A.sounding && now - G.t0 >= 100)) && now > b.nextRelic && now - G.t0 >= 82 && !b.find.live && !G.hide && !G.fadeDir && !b.swing.live && !b.rel.live) {
      const float wx = b.scroll + W - HX + 30; bool clear = true; for (const Obs& o : b.obs) if (o.live && fabsf(o.wx - wx) <= 170) clear = false;
      if (clear) { b.rel = Relic(); b.rel.live = true; b.rel.wx = wx; double arr = now + (wx - b.scroll) / V + 1; if (arr > b.lastArr) b.lastArr = arr; b.nextGap = 4; b.nextRelic = now + 45 + rand01(G) * 45; }
    }
    if (b.fall.live) { b.fall.t += dt; if (b.fall.t > 0.55f && !G.fadeDir) switchTo(G, SC_CAVE); }   // the revealed shaft leads straight into the brick tunnel
    if (!b.find.live && !b.fall.live && b.jump < 0 && !b.swing.live && !G.fadeDir && b.rel.live && !b.rel.taken && b.rel.wx - b.scroll <= 16) { b.find.live = true; b.find.t = 0; }
    if (b.find.live) {
      Relic& r = b.rel; b.find.t += dt; const float ft = b.find.t;
      if (ft >= 1.7f && !r.taken) { r.taken = true; b.relics++; }
      r.clue = ft >= 2.4f && ft < 3.2f; r.shift = r.clue ? (float)((int)floorf(ft * 12) & 1) : (float)rnd(32 * sm01(cl01((ft - 3.2f) / 1.5f)));
      if (ft > 4.9f) { b.fall.live = true; b.fall.t = 0; b.find.live = false; }   // take the mask, notice the inlay, push the pedestal, then enter the hidden shaft
    }
    if (b.rel.live && HX + b.rel.wx - b.scroll <= -30) b.rel.live = false;
  }
  Hunter& hu = b.hunter;
  if (!hu.live && now >= b.nextHunter && !G.fadeDir && !G.hide && !b.swing.live && !b.find.live && !b.fall.live && !b.rel.live) { hu = Hunter(); hu.live = true; hu.front = (++b.hunterN & 1) != 0; b.nextHunter = now + 17 + rand01(G) * 7; }
  if (hu.live) {
    hu.t += dt;
    if (!hu.dodged && hu.t >= (hu.front ? 2.4f : 1.5f)) { hu.dodged = true; if (b.jump < 0 && !G.hide) { b.jump = 0; b.jd = JD; b.jh = JH; } }   // the hero leaps and leaves him behind
    if (hu.t > 5) hu.live = false;
  }
  for (Obs& o : b.obs) if (o.live && o.pulse && obsX(b, o) > HX + 75) o.hw += ((8 + 12 * A.level) - o.hw) * fminf(1.0f, dt * 6);   // a breathing pond: its width follows the volume until it is close, then it holds still
  if (b.swing.live) { float dx = obsTtl(b, b.obs[b.swing.o]) * V; if (dx < -40) { b.swing.pt += dt; if (b.swing.pt > 0.4f) b.swing.live = false; } }
  else if (b.jump < 0) { for (int i = 0; i < 6; i++) { Obs& o = b.obs[i]; if (o.live && o.wide && !o.done) { float d = obsTtl(b, o) * V; if (d < 67 && d > -40) { o.done = true; b.swing.live = true; b.swing.o = i; b.swing.pt = 0; break; } } } }
  if (b.jump < 0 && !b.swing.live) {   // hop what comes: he leaves the ground half a jump before it arrives, so he lands on the beat
    for (Obs& o : b.obs) if (o.live && !o.done && !o.wide) { float t = obsTtl(b, o); if (t < JD / 2 && t > -0.1f) { b.jd = JD; b.jh = JH; b.jump = fmaxf(0, b.jd / 2 - t); o.done = true; break; } }
  }
  if (!b.forest && !G.fadeDir && now - b.shipStart >= 48) switchTo(G, SC_FOREST);   // the liner disappears at the horizon, and the beach gives way to forest
  if (b.hills && !G.fadeDir && !G.hide && now - G.t0 >= 55) switchTo(G, SC_WALL);   // the trail ends at a landslide
  if (b.forest && !b.hills && !G.fadeDir && !G.hide && now - G.t0 >= 100) switchTo(G, SC_LINER);   // the treasure and the hidden shaft come next in the web demo; until they are ported, the story starts again
}

static inline float rvTtl(const River& r, const RvObs& o) { return (o.wx - r.scroll) / V; }
static void updateRiver(Game& G, const AudioIn& A, float dt, double now) {
  River& r = G.rv;
  r.scroll += V * G.sp * dt;
  if (!r.under && A.beatNow && !G.fadeDir && !G.isQuiet && A.level >= 0.06f) {   // on a beat: something comes along the river (crocodiles, piranhas and water snakes share the lanes)
    const float T = A.T, need = (W + 18 - HX + 64) / V; int k = (int)ceilf(need / T); if (k < 2) k = 2;
    const double arr = now + k * T;
    if (!(arr - r.lastArr < r.nextGap || ((A.beatN & 1) && A.level < 0.55f))) {
      r.lastArr = arr; r.nextGap = 2.2 + rand01(G) * 2.4;
      for (RvObs& o : r.obs) if (!o.live) { float q = rand01(G); o.live = true; o.kind = q < 0.25f ? RV_CROC : q < 0.52f ? RV_PIRANHA : q < 0.73f ? RV_SNAKE : RV_WOOD; o.wx = r.scroll + (float)(arr - now) * V; o.sine = rand01(G) < 0.5f; o.lane = (int)(rand01(G) * 3); break; }
    }
  }
  if (!r.under && now >= r.nextLife && !G.fadeDir) {   // and on a schedule, the three kinds in turn
    static const RvKind KINDS[3] = {RV_PIRANHA, RV_SNAKE, RV_CROC}; static const int LN[3] = {0, 2, 1};
    const RvKind kind = KINDS[r.lifeN % 3]; const int lane = LN[r.lifeN % 3]; r.lifeN++;
    const float wx = r.scroll + W - HX + 24; const double arr = now + (wx - r.scroll) / V;
    for (RvObs& o : r.obs) if (!o.live) { o.live = true; o.kind = kind; o.wx = wx; o.sine = kind != RV_CROC; o.lane = lane; break; }
    if (arr > r.lastArr) r.lastArr = arr; r.nextLife = now + 7;
  }
  {   // he changes lane to keep clear: out of the way of what is about to reach him, and back to the middle when the way is free
    const RvObs* hit = nullptr; for (const RvObs& o : r.obs) if (o.live && o.lane == r.lane && rvTtl(r, o) < 0.75f && rvTtl(r, o) > -0.1f) { hit = &o; break; }
    auto busy = [&](int l) { for (const RvObs& p : r.obs) if (p.live && p.lane == l && rvTtl(r, p) < 1.2f && rvTtl(r, p) > -0.2f) return true; return false; };
    if (hit) { int best = -1; for (int l = 0; l < 3; l++) if (!busy(l) && (best < 0 || abs(l - r.lane) < abs(best - r.lane))) best = l; if (best >= 0) r.lane = best; }
    else if (r.lane != 1 && !busy(1) && A.beatNow) r.lane = 1;
  }
  const float ty = (float)LANES[r.lane], dy = ty - r.laneY, step = fminf(fabsf(dy), 170 * dt); r.laneY += (dy > 0 ? 1.0f : dy < 0 ? -1.0f : 0.0f) * step;
  if (A.beatNow || rand01(G) < 0.04f) for (RvBubble& b : r.bub) if (!b.live) { b.live = true; b.x = HX - 4; b.y = r.laneY - 4; b.v = 18 + rand01(G) * 14; break; }
  for (RvBubble& b : r.bub) if (b.live) { b.y -= b.v * dt; if (!r.under) b.x -= V * 0.3f * dt; if (b.y <= 36 || b.x <= -4) b.live = false; }
  for (RvObs& o : r.obs) if (o.live && HX + o.wx - r.scroll <= -40) o.live = false;
  if (r.under) {
    if (r.entryLive) { r.entryT += dt; if (r.entryT >= 1.1f) r.entryLive = false; }
    if (!r.exitLive && r.scroll >= WATERWAY_LEN - 6) { r.scroll = WATERWAY_LEN - 6; r.exitLive = true; r.exitT = 0; r.lane = 1; }
    if (r.exitLive) { r.exitT += dt; if (r.exitT > 1.7f && !G.fadeDir) setScene(G, SC_FOUNTAIN, now); }   // the current feeds a vertical geyser inside the building
  }
  else if (!G.fadeDir && now - G.t0 >= 90 && (r.scroll > V * 90 * 0.65f || now - G.t0 >= 165)) switchTo(G, SC_FOREST3);   // the river gives way to the flat forest
}

// The ceiling: the underside of the roof at screen column x.
static inline int roofAt(const Cave& c, int i) { return (i >= c.roofN || i < c.roofN - 90) ? -1 : c.roof[i & 127]; }
static int cey(const Cave& c, float x) {
  const float f = (c.scroll + x) / PITCH; const int i = (int)floorf(f);
  int r0 = roofAt(c, i); if (r0 < 0) r0 = c.cv; int r1 = roofAt(c, i + 1); if (r1 < 0) r1 = roofAt(c, i) >= 0 ? roofAt(c, i) : c.cv;
  return rnd(lp((float)r0, (float)r1, f - i) + 2 * sinf((x + c.scroll) * 0.13f));
}
static float labH(const Cave& c, float wx) {   // the labyrinth floor: rolling slopes between the events, flat near a hole or a ladder
  for (int i = 0; i < c.nGaps; i++) if (fabsf(wx - c.gaps[i]) < 60) wx = c.gaps[i] - 60;   // a level plateau round each broken place in the track, so that he lands at the height he left
  const float d = fminf(fabsf(wx - c.wx), fabsf(wx - c.pw)), w = cl01((d - 50) / 70);
  const float base = fmaxf(0.0f, 15 * sinf(wx * 0.028f + 0.6f) + 3 * sinf(wx * 0.07f) + 6) * w, co = c.nGaps == 0 ? 0.0f : cl01((wx - c.cartX - 40) / 80);
  return co > 0 ? lp(base, (22 - 22 * cosf((wx - c.cartX - 40) * 0.01309f)) * w, co) : base;   // after the cart: long slopes, a whole screen climbing, then a whole screen descending
}
static inline float cvTtl(const Cave& c, const CvObs& o) { return (o.wx - c.scroll) / (V * c.vm); }
static void updateCave(Game& G, const AudioIn& A, float dt, double now) {
  Cave& L = G.cv; L.t += dt; L.scroll += V * G.sp * dt;   // (G.sp already includes the cart's speed-up)
  if (L.ld < 1) L.ld += dt;
  if (L.mv.live) L.mvT += dt; else L.mvT = 0;
  if ((L.t > 170 || L.mvT > 14) && !G.fadeDir) switchTo(G, SC_LINER);   // a watchdog: whatever goes wrong, the tunnel lets him out (back to the start for now)
  // the ceiling presses: the light shrinks and the roof falls when it is quiet and rises when it is loud
  while (L.roofN * PITCH < L.scroll + W + 16) { int target = rnd(L.cart ? 26 - 11 * A.level : 38 - 22 * A.level), i = L.roofN++; int d = target - L.roofPrev; d = d < -2 ? -2 : d > 2 ? 2 : d; L.roofPrev += d; L.roof[i & 127] = (uint8_t)L.roofPrev; }
  L.cv = cey(L, HX);
  if (A.beatNow && A.sounding) for (CvDrip& d : L.drips) if (!d.live) { d.live = true; d.x = 90 + rand01(G) * 150; d.y = (float)cey(L, d.x); d.v = 0; break; }
  for (CvDrip& d : L.drips) if (d.live) { d.v += 260 * dt; d.y += d.v * dt; d.x -= V * G.sp * dt; if (d.y >= GY - rnd(labH(L, L.scroll + d.x - HX))) { d.live = false; for (CvSplash& p : L.splash) if (!p.live) { p.live = true; p.x = d.x; p.t = 0; break; } } else if (d.x <= -4) d.live = false; }
  for (CvSplash& p : L.splash) if (p.live) { p.t += dt; p.x -= V * G.sp * dt; if (p.t >= 0.25f) p.live = false; }
  // bats
  if (A.beatNow && A.sounding && !G.fadeDir && A.level > 0.25f && rand01(G) < 0.3f) { int n = 0; for (const Bat& b : L.bats) if (b.live) n++; if (n < 2) for (Bat& b : L.bats) if (!b.live) { b = Bat(); b.live = true; b.x = W + 8; b.y = b.y0 = (float)(L.cv + 28 > 40 ? L.cv + 28 : 40); break; } }
  for (Bat& b : L.bats) if (b.live) { b.t += dt; b.x -= (62 + V * G.sp * 0.3f) * dt; float u = b.t * 1.7f; b.y = b.y0 + (fabsf(u - floorf(u) - 0.5f) * 4 - 1) * 12; if (b.x <= -10) b.live = false; }
  // obstacles come on the beat, once the way is clear of the end of the tunnel
  if (A.beatNow && !G.fadeDir && !G.isQuiet && A.level >= 0.06f) {
    const float T = A.T, need = (W + 18 - HX + 64) / V; int k = (int)ceilf(need / T); if (k < 2) k = 2;
    const double arr = now + k * T;
    if (!(arr - L.lastArr < L.nextGap || ((A.beatN & 1) && A.level < 0.55f))) {
      L.lastArr = arr; L.nextGap = 1.8 + rand01(G) * 2.2;
      if (!(L.wx - L.scroll < 460 || L.boulder.live)) for (CvObs& o : L.obs) if (!o.live) { float r = rand01(G); o.live = true; o.done = false; o.wx = L.scroll + (float)(arr - now) * V; o.kind = r < 0.14f ? CV_ROCK : r < 0.28f ? CV_STAL : r < 0.42f ? CV_LOW : r < 0.72f ? CV_SCORP : CV_SNAKE; break; }
    }
  }
  for (CvObs& o : L.obs) if (o.live && HX + o.wx - L.scroll <= -40) o.live = false;
  // jump what comes; duck under a low roof
  if (L.jump >= 0) { L.jump += dt; if (L.jump > JD) L.jump = -1; }
  L.duck = false;
  // the mine cart waits ahead on the lower level; the guardians come after him; he hops in and rides
  if (!L.chased && L.depth == 1 && L.ld >= 1 && !L.mv.live) { L.chased = true; L.cartS = 1; L.cartX = L.scroll + 260; L.wx = L.cartX + 2100; L.gaps[0] = L.cartX + 760; L.gaps[1] = L.cartX + 1500; L.nGaps = 2; }   // a long ride ahead, with two places where the track is broken
  if (L.cartS == 1 && !L.chase.live && L.cartX - L.scroll < 140) { L.chase.live = true; L.chase.t = 0; }   // he sees it, and the guardians come after him
  if (L.cartS == 1 && L.cartX - L.scroll <= 14 && !L.mv.live) { L.cartS = 2; L.bt = 0; L.bs = L.scroll; L.jump = -1; }   // he reaches it and hops in
  if (L.cartS == 2) { L.bt += dt; L.scroll = lp(L.bs, L.cartX, sm01(cl01(L.bt / 0.5f))); if (L.bt >= 0.55f) { L.cartS = 3; L.cart = true; L.rideT = 0; } }   // the world slides the last few dots while he jumps; then the cart rolls off
  if (L.cartS == 3) L.rideT += dt;
  if (L.mv.live) { L.cart = false; if (L.cartS == 3) L.cartS = 0; }
  if (L.chase.live) { L.chase.t += dt; if (L.chase.t > 11) L.chase.live = false; }
  // a rumble: a huge boulder comes down the tunnel; there is a niche in the wall just ahead
  if (!L.bd && L.stage == 0 && !L.mv.live && L.ld >= 1 && !L.boulder.live && L.wx - L.scroll < 330 && L.wx - L.scroll > 250) {
    L.bd = true; for (CvObs& o : L.obs) if (o.live && !(o.wx < L.scroll - 20)) o.live = false;
    L.boulder = {}; L.boulder.live = true; L.boulder.nx = L.scroll + 120; }
  if (L.boulder.live) {
    auto& B = L.boulder; B.t += dt; const float dxn = B.nx - L.scroll;
    if (!B.roll && dxn <= 0) { B.roll = true; B.bw = L.scroll + (W + 50 - HX); }   // he is in the niche: now it comes into view, from the right
    if (B.roll) { B.bw -= 230 * dt; if (HX + B.bw - L.scroll < -60) B.gone = true; }
    B.stop = dxn <= 0 && !(B.gone && HX + B.bw - L.scroll < -90);
    if (B.gone && HX + B.bw - L.scroll < -90) { B.stop = false; if (B.endT < 0) B.endT = B.t; if (B.t - B.endT > 0.5f) B.live = false; }
  }
  // an arrow trap: a slit in the right wall flashes, then an arrow flies along the tunnel at head height
  if (!L.mv.live && !L.boulder.live && !L.trap.live && A.beatNow && now > L.nextArrow && L.ld >= 1 && L.wx - L.scroll > 320) {
    bool clear = true; for (const CvObs& o : L.obs) if (o.live && cvTtl(L, o) < 1.8f && cvTtl(L, o) > -0.3f) clear = false;
    if (clear) { L.trap.live = true; L.trap.t = 0; L.nextArrow = now + (L.stage == 2 ? 2.5 + rand01(G) * 2.5 : 5 + rand01(G) * 4); }
  }
  if (L.trap.live) { L.trap.t += dt; if (L.trap.t > 0.5f) { for (auto& a : L.arrows) if (!a.live) { a.live = true; a.x = W - 2; break; } L.trap.live = false; } }
  for (auto& a : L.arrows) if (a.live) { a.x -= 260 * dt; if (a.x <= -20) a.live = false; }
  if (L.jump < 0 && !L.mv.live) { for (const auto& a : L.arrows) if (a.live && a.x - HX < 52 && a.x - HX > -6) L.duck = true; if (L.boulder.live && L.boulder.stop) L.duck = true; }   // he ducks under an arrow, and presses into the niche while the boulder passes
  for (const CvObs& o : L.obs) if (o.live && o.kind == CV_LOW && cvTtl(L, o) < 0.3f && cvTtl(L, o) > -0.28f && L.jump < 0) L.duck = true;
  if (L.jump < 0 && !L.duck && !L.mv.live && L.cartS != 2) for (CvObs& o : L.obs) if (o.live && !o.done && o.kind != CV_LOW && cvTtl(L, o) < JD / 2 && cvTtl(L, o) > -0.1f) { L.jump = fmaxf(0, JD / 2 - cvTtl(L, o)); o.done = true; break; }
  // the way on: a hole down, a ladder up, then the golden doorway
  if (L.stage == 2 && now - G.t0 < 86 && L.wx - L.scroll < 300) L.wx = L.scroll + 520;   // the doorway waits until the stage has had its time
  if (!L.mv.live && L.wx - L.scroll <= (L.stage == 2 ? 14 : 5) && !G.fadeDir) { L.mv = CvMv(); L.mv.live = true; L.mv.kind = L.stage == 2 ? MV_GATE : L.stage == 0 ? MV_DOWN : MV_UP; }
  if (L.mv.live) {
    CvMv& m = L.mv;
    if (m.kind == MV_GATE) { m.t += dt; HC.ph += dt / A.T * 0.7f; if (m.t > 1.2f && !G.fadeDir) switchTo(G, SC_CAVERN); }   // through the golden doorway into the treasure store
    else if (m.kind == MV_DOWN) { m.t += dt; m.d = 135 * powf(fminf(1.0f, m.t / 1.0f), 2); if (m.t >= 1.0f) { L.depth++; L.stage++; m.live = false; L.ld = 0; L.pw = L.wx; L.wx = L.scroll + (L.depth == 1 ? 1000 : 560); } }
    else if (m.mount) { m.mt += dt; m.q = cl01(m.mt / 1.8f); m.z = LAB_TOP - 12 + 12 * sm01(m.q); if (m.q >= 1) { L.depth--; L.stage++; m.live = false; L.ld = 1; L.pw = L.wx; L.wx = L.scroll + 600; } }
    else { m.z += 40 * dt; if (m.z >= LAB_TOP - 12) { m.z = LAB_TOP - 12; m.mount = true; m.mt = 0; } }
  }
  // the slope under him leans him in or back
  { const float sl = (labH(L, L.scroll + 4) - labH(L, L.scroll - 4)) / 8; G.sl += (fmaxf(-1.0f, fminf(1.0f, sl / 0.5f)) - G.sl) * 0.2f; }
}

static void updateCavern(Game& G, const AudioIn& A, float dt, double now) {
  Cavern& C = G.cn; const CavTab& T = cavTab(); C.t += dt; C.scroll += V * G.sp * dt;
  if (C.jump >= 0) { C.jump += dt; if (C.jump > C.jd) C.jump = -1; }
  if (!C.mv.live && C.lvl == NF - 1 && C.jump < 0 && !cavSolid(C.lvl, C.scroll)) { C.mv.live = true; C.mv.kind = CM_MISS; C.mv.t = 0; C.mv.from = (float)CFL(C.lvl); }   // a short jump can miss the first gap and drop into the river
  if (!C.mv.live) {
    for (int i = 0; i < T.nst; i++) { const CavSt& st = T.st[i]; const float dx = st.xs - C.scroll;
      if (st.lvl != C.lvl || C.done[i]) continue;
      if (st.kind == ST_GAP && C.jump < 0 && dx <= 28 && dx > 0) { const bool shortJ = C.lvl == NF - 1 && A.sounding && A.level < 0.35f; C.jd = shortJ ? 0.36f : A.sounding ? JD : 0.82f; C.jh = shortJ ? 20.0f : JH; C.jump = 0; C.done[i] = true; }   // a slow silent run still clears an ordinary gap
      else if (st.kind == ST_LADUP && dx <= 0 && C.jump < 0) { C.scroll = (float)st.xs; C.mv.live = true; C.mv.kind = CM_UP; C.mv.t = 0; C.done[i] = true; }
      else if (st.kind == ST_ROPE && dx <= 29 && C.jump < 0) { C.mv.live = true; C.mv.kind = CM_SWING; C.mv.t = 0; C.mv.xs = (float)st.xs; C.done[i] = true; if (C.chase.live) { C.chase.stuck = true; C.chase.t = fminf(C.chase.t, 6.0f); C.chase.wx0 = st.xs - 35.0f; } }   // a rope over a wide gap: he grabs it and swings across
      else if (st.kind == ST_PIT && dx <= 0 && C.jump < 0) { C.mv.live = true; C.mv.kind = CM_DROP; C.mv.t = 0; C.mv.from = (float)CFL(0); C.done[i] = true; } }
    for (int i = 0; i < T.ng; i++) { const CavGoldP& p = T.gold[i]; if (C.taken[i] || p.lvl != C.lvl || C.jump >= 0) continue;   // gold bars are picked up on the run
      if (fabsf(p.x - C.scroll) < 7) { C.taken[i] = true; C.bars++; for (int k = 0; k < 8; k++) for (Spark& s : C.sparks) if (!s.live) { s.live = true; s.x = HX + 4; s.y = (float)(CFL(C.lvl) - 8); s.vx = (rand01(G) - 0.5f) * 60; s.vy = -30 - rand01(G) * 40; s.t = 0; break; } } }
  } else {
    auto& m = C.mv; m.t += dt;
    if (m.kind == CM_SWING) { if (m.xs - C.scroll < -29) m.live = false; }
    else if (m.kind == CM_DROP || m.kind == CM_MISS) { const float splashAt = sqrtf((CWATER - m.from) / 300); if (m.t > splashAt + 0.12f && !G.fadeDir) setScene(G, SC_WATERWAY, now); }   // the river carries him into the waterway, straight in without a black cut
    else if (m.t >= 2.0f) { C.lvl--; m.live = false; if (C.lvl == 6 || C.lvl == 3 || C.lvl == 1) { C.chase.live = true; C.chase.stuck = false; C.chase.t = 0; C.chase.nextAttack = 1.5f; C.chase.attack = 0; } }   // up on floors 6, 3 and 1 the temple guardians, who live here, come after him
  }
  if (C.chase.live) { auto& ch = C.chase; ch.t += dt;
    if (!ch.stuck && !C.mv.live && ch.t > 1.2f && ch.t >= ch.nextAttack) { ch.attack = 0.45f; ch.nextAttack = ch.t + 2.6f; }
    if (ch.stuck) ch.attack = 0; else if (ch.attack > 0) ch.attack = fmaxf(0.0f, ch.attack - dt);
    if (ch.t > 11) ch.live = false; }
  C.duck = C.jump < 0 && !C.mv.live && C.chase.live && C.chase.attack > 0 && C.chase.attack < 0.28f;   // duck the guardian's spear thrust
  for (Spark& s : C.sparks) if (s.live) { s.t += dt; s.x += s.vx * dt; s.y += s.vy * dt; s.vy += 120 * dt; if (s.t >= 0.7f) s.live = false; }
  // bats
  if (A.beatNow && A.sounding && !G.fadeDir && A.level > 0.25f && rand01(G) < 0.1f) { int n = 0; for (const Bat& b : C.bats) if (b.live) n++; if (n < 1) for (Bat& b : C.bats) if (!b.live) { b = Bat(); b.live = true; b.x = W + 8; b.y = b.y0 = 64; break; } }
  for (Bat& b : C.bats) if (b.live) { b.t += dt; b.x -= (62 + V * G.sp * 0.3f) * dt; float u = b.t * 1.7f; b.y = b.y0 + (fabsf(u - floorf(u) - 0.5f) * 4 - 1) * 12; if (b.x <= -10) b.live = false; }
  (void)now;
}

static void updateWall(Game& G, const AudioIn& A, float dt, double now) {
  Wall& c = G.wl; const int ORDER[4] = {RH_, LF_, LH_, RF_};
  c.tt += dt; c.stepT += dt; c.dodge = fmaxf(0.0f, c.dodge - dt); c.grip = fmaxf(0.0f, c.grip - dt);
  if (c.phase == WP_APPROACH) { c.x = fminf(91.0f, c.x + V * G.sp * dt); if (c.x >= 91) { c.phase = WP_PAUSE; c.tt = 0; } }
  else if (c.phase == WP_PAUSE) { if (c.tt >= 0.75f) { c.phase = WP_UP; c.tt = 0; c.stepT = 0; } }
  else if (c.phase == WP_UP || c.phase == WP_TOP) {
    for (Limb4& l : c.L) if (l.t < 1) { l.t = fminf(1.0f, l.t + dt / (0.55f * A.T)); l.h = lp(l.from, l.tgt, sm01(l.t)); }
    if (c.phase == WP_UP) {
      static const int ROCK_AT[3] = {8, 24, 39}, ROCK_X[3] = {197, 174, 190}, CONDOR_AT[3] = {10, 27, 42};
      if (c.nextRock < 3 && c.steps >= ROCK_AT[c.nextRock]) {   // a stone comes down the face
        for (Wall::Rk& r : c.rocks) if (!r.live) { r.live = true; r.t = 0; r.x = (float)ROCK_X[c.nextRock]; r.y = fmaxf(28.0f, 98 + fmaxf(0.0f, c.z - 22) - WALL_HEIGHT + 15); break; }
        c.nextRock++;
      }
      for (Wall::Rk& r : c.rocks) if (r.live) { r.t += dt; if (r.t >= 2.3f) r.live = false; }
      if (!c.condor.live && c.nextCondor < 3 && c.steps >= CONDOR_AT[c.nextCondor]) { c.condor.live = true; c.condor.x = W + 20; c.condor.pass = false; c.condor.t = 0; }
      if (c.condor.live) {
        c.condor.x -= 100 * dt; c.condor.t += dt;
        if (!c.condor.pass && c.condor.x <= 111) { c.condor.pass = true; c.dodge = 0.85f; c.grip = 0.65f; c.stepT = 0; c.nextCondor++; }   // he ducks and holds on
        if (c.condor.x < -25) c.condor.live = false;
      }
      if (c.grip <= 0 && ((A.beatNow && !G.isQuiet) || c.stepT >= (G.isQuiet ? 1.15f : A.sounding ? 1.4f : 0.8f))) {
        c.stepT = 0; Limb4& l = c.L[ORDER[c.steps % 4]]; l.from = l.h; l.tgt = l.h + 12; l.t = 0; c.steps++;
        if (c.steps >= WALL_STEPS) { c.phase = WP_TOP; c.tt = 0; }
      }
    } else if (c.tt >= 1.2f) { c.phase = WP_EXIT; c.tt = 0; c.x = 100; }
  } else if (c.phase == WP_EXIT) {
    c.x += V * G.sp * dt;
    if (c.x >= W + 12 && !G.fadeDir) switchTo(G, SC_VINES);   // over the top, to the vine ravine
  }
  c.z = (c.L[LH_].h + c.L[RH_].h + c.L[LF_].h + c.L[RF_].h) / 4 - 2.5f;
  if (c.phase == WP_UP) c.x = 91;
  (void)now;
}

// One frame of the game: the quiet detector, the sneak, the stride, then the stage and the fade between stages.
static void gameUpdate(Game& G, const AudioIn& A, float dt, double now) {
  G.dt = dt;
  G.isQuiet = A.sounding && (G.isQuiet ? A.level < 0.38f : A.level < 0.27f);
  G.sneak += (((G.isQuiet || !A.sounding) ? 1.0f : 0.0f) - G.sneak) * fminf(1.0f, dt * 2);
  { float sum = 0; for (int j = 19; j < 30; j++) sum += A.eqH[j]; float raw = A.sounding ? fminf(1.0f, sum / 70) : 0; G.treble += (raw - G.treble) * (1.0f - powf(0.88f, 60.0f * dt)); }   // the high bands: a small sway of the flames
  G.quietT = G.isQuiet ? G.quietT + dt : fminf(0.0f, G.quietT + dt);   // seconds of quiet; after a hide it counts up from negative (a rest)
  if (onLand(G) && !G.hide && !G.fadeDir && !G.bc.find.live && G.quietT > 5 && G.bc.jump < 0) {   // quiet for long: he hides behind a bush, unless something is about to reach him
    bool near = false; for (const Obs& o : G.bc.obs) if (o.live) { float x = obsX(G.bc, o); if (x > HX - 14 && x < HX + 110) near = true; }
    if (!near) { G.hide = true; G.hideT = 0; }
  }
  if (G.hide) { G.hideT += dt; if (G.hideT > 5.9f || (A.level > 0.5f && G.hideT > 1)) { G.hide = false; G.quietT = -12; } }
  G.sp = ((G.hide || (onLand(G) && (G.bc.find.live || G.bc.fall.live)) || (G.scene == SC_CAVE && (G.cv.mv.live || G.cv.cartS == 2 || (G.cv.boulder.live && G.cv.boulder.stop))) || (G.scene == SC_CAVERN && G.cn.mv.live && G.cn.mv.kind != CM_SWING) || (G.scene == SC_WATERWAY && G.rv.exitLive) || (G.scene == SC_PYRAMID && G.py.duck)) ? 0.0f : 1 - 0.4f * G.sneak) * (onLand(G) && G.bc.hunter.live ? 1.2f : 1.0f);   // with a hunter behind him he keeps running
  if (G.scene == SC_CAVERN && G.cn.mv.live && G.cn.mv.kind == CM_SWING) G.sp = 0.7f;   // swinging on a rope
  if (G.scene == SC_CAVE) {   // in the mine cart the tunnel races past, faster in loud music
    Cave& L = G.cv; L.vm = (L.cart && !L.mv.live) ? (1.45f + 0.5f * A.level) * (0.25f + 0.75f * sm01(cl01(L.rideT / 1.5f))) * fmaxf(0.6f, fminf(1.6f, 1 - 2.2f * (labH(L, L.scroll + 4) - labH(L, L.scroll - 4)) / 8)) : 1.0f; G.sp *= L.vm; }
  HC.sneak = G.sneak;
  if (!G.hide) HC.ph += dt * G.sp / (A.sounding ? A.T : 0.6f);   // the stride follows the beat: the left foot lands on it
  if (!G.hide && A.beatNow && G.sneak < 0.3f) { float f = (HC.ph - 0.25f) - floorf(HC.ph - 0.25f), e = f < 0.5f ? -f : 1 - f; HC.ph += e * 0.35f; }
  HC.ph -= floorf(HC.ph);
  updateMountains(G, A, now);
  if (G.scene == SC_LINER) updateLiner(G, A, dt, now);
  else if (G.scene == SC_WALL) updateWall(G, A, dt, now);
  else if (G.scene == SC_RIVER || G.scene == SC_WATERWAY) updateRiver(G, A, dt, now);
  else if (G.scene == SC_FOUNTAIN) { if (now - G.t0 > 4.9) setScene(G, SC_SKYFALL, now); }   // the geyser throws him out of the top of the building
  else if (G.scene == SC_SKYFALL) { if (now - G.t0 > 3.3) setScene(G, SC_PYRAMID, now); }   // he lands on the sunlit outer wall
  else if (G.scene == SC_PYRAMID) {
    Pyramid& P = G.py; const float el = (float)(now - G.t0); P.scroll += V * G.sp * dt;
    while (P.rockN < 3 && el >= PYRAMID_ROCK_TIMES[P.rockN]) { Pyramid::Rock& r = P.rocks[P.rockN]; r.live = true; r.x = -20; r.speed = 60.0f + (P.rockN % 3) * 4; P.rockN++; }
    for (Pyramid::Rock& r : P.rocks) if (r.live) { r.x += r.speed * dt; if (r.x >= W + 20) { r.live = false; P.passed++; } }
    const float runX = PYRAMID_X + rnd(24 * sm01(cl01(el / 0.9f)));
    P.look = P.duck = false;
    for (const Pyramid::Rock& r : P.rocks) if (r.live) { if (r.x > runX - 90 && r.x <= runX - 45) P.look = true; if (r.x > runX - 45 && r.x < runX + 25) P.duck = true; }
    if (P.passed >= 3 && !G.fadeDir) switchTo(G, SC_LINER);   // the jungle is not ported yet: the story starts again
  }
  else if (G.scene == SC_CAVE) updateCave(G, A, dt, now);
  else if (G.scene == SC_CAVERN) updateCavern(G, A, dt, now);
  else if (G.scene == SC_BOAT) {
    Boat& b = G.bt; b.t += dt;
    b.phase = b.t < 1.6f ? BT_SHORE : b.t < 2.8f ? BT_BOARD : b.t < 8.4f ? BT_CRUISE : b.t < 11 ? BT_LIP : b.t < 13.5f ? BT_DROP : BT_SPLASH;
    if (b.t >= 15.5f && !G.fadeDir) setScene(G, SC_RIVER, now);   // the waterfall basin opens straight into the crocodile river
  }
  else if (G.scene == SC_VINES) { G.vn.t = fminf(TARZAN_TOTAL, G.vn.t + dt); if (G.vn.t >= TARZAN_TOTAL && !G.fadeDir) switchTo(G, SC_BOAT); }   // on to the brook and the boat
  else updateBeach(G, A, dt, now);
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

static void drawJaguar(float x, double now) {   // something big and quiet passing behind the bush: a dark cat with yellow eyes, legs padding
  const uint16_t K = rgb(14, 12, 18), GOLD = rgb(255, 210, 60); const float w = (float)fmod(now * 9, 6.283185307179586);
  for (int i = 0; i < 4; i++) { float lx = x - 8 + (i >> 1) * 15 + (i & 1) * 3; int o = rnd(sinf(w + i * 1.6f) * 2); R(lx + o, GY - 5, 2, 5, K); }
  R(x - 11, GY - 12, 24, 8, K); R(x - 18, GY - 14, 8, 7, K); R(x - 17, GY - 16, 2, 2, K); R(x - 12, GY - 16, 2, 2, K);
  seg(x + 12, GY - 11, x + 19, GY - 15, K, 2); seg(x + 19, GY - 15, x + 21, GY - 11, K, 2);
  R(x - 17, GY - 12, 2, 2, GOLD); R(x - 13, GY - 12, 2, 2, GOLD);
}
static void drawBush(float x, float u) {   // the bush the hero hides behind, grown from the ground by u (0..1)
  const uint16_t LEAF = rgb(10, 90, 30), LEAF2 = rgb(20, 130, 40);
  const int h = rnd(15 * u); if (h < 1) return;
  for (int i = 0; i < 6; i++) { int bx = rnd(x - 12 + i * 4), bh = (int)(h - (i % 2) * 3 - fabsf(i - 2.5f) * 1.2f); if (bh > 0) { Ri(bx, GY - bh, 5, bh, LEAF); Ri(bx, GY - bh, 5, 2, LEAF2); } }
  Ri(rnd(x - 10), GY - 2, 20, 2, LEAF);
}
static void drawMaskedHunter(float x, float fy, float ph) {   // a pursuer in a carved mask with a spear
  const Col SK{157, 93, 57}, DARKc{38, 29, 26};
  const uint16_t OCHRE = rgb(232, 169, 56), RED = rgb(192, 48, 37), TQ = rgb(43, 189, 167), DARK = rgb(38, 29, 26), WOOD = rgb(119, 75, 39);
  setLimbs(SK, DARKc, SK, SK);
  const float hx = x, p = ph;
  const Leg l0 = runLeg(p, hx, 0), l1 = runLeg(p + 0.5f, hx, 0);
  const float hy = fy - fmaxf(l0.ay, l1.ay) - 1;
  const Leg back = runLeg(p + 0.5f, hx, hy), front = runLeg(p, hx, hy);
  const float sx = hx + 2, sy = hy - TORSO_L + 1, pump = sinf(2 * (float)M_PI * p);
  drawLeg(back, hx, hy, true);
  const Pt grip = armTo(sx + 1, sy + 1, sx + 7 + pump, sy - 3 + pump * 2, true);
  seg(grip.x - 7, grip.y + 8, grip.x + 13, grip.y - 11, WOOD, 1); R(grip.x + 12, grip.y - 14, 4, 4, OCHRE); R(grip.x + 13, grip.y - 12, 2, 2, TQ);
  seg(hx, hy, sx, sy, OCHRE, 5); R(hx - 3, hy, 7, 3, RED); R(hx - 1, hy + 2, 3, 3, TQ);
  R(sx - 4, sy - 2, 9, 2, TQ); R(sx - 5, sy - 6, 10, 2, RED);   // mask silhouette and painted crown
  R(sx - 4, sy - 5, 9, 8, OCHRE); R(sx - 4, sy - 5, 2, 8, RED); R(sx + 3, sy - 5, 2, 8, RED);
  R(sx - 2, sy - 2, 2, 1, DARK); R(sx + 2, sy - 2, 2, 1, DARK); R(sx, sy, 2, 2, TQ); R(sx - 1, sy + 3, 4, 1, DARK);
  R(sx - 6, sy - 3, 2, 3, OCHRE); R(sx + 5, sy - 3, 2, 3, OCHRE);   // broad mask and side tabs
  drawLeg(front, hx, hy, false);
  armTo(sx - 1, sy + 2, sx + 3 - pump, sy + 5 - pump * 2, false);
  resetLimbs();
}
static void drawJungleHunter(const Hunter& h, const Beach& b) {   // appears ahead (facing back) or behind, then loses the faster, leaping hero
  const int x = rnd(h.front ? 255 - 64 * h.t : -20 + 45 * fminf(h.t, 2.2f) - 50 * fmaxf(0.0f, h.t - 2.2f));
  if (x < -25 || x > W + 25) return;
  if (h.front) mirrorOn(x);
  drawMaskedHunter((float)x, (float)(GY - ghr(b, b.scroll + x - HX)), HC.ph + h.t * 1.4f);
  mirrorOff();
}
static void drawBat(const Bat& b, const AudioIn& A, uint16_t col = 0) {
  const int x = rnd(b.x), y = rnd(b.y), f = (int)floorf(b.t / (A.T / 4)) & 1; const uint16_t c = col ? col : rgb(90, 50, 120), GOLD = rgb(255, 210, 60);
  Ri(x - 1, y, 4, 3, c); Ri(x, y, 1, 1, GOLD); Ri(x + 2, y, 1, 1, GOLD); Ri(x - 1, y - 2, 1, 2, c); Ri(x + 2, y - 2, 1, 2, c);
  if (f) { Ri(x - 5, y - 2, 4, 1, c); Ri(x - 7, y - 3, 2, 1, c); Ri(x + 3, y - 2, 4, 1, c); Ri(x + 7, y - 3, 2, 1, c); }
  else { Ri(x - 5, y + 2, 4, 1, c); Ri(x - 7, y + 3, 2, 1, c); Ri(x + 3, y + 2, 4, 1, c); Ri(x + 7, y + 3, 2, 1, c); }
}


// ---- Stage 3: the forest (the web demo's "Forest 2": hills). The ground is the volume; tigers, bears and logs come along it.
static void drawTiger(float x, float y, double now, float tt) {   // a tiger facing the hero: it paces in place, tail swishing; as he comes it rears its head and roars
  const uint16_t TG = rgb(226, 124, 28), TGD = rgb(24, 16, 10), PALE = rgb(240, 226, 190), RED = rgb(170, 30, 44), GOLD = rgb(255, 210, 60);
  const bool roar = tt < 0.9f && tt > -0.25f; const int st = rnd(tsin(now * 5) * 1.5f); const float tw = tsin(now * 4);
  R(x - 9, y - 12, 21, 7, TG); for (int i = 0; i < 4; i++) R(x - 6 + i * 5, y - 12, 2, 7, TGD); R(x - 9, y - 6, 21, 1, PALE);   // body, stripes, pale belly
  R(x - 9 + st, y - 5, 3, 5, TG); R(x - 5 - st, y - 5, 3, 5, TG); R(x + 6 - st, y - 5, 3, 5, TG); R(x + 10 + st, y - 5, 3, 5, TG);   // legs padding
  seg(x + 12, y - 11, x + 17, y - 11 - 3 * tw, TG, 2); seg(x + 17, y - 11 - 3 * tw, x + 18, y - 17 - 3 * tw, TG, 2); R(x + 17, y - 14 - 3 * tw, 2, 1, TGD);   // the tail
  const float hy = y - (roar ? 18 : 15);   // the head lifts to roar
  R(x - 18, hy, 9, 7, TG); R(x - 18, hy - 2, 2, 2, TG); R(x - 11, hy - 2, 2, 2, TG); R(x - 15, hy + 2, 2, 1, GOLD); R(x - 12, hy, 1, 3, TGD); R(x - 16, hy, 1, 2, TGD);   // head, ears, an eye, face stripes
  if (!roar) R(x - 20, hy + 4, 3, 3, PALE); else { R(x - 21, hy + 1, 4, 2, TG); R(x - 21, hy + 7, 4, 2, TG); R(x - 20, hy + 3, 4, 4, RED); R(x - 20, hy + 3, 1, 1, WHITE565); R(x - 20, hy + 6, 1, 1, WHITE565); }   // closed: a pale muzzle; roaring: jaws wide, red, with fangs
}
static void drawBear(float x, float y, double now, float tt) {   // a brown bear on all fours; as he comes it rises on its hind legs, arms up, and roars
  const uint16_t FUR1 = rgb(100, 62, 34), FUR2 = rgb(64, 38, 20), TGD = rgb(24, 16, 10), RED = rgb(170, 30, 44), GOLD = rgb(255, 210, 60), MUZ = rgb(150, 110, 70);
  const float up = cl01((0.9f - tt) / 0.25f) * (1 - cl01((-0.2f - tt) / 0.25f));
  if (up < 0.5f) {
    const int st = rnd(tsin(now * 4) * 1.5f);
    R(x - 11, y - 14, 24, 9, FUR1); R(x - 11, y - 14, 24, 2, FUR2); R(x - 11 + st, y - 5, 4, 5, FUR2); R(x - 6 - st, y - 5, 4, 5, FUR1); R(x + 5 - st, y - 5, 4, 5, FUR1); R(x + 10 + st, y - 5, 4, 5, FUR2);
    R(x - 19, y - 15, 9, 8, FUR1); R(x - 19, y - 17, 3, 2, FUR2); R(x - 12, y - 17, 3, 2, FUR2); R(x - 23, y - 11, 5, 4, MUZ); R(x - 23, y - 12, 2, 1, TGD); R(x - 15, y - 13, 2, 1, GOLD);
  } else {   // rearing and attacking: up on its hind legs, leaning toward him, both arms raised high and clawing at him, the mouth wide open
    const int lz = rnd(5 * up), sw = rnd(tsin(now * 14) * 2);
    R(x - 5, y - 7, 5, 7, FUR2); R(x + 2, y - 7, 5, 7, FUR2);   // hind legs
    R(x - 7 - (lz >> 1), y - 18, 14, 12, FUR1); R(x - 7 - (lz >> 1), y - 18, 14, 2, FUR2);   // barrel body, leaning
    const float sx1 = x - 4 - lz, sx2 = x + 1 - lz, sy = y - 17;   // shoulders
    seg(sx2, sy, x - 13 - lz, y - 33 + sw, FUR2, 4); seg(sx1, sy + 1, x - 22 - lz, y - 28 - sw, FUR1, 4);   // both arms thrown up and forward, the far one darker
    const float cxs[2] = {x - 13 - lz, x - 22 - lz}, cys[2] = {y - 34 + sw, y - 29 - sw};
    for (int i = 0; i < 2; i++) { R(cxs[i] - 2, cys[i] - 2, 1, 2, WHITE565); R(cxs[i], cys[i] - 2, 1, 2, WHITE565); R(cxs[i] + 2, cys[i] - 2, 1, 2, WHITE565); }   // claws
    const float hx = x - 7 - lz, hy = y - 25; R(hx, hy, 9, 8, FUR1); R(hx, hy - 2, 3, 2, FUR2); R(hx + 6, hy - 2, 3, 2, FUR2); R(hx - 4, hy + 3, 5, 5, MUZ); R(hx - 4, hy + 5, 5, 2, RED); R(hx - 3, hy + 5, 1, 1, WHITE565); R(hx - 3, hy + 7, 1, 1, WHITE565); R(hx + 3, hy + 2, 2, 1, GOLD);
  }
}

static const uint16_t FOREST_EQ[8] = {rgb(0, 0, 255), rgb(0, 255, 255), rgb(0, 255, 0), rgb(173, 255, 41), rgb(255, 255, 0), rgb(255, 165, 0), rgb(255, 0, 0), rgb(255, 0, 255)};   // KeySpeaker's equalizer palette, low pitch to high
static const uint16_t FOREST_LEAF[6] = {rgb(7, 74, 32), rgb(12, 94, 35), rgb(24, 118, 39), rgb(45, 147, 43), rgb(83, 181, 48), rgb(144, 213, 55)};
static void drawFar(const Mtn& m, int baseY, int off, int r0, int r1, int g0, int g1, int b0, int b1) {   // the far mountains in blocks, like KeySpeaker's terrain
  for (int i = 0; i + off < 34; i++) {
    int x0 = W - m.farOff - (i + 1) * PITCH + 1; if (x0 > W || x0 < -PITCH) continue;
    int n = 1 + m.far[i + off];
    for (int k = 0; k < n; k++) { float u = k / 9.0f; R(x0, baseY - SEG * (k + 1) + 1, 6, SEG_H, rgb(rnd(r0 + r1 * u * u), rnd(g0 + g1 * u), rnd(b0 + b1 * u))); }
  }
}
static void drawForestApple(int x, int y) {   // one bright fruit at the same x as its trunk, at the explorer's shoulder height
  Ri(x - 1, y - 3, 2, 2, rgb(92, 67, 28)); Ri(x, y - 4, 3, 2, rgb(67, 167, 58));
  Ri(x - 3, y - 1, 7, 5, rgb(194, 35, 30)); Ri(x - 2, y, 5, 4, rgb(235, 47, 36)); Ri(x - 2, y, 2, 1, rgb(255, 153, 100));
}
static void drawForestBack(const Game& G, const AudioIn& A, double now) {
  const Beach& b = G.bc; const Mtn& m = G.mt; const float sc = b.scroll;
  Ri(0, 0, W, GY, rgb(8, 40, 24)); for (int y = 30; y < GY; y += 6) Ri(0, y, W, 3, rgb(14, 70, 36));
  drawFar(m, GY - 6, 10, 6, 40, 30, 40, 24, 16);   // a second, hazier range behind: the volume of ten columns earlier
  drawFar(m, GY, 0, 8, 100, 40, 90, 26, 20);       // the mountains in blocks: a picture of the last few seconds of volume
  for (int j = 0; j < 30; j++) { int n = rnd(A.eqH[j]); if (n > 8) n = 8; int x = j * PITCH + 1; for (int k = 0; k < n; k++) R(x, GY - SEG * (k + 1) + 1, 6, SEG_H, FOREST_EQ[k < 7 ? k : 7]); }   // the equalizer behind the trunks
  const uint16_t TRUNK = rgb(80, 50, 24), DIRT2 = rgb(70, 44, 20);
  const int firstTree = (int)floorf(sc * 0.25f / 47) - 1;
  for (int i = firstTree; i < firstTree + 9; i++) {
    float x = i * 47 - sc * 0.25f - 30; R(x, 0, 7, GY, TRUNK); R(x + 5, 0, 2, GY, DIRT2);
    bool taken = false; for (int a = 0; a < b.nApples; a++) if (b.apples[a] == i) taken = true;
    if (i % 2 == 0 && !taken) drawForestApple(rnd(x + 3), GY - ghr(b, sc + rnd(x) + 3 - HX) - 18);   // apple and trunk share a world position; collected fruit stays gone
  }
  for (int i = 33; i >= -1; i--) {   // fixed-height leaf columns drift in front of the trunks; 3-pixel green-to-lime bands
    int x = W - m.farOff - (i + 1) * PITCH, h = i < 0 ? m.canopyNext : m.canopy[i];
    for (int y = 0; y < h; y += 3) { int band = (y + 1) * 6 / h; if (band > 5) band = 5; Ri(x, y, PITCH, h - y < 3 ? h - y : 3, FOREST_LEAF[band]); }
    Ri(x + 2, h - 3, 4, 2, rgb(174, 229, 67));
  }
  if (1 - A.level > 0.45f) for (int i = 0; i < 3; i++) { int ex = (int)(fmod(fmod(i * 97 - sc * 0.5, 260) + 260, 260)) - 10; if (tsin(now * 2 + i * 2) > -0.7f) { Ri(ex, GY - 22 - i * 6, 2, 2, rgb(255, 210, 60)); Ri(ex + 6, GY - 22 - i * 6, 2, 2, rgb(255, 210, 60)); } }   // eyes in the bushes when it is quiet
  if (b.hills) { for (int x = 0; x < W; x += 4) { int y = GY - ghr(b, sc + x - HX); Ri(x, y + 4, 4, H - y - 4, rgb(110, 70, 30)); Ri(x, y + 14, 4, 12, rgb(70, 44, 20)); Ri(x, y, 4, 4, rgb(40, 170, 50)); } }   // the ground is the volume
  else {   // the flat forest: level ground with tufts and pits, fixed to the world
    Ri(0, GY, W, 4, rgb(40, 170, 50)); Ri(0, GY + 4, W, H - GY - 4, rgb(110, 70, 30)); Ri(0, GY + 14, W, 12, rgb(70, 44, 20));
    for (int i = -1; i < 28; i++) {
      int k = (int)floorf((sc - HX) / 9) + i; float x = HX + k * 9 - sc;
      uint32_t hh = (uint32_t)k * 2654435761u; int a = (hh >> 8) % 5, bb = (hh >> 16) % 7;
      if (a < 2) { uint16_t gc = rgb(20, 130, 40); R(x, GY - 2, 1, 2, gc); R(x + 2, GY - 3, 1, 3, gc); R(x + 4, GY - 2, 1, 2, gc); }
      if (bb == 0) R(x, GY + 7, 5, 2, rgb(140, 100, 58)); else if (bb == 3) R(x + 1, GY + 11, 3, 2, rgb(30, 18, 10)); else if (bb == 5) R(x, GY + 5, 2, 1, rgb(140, 100, 58));
    }
  }
}

// ---- The wall, drawn.
static void drawCondor(float x, float y, double now) {   // broad dark wings, hooked beak and pale neck; a low pass at the climber
  const int flap = rnd(5 * tsin(now * 15));
  seg(x - 3, y, x - 18, y - 5 + flap, rgb(56, 47, 44), 4); seg(x + 3, y, x + 18, y - 5 + flap, rgb(56, 47, 44), 4);
  for (int i = 0; i < 3; i++) { R(x - 18 + i * 3, y - 5 + flap + i, 2, 4, rgb(44, 39, 39)); R(x + 12 + i * 3, y - 3 + flap - i, 2, 4, rgb(44, 39, 39)); }
  R(x - 5, y - 2, 11, 6, rgb(69, 54, 48)); R(x - 6, y - 2, 4, 4, rgb(211, 191, 160));
  R(x - 8, y - 3, 3, 4, rgb(151, 68, 55)); R(x - 10, y, 3, 2, rgb(225, 181, 69)); R(x - 8, y - 2, 1, 1, rgb(15, 13, 13));
  R(x + 5, y + 2, 7, 2, rgb(46, 38, 36));
}
static void drawWallRock(float x, float y) {
  R(x - 5, y - 2, 10, 6, rgb(66, 62, 57)); R(x - 4, y - 5, 8, 3, rgb(84, 79, 69));
  R(x - 2, y - 6, 4, 2, rgb(119, 111, 91)); R(x - 4, y - 2, 3, 2, rgb(145, 134, 108));
  R(x + 3, y + 2, 3, 2, rgb(43, 42, 40));
}
static void wallRockPos(const Wall::Rk& r, int& px, int& py) {   // three contacts with the cliff face reverse the falling stone's horizontal course
  const float keys[5][3] = {{0, r.x, -9}, {0.55f, r.x - 24, r.y}, {1.02f, r.x + 8, r.y + 25}, {1.52f, r.x - 20, r.y + 49}, {2.3f, r.x + 12, H + 20}};
  for (int i = 1; i < 5; i++) if (r.t <= keys[i][0]) {
    const float* a = keys[i - 1]; const float* b = keys[i]; float t = (r.t - a[0]) / (b[0] - a[0]);
    px = rnd(lp(a[1], b[1], t)); py = rnd(lp(a[2], b[2], t)); return;
  }
  px = (int)(r.x + 12); py = H + 20;
}
static void drawWallRear(const Game& G, const AudioIn& A, double now) {   // after the first grips, look over his shoulder straight at the rock face
  const Wall& c = G.wl; const Mtn& m = G.mt;
  const float camera = fmaxf(0.0f, c.z - 22), groundY = 98 + camera, crestY = groundY - WALL_HEIGHT; const int bx = 120;
  R(0, 0, W, H, rgb(106, 177, 211)); R(0, 0, W, 30, rgb(69, 144, 202));
  if (crestY > 0) for (int x = 0; x < W; x += 8) { int i = (W - x) / PITCH; int h = 10 + 3 * (i < 34 ? m.far[i] : 0); R(x, crestY - h, 8, h, rgb(42, 102, 87)); }
  const float top = fmaxf(0.0f, crestY); const uint16_t stone = rgb(111, 75, 43), shadeStone = rgb(79, 58, 39);
  R(8, top, 224, H - top, stone); R(8, top, 10, H - top, shadeStone); R(222, top, 10, H - top, shadeStone);
  if (crestY > -3) { R(8, crestY, 224, 3, rgb(144, 110, 72)); R(8, crestY - 3, 224, 3, rgb(40, 170, 50)); }
  for (int i = 0; i < 100; i++) { int x = 19 + (i * 47) % 198; float y = crestY + 8 + (i * 31) % (WALL_HEIGHT + 40);
    if (y < top + 3 || y > H - 3) continue;
    R(x, y, 5 + i % 4, 2 + i % 2, i % 3 ? rgb(136, 119, 94) : rgb(92, 83, 70));
    if (i % 4 == 0) R(x + 1, y, 3, 1, rgb(174, 153, 119)); }
  auto posX = [&](int k) { return (float)(bx + c.L[k].x); }; auto posY = [&](int k) { return (float)rnd(groundY - c.L[k].h); };
  if (c.phase == WP_UP) for (int k = 0; k < 4; k++) { float px = posX(k), py = posY(k); R(px - 3, py + 1, 8, 3, shadeStone); R(px - 3, py, 8, 1, rgb(166, 143, 108)); }
  if (c.phase == WP_UP) for (const Wall::Rk& r : c.rocks) if (r.live) {
    const float lx[3] = {r.x - 24, r.x + 8, r.x - 20}, ly[3] = {r.y, r.y + 25, r.y + 49};
    for (int i = 0; i < 3; i++) if (ly[i] > top + 3 && ly[i] < H) { R(lx[i] - 6, ly[i] + 4, 13, 3, shadeStone); R(lx[i] - 4, ly[i] + 3, 9, 1, rgb(171, 148, 112)); }
    int px, py; wallRockPos(r, px, py); if (py > -8 && py < H + 6) drawWallRock((float)px, (float)py);
    static const float HITS[3] = {0.55f, 1.02f, 1.52f};
    for (float hit : HITS) if (fabsf(r.t - hit) < 0.12f) { R(px - 9, py - 5, 2, 2, rgb(174, 155, 121)); R(px + 7, py - 8, 2, 2, rgb(174, 155, 121)); }
  }
  const float hipY = (float)rnd(groundY - c.z), sy = hipY - 7;
  const float lfx = posX(LF_), lfy = posY(LF_), rfx = posX(RF_), rfy = posY(RF_), lhx = posX(LH_), lhy = posY(LH_), rhx = posX(RH_), rhy = posY(RH_);
  if (c.condor.live) drawCondor((float)rnd(c.condor.x), hipY - 27 + rnd(8 * cl01((W - c.condor.x) / 130)), now);
  legTo(bx - 2, hipY + 1, lfx, lfy, true, -1);
  legTo(bx + 2, hipY + 1, rfx, rfy, false, 1);
  R(bx - 3, hipY - 1, 6, 4, P_P); R(bx - 3, sy - 1, 6, 8, P_G);
  R(bx, sy, 1, 6, px565(shadeC(C_G, 0.7f))); R(bx - 3, hipY - 1, 6, 1, P_B);   // jacket seam identifies the rear view
  armTo(bx - 3, sy, lhx, lhy, true, -1);
  if (c.dodge > 0) { armTo(bx + 3, sy, bx + 2, sy - 7, false, 1); R(bx - 4, sy - 7, 9, 2, P_H); }
  else armTo(bx + 3, sy, rhx, rhy, false, 1);
  R(bx - 2, sy - 4, 5, 3, P_K); R(bx - 5, sy - 5, 11, 1, P_H); R(bx - 2, sy - 6, 5, 1, P_H);   // hair, hat and no face
  (void)A;
}
static void drawWall(const Game& G, const AudioIn& A, double now) {   // the landslide blocks the trail; the camera follows a long vertical climb until the ground vanishes
  const Wall& c = G.wl;
  const float camera = fmaxf(0.0f, c.z - 22), groundY = 98 + camera, crestY = groundY - WALL_HEIGHT; const int toeX = 96;
  if (c.phase == WP_TOP || (c.phase == WP_UP && c.steps >= 2)) { drawWallRear(G, A, now); return; }
  static const int WOB[6] = {0, -1, 1, 0, -2, 1};
  auto slopeY = [&](float x) { float d = fmaxf(0.0f, x - toeX); return crestY - rnd(fminf(30.0f, d * 0.25f)) + WOB[(int)floorf(d / 6) % 6]; };
  const uint16_t DIRT = rgb(110, 70, 30), DIRT2 = rgb(70, 44, 20), GRASS = rgb(40, 170, 50), TRUNK = rgb(80, 50, 24), LEAF = rgb(10, 90, 30), LEAF2 = rgb(20, 130, 40);
  R(0, 0, W, H, rgb(106, 177, 211)); R(0, 0, W, 30, rgb(69, 144, 202));
  for (int i = 0; i < 4; i++) { int x = 14 + i * 61, y = 20 + i * 6; R(x, y, 20, 3, rgb(216, 240, 237)); R(x + 5, y - 2, 12, 2, rgb(216, 240, 237)); }
  for (int x = 0; x < W; x += 4) { float y = 74 + camera + rnd(7 * sinf(x * 0.045f)); if (y < H) R(x, y, 4, H - y, rgb(42, 102, 87)); }
  if (groundY < H) { R(0, groundY, toeX, H - groundY, DIRT); R(0, groundY, toeX, 3, GRASS); R(0, groundY + 13, toeX, 7, DIRT2); }
  static const int JAG[8] = {0, -3, 2, -2, 3, -4, 1, -1};
  for (int y = (int)fmaxf(0.0f, floorf((crestY - 32) / 3) * 3); y < H; y += 3) {
    int row = (int)floorf((y - crestY) / 8), jag = JAG[((row % 8) + 8) % 8];
    int slopeStart = y < crestY ? toeX + (int)ceilf((crestY - y) * 4) : toeX - 4;
    int x0 = (y >= groundY ? toeX - 4 : toeX + jag); if (slopeStart > x0) x0 = slopeStart; if (x0 < 0) x0 = 0;
    if (x0 < W) R(x0, y, W - x0, 3, y > groundY + 12 ? DIRT2 : DIRT);
  }
  for (int x = toeX; x < W; x += 4) R(x, slopeY((float)x), 4, 2, rgb(146, 112, 77));
  for (int i = 0; c.phase != WP_EXIT && i < 90; i++) { int x = toeX + ((i * 29) % 150); float y = slopeY((float)x) + 5 + ((i * 37) % (WALL_HEIGHT + 24));
    if (y > -9 && y < H - 3) { int w = 5 + i % 5, h = 3 + i % 4; R(x, y, w, h, i % 3 ? rgb(105, 99, 86) : rgb(129, 119, 98)); R(x + 1, y, w - 3 > 2 ? w - 3 : 2, 1, rgb(170, 158, 128)); } }
  if (groundY < H + 12) for (int i = 0; i < 13; i++) { int x = toeX - 19 + i * 7; float y = groundY - 2 - ((i * 7) % 8); R(x, y, 5 + i % 3, 4 + i % 3, i % 2 ? rgb(121, 108, 86) : rgb(88, 82, 69)); }
  const float treeTop = groundY - 90; if (treeTop < H) { R(70, treeTop, 6, 90, TRUNK); R(74, treeTop, 2, 90, DIRT2);
    for (int i = 0; i < 6; i++) { int x = 52 + i * 7; float y = treeTop + 3 + i % 2 * 3; R(x, y, 16, 8, LEAF); R(x + 3, y - 3, 9, 3, LEAF2); } }
  R(115, slopeY(115) + 8, 3, 16, TRUNK); R(117, slopeY(115) + 8, 7, 2, TRUNK);
  for (int i = 0; i < 5; i++) { int x = 116 + i * 24; float y = slopeY((float)x) - 5; R(x, y, 10, 5, LEAF); R(x + 3, y - 3, 6, 3, LEAF2); }
  if (c.condor.live) { const float heroY = groundY - c.z, dive = cl01((W + 20 - c.condor.x) / (W - 92));
    drawCondor((float)rnd(c.condor.x), (float)rnd(heroY - 26 + 20 * dive - 8 * cl01((111 - c.condor.x) / 70)), now); }
  HC.slope = 0; HC.wob = 0;
  if (c.phase == WP_APPROACH || c.phase == WP_PAUSE) { drawHero((float)rnd(c.x), groundY, now, c.phase == WP_APPROACH ? HM_RUN : HM_IDLE); return; }
  if (c.phase == WP_EXIT) { drawHero((float)rnd(c.x), slopeY(c.x), now, HM_RUN); return; }
  drawLadderHero(91, 40 + c.z, 1);   // body stays outside the wall; hands meet the rock at x=96
}

static void drawVines(const Game& G, double now) {   // a sunlit break in the mountain forest: the trail ends at a ravine and hanging vines carry him across
  const float t = G.vn.t;
  const uint16_t sky[4] = {rgb(39, 113, 190), rgb(72, 153, 214), rgb(120, 190, 229), rgb(174, 216, 238)};
  const uint16_t DIRT = rgb(110, 70, 30), DIRT2 = rgb(70, 44, 20), GRASS = rgb(40, 170, 50);
  for (int i = 0; i < 4; i++) R(0, i * 24, W, 25, sky[i]);
  for (int i = 0; i < 4; i++) { int x = rnd((float)(dmod(i * 83 - t * 4, 330) - 30)), y = 13 + i * 12; R(x, y, 25, 3, WHITE565); R(x + 6, y - 3, 12, 3, WHITE565); }
  for (int x = 0; x < W; x += 4) { int far = 78 + rnd(7 * sinf((x + t * 5) * 0.025f) + 3 * sinf(x * 0.08f)); R(x, far, 4, H - far, rgb(74, 135, 135)); int nr = 96 + rnd(8 * sinf((x + t * 11) * 0.035f)); R(x, nr, 4, H - nr, rgb(35, 85, 86)); }
  R(0, 115, W, 20, rgb(17, 50, 68));
  const int edgeL = rnd(105 - t * 45), edgeR = rnd(240 - fmaxf(0.0f, t - 12) * 42);
  if (edgeL > 0) { R(0, 88, edgeL, H - 88, DIRT); R(0, 86, edgeL, 3, GRASS); R(fmaxf(0.0f, (float)(edgeL - 5)), 88, 5, H - 88, DIRT2); }
  if (t > 12) { R(edgeR, 88, W - edgeR, H - 88, DIRT); R(edgeR, 86, W - edgeR, 3, GRASS); R(edgeR, 88, 5, H - 88, DIRT2); }
  const int vines[3] = {46, 120, 194}; const int step = (int)fminf(2.0f, floorf(t / TARZAN_STEP));
  const float u = fminf(1.0f, (t - step * TARZAN_STEP) / TARZAN_STEP); const bool onVine = u < 0.76f;
  const float swing = sm01(cl01(u / 0.76f)), leap = cl01((u - 0.76f) / 0.24f);
  const float nextX = step < 2 ? vines[step + 1] - 22.0f : 235.0f;
  const float handX = onVine ? vines[step] - 22 + 44 * swing : lp(vines[step] + 22, nextX, leap);
  const float handY = onVine ? 57 - rnd(8 * sinf(swing * (float)M_PI)) : (float)rnd((step == 2 ? lp(57, 63, leap) : 57.0f) - 12 * sinf(leap * (float)M_PI));
  for (int i = 0; i < 3; i++) {
    const int ax = vines[i]; const bool grip = (onVine && i == step) || (!onVine && step < 2 && leap > 0.78f && i == step + 1);
    const float bx = grip ? handX : ax + rnd(tsin(now * 1.3 + i * 0.7) * 16), by = grip ? handY : 60.0f;
    R(ax - 8, 0, 16, 5, rgb(27, 104, 49)); seg((float)ax, 3, bx, by, rgb(32, 153, 67), 2);
    for (int y = 12; y < 51; y += 13) R((float)rnd(lp((float)ax, bx, y / by)) + (i & 1 ? 1 : -4), y, 5, 2, rgb(61, 183, 78));
  }
  HC.slope = 0; HC.wob = 0;
  if (onVine || (step < 2 && leap > 0.78f)) { SWING_K = onVine ? 2 * swing - 1 : -1; drawHero((float)rnd(handX), handY + 23, now, HM_SWING); }
  else { AIR_U = 0; drawHero((float)rnd(handX), handY + 23, now, HM_AIR); }
}

// ---- The boat and the falls, drawn.
static void rotOn(float ox, float oy, float a) { gRot = true; gRotX = (float)rnd(ox); gRotY = (float)rnd(oy); gRotCos = cosf(a); gRotSin = sinf(a); }
static void rotOff() { gRot = false; }
static void drawSmallBoat(float x, float y, float tilt) {   // wood hull and raised bow, wide enough for one explorer
  rotOn(x, y, tilt);
  R(-23, -2, 46, 3, rgb(70, 42, 27)); R(-21, 1, 42, 5, rgb(141, 90, 48));
  R(-18, 6, 36, 3, rgb(104, 59, 34)); R(-13, 9, 26, 2, rgb(70, 42, 27));
  R(-22, -5, 4, 7, rgb(116, 68, 36)); R(18, -7, 5, 9, rgb(116, 68, 36));
  R(-14, -2, 11, 2, rgb(187, 132, 75)); R(6, -2, 10, 2, rgb(187, 132, 75));
  rotOff();
}
static void drawBoatRower(float x, float y, double now) {   // seated low in the hull, rowing with a visible stroke
  const float stroke = tsin(now * 6), handX = x + 6 + rnd(stroke * 2), handY = y - 12;
  seg(x, y - 5, x + 5, y - 7, P_P, 2); seg(x + 5, y - 7, x + 9, y - 3, P_P, 2); R(x + 8, y - 4, 4, 2, P_B);
  seg(x - 1, y - 5, x + 3, y - 8, P_P_D, 2); seg(x + 3, y - 8, x + 7, y - 3, P_P_D, 2);
  drawBody(x, y - 5, 3 * DEG);   // same torso, head and fedora proportions as the running explorer
  armTo(x, y - 12, x + 3, y - 9, true);
  armTo(x + 1, y - 12, handX, handY, false);
}
static void drawBoatOar(float x, float y, double now) {
  const float stroke = tsin(now * 6);
  seg(x + 6 + rnd(stroke * 2), y - 12, x + 25 + rnd(stroke * 5), y + 9, rgb(112, 70, 40), 2);
  R(x + 23 + rnd(stroke * 5), y + 7, 8, 2, rgb(174, 125, 74));
}
static void drawBoatTumble(float x, float y, double now, float u) {   // head tips forward; arms and legs flail independently
  rotOn(x, y, lp(0.4f, 1.15f, sm01(cl01(u * 2))) + tsin(now * 15) * 0.14f);
  drawHero(0, 0, now, HM_PLUNGE);
  rotOff();
}
static void drawBoatfallRidge(const Mtn& m, int endX) {   // recorded volume columns travel left; each mountain height stays fixed after entering
  for (int x = 0; x < endX; x += 4) {
    int i = (W - m.farOff - x) / PITCH; int h = 10 + 3 * ((i >= 0 && i < 34) ? m.far[i] : 0); int w = endX - x < 4 ? endX - x : 4;
    R(x, 74 - h, w, h, rgb(31, 105, 93)); R(x, 74 - h, w, 2, rgb(91, 173, 162));
  }
}
static void drawBoatfallSurf(const AudioIn& A, int endX, double now) {   // pale blue and white EQ in the lower part of the stream
  for (int j = 0; j < 30 && j * PITCH < endX; j++) {
    int x = j * PITCH + 1, n = rnd(A.eqH[j] + 0.5f + 0.6f * tsin(now * 3 + j * 0.7)); n = n < 1 ? 1 : n > 5 ? 5 : n;
    for (int k = 0; k < n; k++) { int y = 114 - 4 * (k + 1); int w = endX - x < 6 ? endX - x : 6; R(x, y, w, 3, SURF_EQ[5 - k]); }
    if (n > 2) { int w = endX - x - 1 < 4 ? endX - x - 1 : 4; R(x + 1, 112 - 4 * n, w, 1, rgb(245, 253, 255)); }
  }
}
static void drawBoatfallCascade(float lipX, float topY, double now) {   // the stream keeps its forward motion as it pours over the edge
  const int fallSpeed = (int)floor(now * 48);
  for (int y = (int)floorf(topY); y < H; y += 2) {
    float d = y - topY; int bend = rnd(0.35f * d - 0.0006f * d * d); float x = lipX + bend;
    R(x, y, 16, 2, rgb(34, 134, 197)); R(x + 2, y, 8, 2, rgb(113, 210, 239));
    if (((y + fallSpeed) % 13 + 13) % 13 < 3) R(x + 1, y, 4, 2, rgb(238, 250, 255));
    if (((y + fallSpeed + 6) % 17 + 17) % 17 < 3) R(x + 13, y, 3, 2, rgb(214, 244, 255));
    if (d > 20 && y % 9 < 2) R(x + 18 + (int)d % 5, y - 1, 2, 2, rgb(187, 234, 249));
  }
}
static void drawBoatfall(const Game& G, const AudioIn& A, double now) {   // vines land beside a brook; a moored boat carries him over a waterfall
  const float t = G.bt.t; const BtPh phase = G.bt.phase; const Mtn& m = G.mt;
  const uint16_t WATER = rgb(26, 104, 174), DEEP = rgb(13, 65, 136), FOAM = rgb(226, 250, 255), TRUNK = rgb(80, 50, 24), LEAF = rgb(10, 90, 30), LEAF2 = rgb(20, 130, 40);
  HC.slope = 0; HC.wob = 0;
  if (phase == BT_DROP) {
    const float u = cl01((t - 11) / 2.5f); const int rush = rnd(u * 210), cliffTop = 74 - rush;
    R(0, 0, W, H, rgb(105, 184, 220));
    R(0, cliffTop > 0 ? cliffTop : 0, 105, H, rgb(110, 80, 55));
    for (int row = -1; row < 14; row++) { int y = (((row * 15 - rush) % 165) + 165) % 165 - 15; R(4, y + 4, 31, 2, rgb(151, 121, 82)); R(55, y + 10, 40, 2, rgb(74, 63, 51)); }
    if (cliffTop > 0) { R(0, cliffTop - 5, 105, 5, WATER); R(0, cliffTop - 1, 105, 2, FOAM); }
    drawBoatfallCascade(105, (float)(cliffTop - 5), now);
    if (u > 0.64f) { int poolY = rnd(137 - 29 * sm01(cl01((u - 0.64f) / 0.36f))); R(0, poolY, W, H - poolY, DEEP); R(0, poolY, W, 3, FOAM); }
    const int boatY = rnd(78 + 57 * sm01(cl01(u * 1.18f))), heroY = rnd(67 + 51 * sm01(cl01((u - 0.08f) / 0.92f)));
    drawSmallBoat((float)(117 + rnd(9 * u)), (float)boatY, u * 0.55f);
    drawBoatTumble((float)(107 + rnd(14 * u)), (float)heroY, now, u);
    return;
  }
  if (phase == BT_SPLASH) {
    const float u = cl01((t - 13.5f) / 2); const int hx = rnd(lp(100, HX, u));
    R(0, 0, W, 42, rgb(73, 150, 197)); R(0, 42, W, H - 42, WATER); R(0, 92, W, H - 92, DEEP);
    for (int i = 0; i < 30; i++) { int x = i * 11 - 8, y = 41 + rnd(tsin(i * 0.7 - now * 5) * 2); R(x, y, 8, 2, FOAM); }
    drawSmallBoat((float)(150 + rnd(75 * u)), (float)(78 + rnd(3 * tsin(now * 4))), 0.2f);
    for (int i = 0; i < 14; i++) { int x = 95 + ((i * 19) % 40) - rnd(9 * u), y = 40 + ((i * 13) % 48) - rnd(15 * u); R(x, y, 2, 3, i % 2 ? FOAM : rgb(139, 217, 248)); }
    drawHero((float)hx, 91, now, HM_SWIM); return;
  }
  const bool moving = phase == BT_CRUISE || phase == BT_LIP; const float off = moving ? (t - 2.8f) * 21 : 0;
  R(0, 0, W, H, rgb(105, 184, 220)); R(0, 0, W, 27, rgb(68, 145, 205));
  const int lipX = phase == BT_LIP ? rnd(260 - (t - 8.4f) * 62) : W + 20, bankEnd = lipX < W ? lipX : W;
  drawBoatfallRidge(m, bankEnd);
  for (int i = -1; i < 7; i++) { int x = (int)(fmodf(fmodf(i * 48 - off * 0.7f, 335) + 335, 335)) - 35; R(x, 25, 5, 51, TRUNK); R(x - 10, 21, 27, 8, LEAF); R(x - 5, 17, 18, 5, LEAF2); }
  if (lipX < W) R(lipX, 27, W - lipX, 51, rgb(105, 184, 220));   // open air beyond the last bank
  R(0, 74, bankEnd, H - 74, WATER); R(0, 93, bankEnd, H - 93, DEEP);
  for (int i = 0; i < 16; i++) { int x = (int)(fmodf(fmodf(i * 27 - off * 1.3f, 300) + 300, 300)) - 20; if (x + 8 < lipX) R(x, 75 + (i % 4) * 8, 8, 1, rgb(151, 220, 246)); }
  R(0, 115, bankEnd, H - 115, rgb(110, 80, 55)); R(0, 115, bankEnd, 3, rgb(71, 83, 55));
  drawBoatfallSurf(A, bankEnd, now);
  if (lipX < W) { R(lipX - 3, 73, 6, 6, FOAM); drawBoatfallCascade((float)lipX, 78, now); }
  if (!moving) { R(0, 87, 68, H - 87, rgb(110, 70, 30)); R(0, 85, 68, 3, rgb(40, 170, 50)); }
  const int bob = rnd(tsin(now * 4) * (1 + A.level * 2));
  if (moving) drawBoatRower(100, (float)(77 + bob), now);
  drawSmallBoat(102, (float)(77 + bob), phase == BT_LIP ? 0.04f * (t - 8.4f) : 0.0f);
  if (moving) drawBoatOar(100, (float)(77 + bob), now);
  if (phase == BT_SHORE) drawHero((float)rnd(lp(15, 70, cl01(t / 1.6f))), 85, now, HM_RUN);
  else if (phase == BT_BOARD) { const float u = sm01(cl01((t - 1.6f) / 1.2f)); AIR_U = 0; drawHero((float)rnd(lp(70, 102, u)), (float)rnd(lp(85, 76 + bob, u) - 10 * sinf((float)M_PI * u)), now, HM_AIR); }
}

// ---- The river, drawn.
static void drawRiverCroc(float x, float y, double now, float tt) {   // side profile, head facing left, swimming across the screen
  const float t = (float)fmod(now * 4 + x * 0.025, 6.283185307179586); const uint16_t G = rgb(40, 140, 60), shade = rgb(20, 90, 40), belly = rgb(98, 150, 74), teeth = rgb(244, 231, 188), W2 = rgb(40, 110, 220);
  for (int i = 0; i < 8; i++) { float tx = x + 16 + i * 3, ty = y + rnd(sinf(t - i * 0.55f) * (1 + i * 0.25f)); int hh = 5 - (int)floorf(i * 0.6f); if (hh < 1) hh = 1; int h2 = 9 - i; if (h2 < 2) h2 = 2; R(tx, ty - hh, 4, h2, G); }   // submerged tail
  R(x - 11, y - 5, 29, 10, G); R(x - 9, y + 3, 27, 3, belly);
  for (int i = 0; i < 5; i++) R(x - 4 + i * 5, y - 6, 3, 2, shade);
  const int kick = rnd(sinf(t) * 2);
  R(x + 4 + kick, y + 4, 7, 3, shade); R(x - 3 - kick, y + 4, 6, 3, shade);
  R(x - 29, y - 4, 20, 7, G); R(x - 33, y - 3, 6, 6, G);   // the long sideways snout and the eye mound
  R(x - 12, y - 9, 8, 6, G); R(x - 10, y - 8, 3, 3, teeth); R(x - 9, y - 8, 2, 2, shade);
  R(x - 31, y - 4, 2, 1, shade);
  const float gape = fmaxf(0.0f, sinf((1 - tt) * 4)) * sm01(cl01((1.2f - tt) / 0.6f));
  if (gape > 0.25f) { const int drop = rnd(gape * 6); R(x - 31, y + 3 + drop, 24, 3, belly); R(x - 28, y + 3, 21, drop + 1, rgb(145, 42, 42)); for (int i = 0; i < 7; i++) R(x - 28 + i * 3, y + 3, 1, 2, teeth); }
  else { R(x - 32, y + 3, 25, 2, belly); R(x - 30, y + 2, 18, 1, shade); }
  R(x + 2, y + 7, 13, 1, W2); R(x - 26, y + 7, 16, 1, W2);
}
static void drawPiranha(float x, float y, double now) {   // small red fish, flashing teeth and a beating forked tail
  const int kick = rnd(tsin(now * 12 + x * 0.06) * 2); const uint16_t red = rgb(217, 63, 54), dark = rgb(113, 37, 45);
  R(x - 5, y - 4, 12, 8, red); R(x - 3, y - 5, 8, 1, rgb(246, 126, 67));
  seg(x + 6, y, x + 12, y - 5 + kick, dark, 2); seg(x + 6, y, x + 12, y + 5 + kick, dark, 2);
  R(x - 8, y - 2, 4, 5, red); R(x - 7, y - 1, 1, 1, rgb(255, 238, 185));
  for (int i = 0; i < 3; i++) R(x - 7 + i * 2, y + 2, 1, 2, rgb(255, 247, 220));
  R(x - 2, y + 4, 6, 1, dark);
}
static void drawRiverSnake(float x, float y, double now) {   // a long green body cuts through the open river, head facing the swimmer
  const uint16_t body = rgb(82, 143, 70), shade = rgb(35, 86, 61);
  for (int i = 0; i < 13; i++) { float sy = y + rnd(tsin(now * 3 - i * 0.65) * 3); int w = i > 9 ? 2 : 4; R(x + i * 3, sy, w, 3, body); R(x + i * 3, sy, w, 1, shade); }
  R(x - 8, y - 3, 9, 6, body); R(x - 7, y - 4, 5, 2, shade); R(x - 6, y - 1, 1, 1, rgb(252, 224, 100));
  if (tsin(now * 6) > 0) { R(x - 11, y + 1, 3, 1, rgb(214, 82, 83)); R(x - 12, y, 1, 3, rgb(214, 82, 83)); }
}
static void drawFountain(const Game& G, double now) {   // the geyser throws him upward while the inside of the building rushes downward
  const float t = (float)(now - G.t0); const int off = rnd(t * 185);
  R(0, 0, W, H, rgb(18, 23, 38));
  for (int row = -2; row < 15; row++) { const int y = ((row * 12 + off) % 156 + 156) % 156 - 12;
    for (int x = 0; x < W; x += 24) { R(x + ((row & 1) ? 12 : 0), y, 23, 11, rgb(92, 80, 78)); R(x + ((row & 1) ? 12 : 0), y, 23, 2, rgb(133, 112, 99)); }
    if (row % 3 == 0) { R(19, y + 2, 11, 7, rgb(246, 177, 72)); R(210, y + 2, 11, 7, rgb(246, 177, 72)); } }
  R(0, 0, 16, H, rgb(58, 52, 55)); R(W - 16, 0, 16, H, rgb(58, 52, 55));
  R(76, 0, 88, H, rgb(28, 92, 158));
  for (int y = -16; y < H + 16; y += 12) { const float yy = fmodf(fmodf(y - off * 0.7f, H + 24) + H + 24, H + 24) - 12; R(83 + rnd(tsin(y + now * 5) * 5), yy, 65, 4, rgb(80, 181, 231)); R(98, yy + 4, 42, 2, rgb(215, 249, 255)); }
  if (t > 3.8f) { const int lip = rnd(-10 + (t - 3.8f) * 130); R(0, 0, W, lip > 0 ? lip : 0, rgb(110, 185, 229)); R(0, lip, W, 7, rgb(178, 142, 91)); }
  const int lift = t < 3.8f ? 0 : rnd(110 * sm01(cl01((t - 3.8f) / 1.1f)));
  HC.slope = 0; HC.wob = 0;
  drawHero((float)(PYRAMID_X + rnd(tsin(now * 11) * 3)), (float)(79 - lift), now, HM_PLUNGE);
  for (int i = 0; i < 11; i++) { const int x = 84 + i * 7, y = rnd(fmodf(i * 37 - off * 1.2f, 150)); R(x, (y + 150) % 150, 2, 3, WHITE565); }
}
static void drawPyramidBackdrop(float scroll, int rise, float fall, float move = 0) {   // the sunlit stepped wall of the pyramid under a blue sky
  R(0, 0, W, H, rgb(101, 181, 226)); R(0, 0, W, 40, rgb(61, 138, 204));
  for (int i = 0; i < 4; i++) { const int x = (int)fmodf(i * 79 - scroll * 0.04f + 300, 300) - 20, y = 17 + i * 8 - rnd(fall * 18); R(x, y, 25, 4, WHITE565); R(x + 5, y - 3, 15, 3, WHITE565); }
  for (int x = 0; x < W; x += 8) { const int top = 57 + (x / 24) * 5 + rise; if (top >= H) continue; R(x, top, 8, H - top, rgb(174, 135, 82)); R(x, top, 8, 3, rgb(239, 195, 108)); }
  for (int y = 84 + rise; y < H; y += 10) for (int x = -((rnd(move) + (y % 20 ? 0 : 8)) % 24); x < W; x += 24) if (y >= 60 + (x > 0 ? x / 24 : 0) * 5 + rise) R(x, y, 13, 1, rgb(113, 82, 55));
  for (int x = 36 - (rnd(move * 0.6f) % 60); x < W; x += 60) { const int top = 57 + (x > 0 ? x / 24 : 0) * 5 + rise; if (top + 24 < H) { R(x, top + 12, 7, 7, rgb(126, 89, 51)); R(x + 2, top + 14, 3, 3, rgb(228, 182, 98)); } }
}
static void drawSkyfall(const Game& G, double now) {   // after leaving the water jet above the screen, he drops back into view over the pyramid
  const float t = (float)(now - G.t0), u = cl01(t / 3.3f); const int rise = rnd(80 * (1 - sm01(cl01(t / 2.7f))));
  drawPyramidBackdrop(0, rise, t);
  const int feet = -24 + rnd(106 * powf(u, 1.7f));
  HC.slope = 0; HC.wob = 0; AIR_U = 0;
  if (t > 0.35f) drawHero((float)PYRAMID_X, (float)feet, now, u > 0.86f ? HM_AIR : HM_PLUNGE);
  for (int i = 0; i < 7; i++) { const int y = feet + 13 + i * 3; R(PYRAMID_X - 5 + (i * 7) % 12, y, 1, 2, (i & 1) ? WHITE565 : rgb(164, 230, 255)); }
}
static void drawPyramid(const Game& G, double now) {   // the falling hero has landed on the sunlit stepped outer wall
  const Pyramid& P = G.py; const float t = (float)(now - G.t0);
  const int runX = PYRAMID_X + rnd(24 * sm01(cl01(t / 0.9f)));
  drawPyramidBackdrop(0, 0, 0, P.scroll);
  if (t < 0.65f) for (int i = 0; i < 9; i++) { const float d = t * 20, x = PYRAMID_X + (i - 4) * (2 + d * 0.28f), y = 81 - sinf(i * 1.8f) * (4 + d) + d * 0.7f; R(rnd(x), rnd(y), 2, 2, (i & 1) ? WHITE565 : rgb(164, 230, 255)); }
  HC.slope = 0; HC.wob = 0;
  drawHero((float)runX, (float)(57 + (runX / 24) * 5), now, P.duck ? HM_ROCKCOVER : P.look ? HM_RUNLOOK : HM_RUN);
  for (const Pyramid::Rock& rk : P.rocks) if (rk.live) {
    const int x = rnd(rk.x), ground = 57 + (x > 0 ? x / 24 : 0) * 5; const float bounce = 22 * fabsf(sinf((rk.x + 12) * (float)M_PI / 62));
    const int y = rnd(ground - 10 - bounce);
    R(x - 8, ground + 1, 16, 1, rgb(102, 74, 53));
    for (int dy = -10; dy <= 10; dy++) { const int half = (int)floorf(sqrtf((float)(100 - dy * dy))); R(x - half, y + dy, 2 * half + 1, 1, rgb(61, 58, 57)); if (half > 1) R(x - half + 1, y + dy, 2 * half - 1, 1, dy > 5 ? rgb(103, 99, 93) : rgb(137, 132, 122)); }
    R(x - 5, y - 5, 4, 2, rgb(211, 204, 184)); R(x + 2, y - 3, 3, 2, rgb(91, 88, 83)); R(x - 1, y + 4, 4, 2, rgb(105, 100, 93));
    if (bounce < 3) for (int i = 0; i < 3; i++) R(x - 10 + i * 8, ground - 2 - i % 2, 2, 1, rgb(201, 164, 107));
  }
}
static void drawWaterSnake(int x, int y, double now) {   // a small snake undulates in place near the waterway wall
  const uint16_t body = rgb(104, 154, 78), back = rgb(53, 104, 65);
  for (int i = 0; i < 10; i++) { const int sy = y + rnd(tsin(now * 2.4 - i * 0.7) * 2), w = i > 7 ? 2 : 4; R(x + i * 3, sy, w, 2, body); R(x + i * 3, sy, w, 1, back); }
  R(x - 6, y - 2, 7, 5, body); R(x - 5, y - 3, 5, 1, back); R(x - 4, y - 1, 1, 1, rgb(240, 220, 116)); if (tsin(now * 5) > 0) R(x - 8, y + 1, 2, 1, rgb(202, 76, 72));
}
static void drawRiver(const Game& G, const AudioIn& A, double now) {
  const River& r = G.rv; const Mtn& m = G.mt;
  const uint16_t WATER = rgb(20, 70, 170), WATER2 = rgb(40, 110, 220), LEAFC = rgb(10, 90, 30);
  const bool under = G.scene == SC_WATERWAY; const float entry = under && r.entryLive ? r.entryT : 1; const int lift = under ? rnd(62 * (1 - sm01(cl01(entry / 1.1f)))) : 0;
  if (lift) R(0, 0, W, H, rgb(8, 7, 10));   // the same waterline starts where the treasury fall ended, then the camera follows him down
  gOffY = lift;
  if (under) { R(0, 0, W, 40, rgb(22, 16, 28)); const int gapX = rnd(HX - r.scroll * 0.75f);
    for (int x = 0; x < W; x += 4) { if (x + 4 > gapX - 9 && x < gapX + 9) continue; R(x, 0, 4, 8, rgb(212, 124, 42)); R(x, 8, 4, 4, rgb(150, 84, 26)); R(x, 12, 4, 1, rgb(96, 56, 26)); if (x % 24 == 0) R(x, 1, 1, 7, rgb(120, 66, 28)); }
    R(0, 19, W, 1, rgb(66, 48, 43)); for (float x = -fmodf(r.scroll * 0.35f, 24); x < W; x += 24) R(x, 20, 1, 13, rgb(58, 44, 44)); }   // a brick-lined waterway beneath the treasury, not an open river
  else {
  R(0, 0, W, 40, rgb(14, 70, 36));
  for (int i = 33; i >= 0; i--) { int x = W - m.farOff - (i + 1) * PITCH, h = 7 + 3 * m.far[i]; R(x, 40 - h, PITCH, h, rgb(8, 40, 24)); R(x, 40 - h, PITCH, 2, LEAFC); }   // the dark-green ridge keeps each volume-derived height as it scrolls left
  for (int j = 0; j < 30; j++) { int n = rnd(A.eqH[j]); if (n > 6) n = 6; int x = j * PITCH + 1; for (int k = 0; k < n; k++) R(x, 40 - SEG * (k + 1) + 1, 6, SEG_H, SURF_EQ[5 - (k < 5 ? k : 5)]); }   // pale-blue-to-white wave blocks sit behind the waterline
  }
  R(0, 40, W, H - 40, WATER);
  for (int x = 0; x < W; x += 2) { int wave = rnd(A.level * (4 * sinf(x * 0.11f - (float)fmod(now * 3, 6.283185307179586)) + 1.5f * sinf(x * 0.05f + (float)fmod(now * 1.7, 6.283185307179586)))), y = 34 + wave; R(x, y, 2, 40 - y, WATER); R(x, y, 2, 1, WATER2); }
  R(0, 90, W, H - 90, rgb(14, 50, 140)); R(0, 116, W, H - 116, rgb(8, 24, 80));
  static const uint16_t WEED[4] = {rgb(20, 94, 48), rgb(40, 155, 70), rgb(88, 198, 92), rgb(142, 214, 91)};
  for (int j = 0; j < 30; j++) { int n = rnd(A.eqH[j]); if (n > 10) n = 10; int x = j * PITCH + 1; for (int k = 0; k < n; k++) { int sway = rnd(tsin(now * 2.1 + j * 0.65 + k * 0.23) * fminf(3.0f, k * 0.4f)); R(x + sway, H - 4 * (k + 1) + 1, 4, 4, WEED[j % 4]); } }   // green aquatic plants sway with the water
  for (int i = 0; i < 18; i++) { int x = under ? i * 53 % W + rnd(tsin(now * 0.9 + i) * 2) : (int)fmodf(fmodf(i * 53 - r.scroll * 0.8f, 270) + 270, 270) - 15, y = 40 + (i * 29) % 90; R(x, y, 8, 1, WATER2); }
  if (under) drawWaterSnake(150 + rnd(tsin(now * 0.7) * 10), 64 + rnd(tsin(now * 1.1) * 2), now);
  if (!under) for (const RvObs& o : r.obs) if (o.live) {
    const float x = HX + o.wx - r.scroll; if (x > W + 24) continue;
    const float y = LANES[o.lane] + (o.sine ? rnd(tsin(now * 2.6 + o.wx * 0.02) * 13) : 0);   // a sine swimmer weaves up and down within its lane
    if (o.kind == RV_CROC) drawRiverCroc(x, y, now, rvTtl(r, o));
    else if (o.kind == RV_PIRANHA) drawPiranha(x, y, now);
    else if (o.kind == RV_SNAKE) drawRiverSnake(x, y, now);
    else { R(x - 12, y - 4, 24, 7, rgb(130, 80, 30)); R(x - 12, y - 4, 2, 7, rgb(190, 130, 60)); }
  }
  if (under) { const int sx = rnd(HX + WATERWAY_LEN - r.scroll); if (sx < W + 20) {   // the water source at the end of the channel
    R(sx - 15, 13, 34, H - 13, rgb(52, 48, 49)); R(sx - 11, 15, 26, H - 15, rgb(24, 45, 78)); for (int y = 18; y < H; y += 8) R(sx - 9 + rnd(tsin(now * 6 + y) * 2), y, 22, 3, rgb(96, 191, 235)); R(sx - 14, 10, 32, 4, rgb(145, 105, 63)); } }
  for (const RvBubble& b : r.bub) if (b.live) R(b.x, b.y, 2, 2, WATER2);
  HC.slope = 0; HC.wob = 0;
  if (under && r.exitLive) { const float u = sm01(cl01(r.exitT / 1.7f)); R(HX + 22, 30, 27, 104, rgb(37, 103, 163)); for (int y = 36; y < 132; y += 9) R(HX + 26 + rnd(tsin(now * 7 + y) * 3), y, 19, 3, rgb(144, 225, 255)); drawHero((float)(HX + rnd(28 * u)), (float)(rnd(r.laneY) + 8 - rnd(11 * u)), now, HM_SWIM); }
  else if (under && r.entryLive) { const float u = sm01(cl01(entry / 1.1f)); const int hy = rnd(lp(98.0f - lift, r.laneY + 8, u)); if (entry < 0.45f) R(HX - 8, 32, 16, 3, WHITE565); drawHero((float)HX, (float)hy, now, entry < 0.25f ? HM_PLUNGE : HM_SWIM); }
  else drawHero(HX, (float)(rnd(r.laneY) + 8), now, HM_SWIM);
  gOffY = 0;
  if (under) fillA(0, 0, W, H, 0, 0, 30, 0.34f);   // the dim light under the treasury
}

static void drawCrocHead(float x, int sy, double now, float tt) {   // a pond crocodile: just a head, in profile, facing the hero; eyes lurk at the surface; as he comes the head rises, the jaws gape, then it sinks again
  const uint16_t CROC = rgb(40, 140, 60), CROCD = rgb(20, 90, 40), RED = rgb(170, 30, 44), GOLD = rgb(255, 210, 60), W2 = rgb(40, 110, 220);
  const float hx = x + 4, up = cl01((1.5f - tt) / 0.55f) * (1 - cl01((-0.5f - tt) / 0.4f)); const bool open_ = tt < 0.7f && tt > -0.3f; (void)open_;
  const float y = (float)(sy + rnd((1 - up) * 16));
  gClipB = sy + 1;   // the water hides whatever is below the surface
  if (up < 0.15f) { R(hx + 3, sy - 1, 2, 1, GOLD); R(hx - 9, sy - 1, 2, 1, GOLD); }
  else {
    R(hx - 2, y - 4, 9, 5, CROC); R(hx + 2, y - 8, 4, 4, CROC); R(hx + 3, y - 7, 2, 2, GOLD); R(hx + 8, y - 1, 8, 2, CROCD);
    const float ang = 90 * sm01(cl01((1.0f - tt) / 0.55f)) * (1 - sm01(cl01((-0.15f - tt) / 0.45f))); const float a = rnd(ang / 22.5f) * 22.5f * DEG;   // the jaws open in steps
    if (a == 0) { R(hx - 22, y - 4, 21, 5, CROC); R(hx - 22, y - 1, 21, 1, CROCD); R(hx - 23, y - 5, 2, 1, CROC); }
    else {
      R(hx - 24, y - 1, 25, 5, CROC); R(hx - 24, y - 1, 25, 2, RED); for (int i = 0; i < 12; i++) R(hx - 24 + i * 2, y - 2, 1, 1, WHITE565);
      const float px = hx - 1, py = y - 1, L = 23, dx = -cosf(a), dy = -sinf(a), nx = -sinf(a), ny = cosf(a);
      seg(px, py, px + dx * L, py + dy * L, CROC, 5);
      for (int i = 1; i < 11; i++) { float f = i / 11.0f * L, qx = px + dx * f + nx * 3, qy = py + dy * f + ny * 3; R(qx - 1, qy - 1, 2, 2, RED); R(qx - 2, qy - 2, 1, 1, WHITE565); }
    }
  }
  gClipB = H;
  if (tt < 1.8f && tt > -0.9f) { int w = 6 + rnd(8 * (1 - fminf(1.0f, fabsf(tt - 0.3f) / 1.2f))); R(hx - 14 - (w >> 1), sy + 1, w, 1, W2); R(hx + 6, sy + 1, w, 1, W2); }   // ripples
}
static void drawPond(const Obs& o, float x, double now, float tt) {
  const uint16_t WATER = rgb(20, 70, 170), WATER2 = rgb(40, 110, 220);
  const float hw = o.hw;
  R(x - hw, GY, hw * 2, 12, WATER); R(x - hw, GY, hw * 2, 2, WATER2);
  if (o.wide) { drawCrocHead(x - 14, GY, now, tt); drawCrocHead(x + 16, GY, now, tt + 0.12f); } else if (o.hasCroc) drawCrocHead(x + 2, GY, now, tt);
}

static void drawIdol(float x, float by, double now, bool big) {   // an Aztec treasure standing on y = by: a gold mask with turquoise eyes and ear spools under a fan of quetzal feathers; big: held overhead, with rays that pulse on the beat
  const uint16_t G1 = rgb(252, 208, 64), G2 = rgb(176, 124, 24), TQ = rgb(40, 200, 190), GR = rgb(30, 170, 80), RD = rgb(214, 50, 40), DK = rgb(60, 36, 8);
  if (big) { const int r = 8 + rnd(HERO_PULSE * 4); PA(x - r, by - 9 - r, r * 2, r * 2, 255, 220, 90, 0.18f); for (int i = 0; i < 8; i++) { float a = i * (float)M_PI / 4 + (float)fmod(now * 1.5, 6.283185307179586); R(x + cosf(a) * (r + 3), by - 9 + sinf(a) * (r + 3), 1, 1, i & 1 ? WHITE565 : G1); } }
  R(x - 5, by - 12, 2, 4, GR); R(x - 3, by - 14, 2, 6, TQ); R(x - 1, by - 15, 3, 7, RD); R(x + 2, by - 14, 2, 6, TQ); R(x + 4, by - 12, 2, 4, GR);   // the fan of quetzal feathers
  R(x - 3, by - 9, 7, 8, G1); R(x - 3, by - 9, 7, 1, G2); R(x - 5, by - 7, 2, 3, TQ); R(x + 4, by - 7, 2, 3, TQ);   // the gold face and the turquoise ear spools
  R(x - 2, by - 7, 2, 2, TQ); R(x + 1, by - 7, 2, 2, TQ); R(x - 2, by - 7, 1, 1, DK); R(x + 2, by - 7, 1, 1, DK); R(x, by - 5, 1, 2, G2); R(x - 2, by - 3, 5, 1, DK); R(x - 1, by - 3, 1, 1, WHITE565); R(x + 1, by - 3, 1, 1, WHITE565); R(x - 2, by - 1, 5, 1, G2);
}
static void drawRelic(float x, double now, bool taken, bool clue) {   // a person-high stepped stone altar; the golden mask rests on top until taken
  const uint16_t S1 = rgb(120, 112, 100), S2 = rgb(160, 150, 134), TQ = rgb(40, 200, 190), DK = rgb(70, 64, 56);
  static const int ST[5][3] = {{30, 4, 4}, {26, 5, 9}, {22, 6, 15}, {18, 7, 22}, {14, 4, 26}};
  for (const auto& e : ST) { R(x - e[0] / 2.0f, GY - e[2], e[0], e[1], S1); R(x - e[0] / 2.0f, GY - e[2], e[0], 1, S2); }
  for (int i = 0; i < 6; i++) { R(x - 13 + i * 5, GY - 3, 3, 1, DK); R(x - 12 + i * 5, GY - 2, 1, 2, DK); }   // the fret band
  R(x - 2, GY - 14, 4, 3, TQ);   // a turquoise inlay
  if (clue && ((int)floor(now * 8) & 1)) { R(x - 2, GY - 17, 4, 2, rgb(180, 250, 245)); R(x + 11, GY - 12, 2, 2, rgb(180, 250, 245)); }   // the loose stone catches his eye
  if (!taken) { drawIdol(x, GY - 26, now, false); if ((int)floor(now * 3) & 1) { R(x + 5, GY - 38, 1, 3, WHITE565); R(x + 4, GY - 37, 3, 1, WHITE565); } }
}
static void drawHole(float x) {   // a hole in the road: a black shaft with a ragged lip, roots hanging in it
  R(x - 12, GY, 24, H - GY, rgb(6, 4, 3)); R(x - 12, GY, 24, 3, rgb(34, 22, 12)); R(x - 14, GY, 2, 8, rgb(90, 62, 32)); R(x + 12, GY, 2, 8, rgb(90, 62, 32)); R(x - 14, GY - 1, 4, 2, rgb(40, 170, 50)); R(x + 10, GY - 1, 4, 2, rgb(40, 170, 50));
  R(x - 8, GY + 3, 1, 8, rgb(96, 64, 30)); R(x + 5, GY + 3, 1, 6, rgb(96, 64, 30)); R(x - 3, GY + 3, 1, 4, rgb(96, 64, 30));
}

// ---- The tunnel, drawn.
static const uint16_t ROCK0 = rgb(26, 20, 24), ROCK1 = rgb(48, 36, 36), ROCK2 = rgb(78, 60, 52), ROCK3 = rgb(120, 94, 70), DRIPC = rgb(130, 190, 240);
static void spike(float cx, float y0, int w, int h, uint16_t col, bool down) { for (int i = 0; i < h; i++) { int ww = rnd(w * (1 - i / (float)h)); if (ww < 1) ww = 1; R(cx - (ww >> 1), down ? y0 + i : y0 - i, ww, 1, col); } }
static int trebleFlame(const Game& G, double now, float seed) { return rnd(0.8f * tsin(now * 4.5 + seed) + G.treble * 2.2f * tsin(now * 9.5 + seed)); }
static void drawScorpion(float x, double now, float tt) {   // a scorpion facing him: claws forward, legs scuttling, the tail curled over its back; it strikes with the tail as he comes
  const uint16_t B = rgb(140, 84, 28), BD = rgb(96, 56, 18), RD = rgb(210, 50, 40); const bool near = tt < 0.7f && tt > -0.2f; const int k = (int)floor(now * 10);
  R(x - 6, GY - 5, 11, 4, B); R(x - 6, GY - 5, 11, 1, BD); R(x + 3, GY - 6, 3, 5, B);
  for (int i = 0; i < 4; i++) R(x - 5 + i * 3, GY - 1, 1, 1 + ((k + i) & 1), BD);   // legs
  seg(x - 6, GY - 4, x - 10, GY - 6 - (near ? 2 : 0), B, 1); R(x - 13, GY - 8 - (near ? 2 : 0), 3, 3, BD); R(x - 13, GY - 4, 3, 3, BD); if (near) R(x - 12, GY - 6, 1, 2, rgb(0, 0, 0));   // claws, open when it is close
  if (!near) { seg(x + 6, GY - 5, x + 9, GY - 9, B, 2); seg(x + 9, GY - 9, x + 7, GY - 13, B, 2); seg(x + 7, GY - 13, x + 3, GY - 13, B, 1); R(x + 2, GY - 12, 2, 2, RD); }
  else { seg(x + 6, GY - 5, x + 9, GY - 10, B, 2); seg(x + 9, GY - 10, x + 3, GY - 16, B, 2); seg(x + 3, GY - 16, x - 6, GY - 13, B, 1); R(x - 8, GY - 13, 2, 2, RD); }   // the stinger lunges forward
}
static void drawSnake(float x, double now, float tt) {   // a cobra: coiled on the floor, the head and hood rising as he comes, hissing, the tongue flickering
  const uint16_t G1 = rgb(96, 130, 40), G2 = rgb(58, 86, 24), YL = rgb(220, 196, 90), RD2 = rgb(210, 50, 40), BLK = rgb(0, 0, 0);
  const float up = cl01((1.0f - tt) / 0.3f) * (1 - cl01((-0.2f - tt) / 0.3f)); const int h = 4 + rnd(13 * up);
  R(x - 8, GY - 3, 16, 3, G1); R(x - 7, GY - 3, 14, 1, YL); R(x - 6, GY - 6, 12, 3, G2); R(x - 4, GY - 8, 8, 2, G1);   // the coils
  R(x - 7, GY - 6 - h, 3, h, G1);   // the neck
  if (up > 0.3f) { R(x - 10, GY - 8 - h, 9, 5, G2); R(x - 9, GY - 7 - h, 7, 2, YL); R(x - 12, GY - 6 - h, 3, 3, G1); R(x - 11, GY - 6 - h, 1, 1, BLK); R(x - 13, GY - 4 - h, 2, 1, RD2); if ((int)floor(now * 8) & 1) R(x - 15, GY - 5 - h, 2, 1, RD2); R(x - 12, GY - 3 - h, 1, 2, WHITE565); }   // the hood spreads, the mouth and fangs, a flicking tongue
  else { R(x - 10, GY - 8 - h, 6, 4, G1); R(x - 9, GY - 7 - h, 1, 1, BLK); }
}
static void drawCaveObs(const Cave& c, const CvObs& o, float x, double now) {
  const float tt = cvTtl(c, o);
  if (o.kind == CV_ROCK) { R(x - 7, GY - 6, 14, 6, rgb(96, 88, 90)); R(x - 5, GY - 9, 10, 3, rgb(96, 88, 90)); R(x - 3, GY - 11, 6, 2, rgb(120, 112, 112)); R(x + 3, GY - 6, 4, 6, rgb(70, 62, 66)); }
  else if (o.kind == CV_SCORP) drawScorpion(x, now, tt);
  else if (o.kind == CV_SNAKE) drawSnake(x, now, tt);
  else if (o.kind == CV_LOW) { R(x - 8, 0, 16, GY - 19, ROCK2); R(x - 6, GY - 19, 12, 2, ROCK2); R(x - 8, 0, 2, GY - 19, ROCK3); R(x - 3, GY - 17, 6, 2, ROCK1); }
  else { const float f = cl01((0.8f - tt) / 0.25f); const int y0 = cey(c, x), shake = (tt < 1.2f && tt > 0.8f) ? (((int)floor(now * 30) & 1) * 2 - 1) : 0;   // a stalactite shakes, then drops
    if (f < 1) { float tipY = y0 + 12 + f * (GY - y0 - 12); spike(x + shake, tipY - 12, 8, 12, rgb(120, 100, 86), true); }
    else spike(x, GY, 8, 11, rgb(120, 100, 86), false); }
}
static bool nearGap(const Cave& c, float wx, float r);
static void drawCaveBg(const Game& G, double now, int depth) {   // the cave itself: an old brick wall (bluer and darker the deeper it is), stalagmites, the ceiling and the floor in blocks
  const Cave& c = G.cv; const float sc = c.scroll;
  R(0, 0, W, H, ROCK0);
  { const float ox = fmodf(fmodf(-sc * 0.3f, 16) + 16, 16) - 16;   // brick, with torches in brackets
    for (int row = 0; row * 8 < GY + 8; row++) for (float x = ox - (row & 1) * 8; x < W; x += 16) R(x, row * 8, 15, 7, (((int)x + row * 3) & 8) ? (depth ? rgb(40, 46, 68) : rgb(54, 40, 36)) : (depth ? rgb(32, 38, 58) : rgb(46, 34, 30)));
    for (int k = (int)floorf(sc * 0.5f / 130) - 1; k < (int)floorf(sc * 0.5f / 130) + 3; k++) { int x = rnd(k * 130 - sc * 0.5f + 40), fl = trebleFlame(G, now, (float)k); PA(x - 16, GY - 80, 32, 40, 255, 170, 60, 0.10f); R(x, GY - 58, 2, 6, rgb(80, 50, 24)); R(x - 1, GY - 65 - fl, 4, 7 + fl, rgb(255, 210, 60)); R(x, GY - 63, 2, 3, rgb(255, 246, 192)); }
  }
  for (int k = (int)floorf(sc * 0.4f / 37) - 1; k < (int)floorf(sc * 0.4f / 37) + 8; k++) { float x = k * 37 - sc * 0.4f + 10; spike(x, GY, 14, 12 + ((k * 7 & 3) * 5), ROCK1, false); }   // far stalagmites
  for (int k = (int)floorf(sc / PITCH) - 1; k < (int)floorf(sc / PITCH) + 32; k++) { float x0 = k * PITCH - sc + 1; int n = rnd(cey(c, k * PITCH - sc + 3) / (float)SEG); if (n < 1) n = 1; for (int j = 0; j < n; j++) R(x0, j * SEG + 1, 6, SEG_H, j == n - 1 ? ROCK3 : (j & 1 ? ROCK2 : rgb(66, 50, 46))); }   // the ceiling in blocks
  for (int k = (int)floorf(sc / 23) - 1; k < (int)floorf(sc / 23) + 12; k++) { float x = k * 23 - sc + 8; spike(x, (float)(cey(c, x) - 1), 6, 5 + (((k * 7) % 5 + 5) % 5) * 2, ROCK2, true); }   // stalactites that stay up
  R(0, GY, W, H - GY, ROCK0);   // the floor in blocks as well, fixed to the world: the columns rise and fall in slopes
  for (int k = (int)floorf(sc / PITCH) - 1; k < (int)floorf(sc / PITCH) + 32; k++) { float x0 = k * PITCH - sc + 1; int top = GY - rnd(labH(c, sc + x0 + 3 - HX)); if (nearGap(c, sc + x0 + 3 - HX, 22)) continue; for (int j = 0, y = top + 1; y < H; j++, y += SEG) R(x0, y, 6, SEG_H, j == 0 ? ROCK3 : (((j + k) & 1) ? ROCK2 : rgb(66, 50, 46))); }
  for (int gi = 0; gi < c.nGaps; gi++) { const int gx = rnd(HX + c.gaps[gi] - sc - 22); if (gx < -50 || gx > W) continue; const int top = GY - rnd(labH(c, c.gaps[gi] - 60)); R(gx, top - 1, 44, H - top + 1, rgb(3, 2, 4)); R(gx - 1, top, 1, 3, ROCK3); R(gx + 44, top, 1, 3, ROCK3); }   // the broken place: a black drop
  for (int i = -1; i < 24; i++) { int k = (int)floorf(sc / 11) + i; float x = k * 11 - sc; int b = (int)((((uint32_t)k * 2654435761u) >> 16) % 6); if (b < 3) R(x, GY + 5 + b * 6, 4 + b, 2, b == 0 ? ROCK3 : ROCK1); }   // chips and cracks in the floor
}
static void drawLabFeature(const Cave& c, float xl, int kind, double now) {   // a hole in the floor (a drop), a shaft with a ladder (up a level), or the golden doorway
  if (xl < -30 || xl > W + 30) return;
  if (kind == MV_GATE) {
    R(xl - 15, GY - 58, 30, 58, rgb(40, 32, 30)); R(xl - 10, GY - 54, 20, 54, rgb(255, 214, 110)); R(xl - 6, GY - 54, 12, 54, rgb(255, 240, 170)); R(xl - 2, GY - 54, 4, 54, rgb(255, 252, 220));
    R(xl - 17, GY - 60, 6, 60, ROCK3); R(xl + 11, GY - 60, 6, 60, ROCK3); R(xl - 17, GY - 63, 34, 6, ROCK3); R(xl - 14, GY - 61, 28, 2, ROCK1);
    for (int i = 0; i < 4; i++) R(xl - 12 + i * 7, GY - 62, 4, 2, rgb(206, 150, 60)); R(xl - 3, GY - 70, 6, 5, rgb(206, 150, 60)); R(xl - 1, GY - 72, 2, 2, rgb(206, 150, 60)); R(xl - 6, GY - 68, 1, 1, rgb(40, 190, 180)); R(xl + 6, GY - 68, 1, 1, rgb(40, 190, 180));
    PA(xl - 70, GY - 52, 56, 52, 255, 220, 120, 0.10f); return; }
  if (kind == MV_DOWN) { R(xl - 12, GY, 24, H - GY, rgb(6, 4, 3)); R(xl - 12, GY, 24, 2, rgb(90, 72, 60)); R(xl - 14, GY - 1, 2, 3, ROCK3); R(xl + 12, GY - 1, 2, 3, ROCK3); return; }
  const int cb = cey(c, xl) + 3;
  R(xl - 8, -22, 16, 22 + cb, rgb(60, 50, 46)); R(xl - 5, -22, 10, 22 + cb, rgb(84, 70, 60));
  const uint16_t LOG = rgb(130, 80, 30), LOGEND = rgb(190, 130, 60);
  R(xl - 1, -24, 1, 24 + GY, LOG); R(xl + 3, -24, 1, 24 + GY, LOG); for (int yy = GY; yy > -24; yy -= 6) R(xl - 1, yy, 5, 1, LOGEND);
  (void)now;
}


static void drawWarrior(float x, float fy, double now, int i, float ph, float pulse, bool stand = false) {   // an Aztec temple guardian, built like the hero: a feathered headdress, a cotton tunic with a sash and loincloth, gold bands, a round shield on the near arm and a spear in the far hand
  const Col SKc{150, 92, 58}; const uint16_t TN = rgb(236, 150, 60), TQ = rgb(40, 200, 190), RD = rgb(214, 50, 40), GR = rgb(30, 170, 80), G1 = rgb(252, 208, 64), G1D = rgb(151, 125, 38), WD = rgb(120, 80, 40), OB = rgb(30, 24, 30), SK = px565(SKc);
  const float p = ph + i * 0.27f, hx = x; setLimbs(SKc, Col{184, 134, 66}, SKc, SKc);
  auto legF = [&](float q, float y0) { return stand ? legAF(hx, y0, q < 0.5f ? 5.0f : -5.0f, 4) : runLeg(q, hx, y0); };   // a standing guard: the legs at rest, the spear planted upright
  const float hy = fy - fmaxf(legF(p, 0).ay, legF(p + 0.5f, 0).ay) - 1; const Leg lA = legF(p, hy), lB = legF(p + 0.5f, hy);
  const float lean = stand ? 2 * DEG : (12 + 3 * pulse) * DEG, sx = hx + TORSO_L * sinf(lean), sy = hy - TORSO_L * cosf(lean) + 1;
  drawLeg(lB, hx, hy, true);
  const float pump = stand ? 0.0f : sinf(2 * (float)M_PI * p); const Pt hf = stand ? armTo(sx, sy, sx + 5, sy + 4, true) : armTo(sx, sy, sx + 7 + pump, sy - 3 + pump * 2, true);   // the arms pump with the stride; the far hand grips a spear
  if (stand) { seg(hf.x, fy, hf.x, fy - 36, WD, 1); R(hf.x - 1, fy - 40, 3, 4, OB); R(hf.x - 1, fy - 36, 3, 1, RD); }
  else { seg(hf.x - 6, hf.y + 7, hf.x + 10, hf.y - 8, WD, 1); R(hf.x + 10, hf.y - 10, 2, 2, OB); R(hf.x + 8, hf.y - 7, 1, 1, RD); }
  seg(hx, hy, sx, sy, TN, 4); seg(hx - 1, hy - 3, sx + 1, sy + 2, WHITE565, 1);   // the tunic, a pale sash across it
  R(hx - 2, hy, 5, 3, RD); R(hx - 2, hy, 5, 1, G1); R(hx - 3, hy + 1, 1, 2, RD);   // the loincloth
  R(sx - 1, sy - 4, 4, 4, SK); R(sx + 2, sy - 3, 1, 1, OB); R(sx - 1, sy - 4, 1, 4, OB); R(sx - 2, sy - 5, 6, 1, G1);   // the head: hair, an eye, a gold band
  R(sx - 4, sy - 9, 1, 4, TQ); R(sx - 3, sy - 11, 1, 6, RD); R(sx - 1, sy - 12, 1, 7, GR); R(sx + 1, sy - 11, 1, 6, TQ); R(sx - 2, sy - 10, 1, 5, G1);   // the feathers, streaming back as he runs
  drawLeg(lA, hx, hy, false); R(lA.ax, lA.ay - 2, 2, 1, G1); R(lB.ax, lB.ay - 2, 2, 1, G1D);   // gold anklets
  const Pt hn = armTo(sx, sy, sx + 5 - pump, sy + 4 - pump * 2, false); R(hn.x - 1, hn.y - 1, 2, 1, G1); R(hf.x - 1, hf.y - 1, 2, 1, G1);   // gold wristbands
  R(hn.x - 1, hn.y - 4, 6, 8, RD); R(hn.x - 2, hn.y - 3, 8, 6, RD); R(hn.x, hn.y - 2, 4, 4, G1); R(hn.x + 1, hn.y - 1, 2, 2, TQ); R(sx - 1, sy + 3, 2, 1, G1);   // the round shield at the chest
  resetLimbs(); (void)now;
}
static void drawChasers(const Game& G, double now, const AudioIn& A) {
  const Cave& c = G.cv; const float t = c.chase.t;
  for (int i = 2; i >= 0; i--) {
    float x = lp(-26.0f - i * 16, HX - 26.0f - i * 13 + 8 * A.level, sm01(cl01(t / 1.8f))); if (t > 8.5f) x -= (t - 8.5f) * 45; if (c.cartS >= 2) x -= fmaxf(0.0f, c.rideT) * 90;   // in the cart he leaves them far behind
    if (x < -30) continue;
    const float wx = c.scroll + (x - HX); float lift = 0;   // they leap over what he has already cleared
    for (const CvObs& o : c.obs) if (o.live) { float d = fabsf(o.wx - wx), r = 16; if (d < r) lift = fmaxf(lift, 13 * sinf((float)M_PI * (1 - d / r))); }
    drawWarrior((float)rnd(x), (float)(GY - rnd(labH(c, wx)) - rnd(lift)), now, i, HC.ph, A.pulse);
  }
}
static void drawCart(float x, float y, double now, bool parked, float level) {   // a mine cart: an iron-rimmed wooden box on two spoked wheels
  const uint16_t W1 = rgb(96, 70, 54), W2 = rgb(66, 48, 38), ST = rgb(168, 176, 188), DK = rgb(34, 30, 34); const int bob = parked ? 0 : rnd(tsin(now * 32) * 0.6f);
  const float by = y + bob; (void)level;
  R(x - 11, by - 10, 23, 9, W1); R(x - 11, by - 10, 23, 2, ST); R(x - 11, by - 2, 23, 1, W2); R(x - 6, by - 9, 1, 7, W2); R(x + 4, by - 9, 1, 7, W2); R(x - 12, by - 11, 25, 1, ST);
  for (int wi = 0; wi < 2; wi++) { const int wx = wi ? 6 : -7; R(x + wx - 2, by - 3, 5, 5, DK); const int a = parked ? 0 : ((int)floor(now * 28) & 3); R(x + wx - 2, by - 3, 5, 1, ST); R(x + wx - 2, by + 1, 5, 1, ST); R(x + wx - 1 + (a & 1), by - 2 + ((a >> 1) & 1), 1, 1, rgb(228, 232, 234)); R(x + wx + 1 - (a & 1), by + ((a >> 1) & 1), 1, 1, ST); R(x + wx, by - 1, 1, 1, ST); }
}
static void drawCartDust(float x, float y, double now, float level) {   // separate grains behind the rear wheel, bright enough to read in the tunnel
  const int n = 12 + rnd(level * 4);
  for (int i = 0; i < n; i++) {
    const float age = (float)fmod(now * (3.2 + (i % 4) * 0.28) + i * 0.29, 1.0); const float px = x - 14 - rnd(age * (24 + (i % 3) * 8)) - (i % 2), py = y + 1 - rnd(age * (4 + (i % 4) * 2)) + (i % 3) - 1, alpha = (0.78f + level * 0.10f) * (1 - age * 0.55f);
    if (i % 3 == 0) PA(px, py, i % 4 == 0 ? 4 : 2, i % 5 == 0 ? 2 : 1, 242, 206, 154, alpha); else PA(px, py, i % 4 == 0 ? 4 : 2, i % 5 == 0 ? 2 : 1, 202, 164, 115, alpha);
  }
  for (int i = 0; i < 3; i++) { const float phase = (float)fmod(now * 15 + i * 0.37, 1.0); const float px = x - 18 - rnd(phase * 33); PA(px, y + 2 + (i % 2), 4, 1, 248, 220, 178, 0.72f - phase * 0.35f); }   // short separated streaks follow the spinning rear wheel
}
static bool nearGap(const Cave& c, float wx, float r) { for (int i = 0; i < c.nGaps; i++) if (fabsf(wx - c.gaps[i]) < r) return true; return false; }
static void drawRails(const Cave& c, float sc) {   // two iron rails on wooden sleepers, following the sloping floor, from the parked cart on
  const int x0 = (int)fmaxf(0.0f, (float)rnd(HX + c.cartX - 30 - sc));
  for (int x = x0 - x0 % 3; x < W; x += 3) { if (nearGap(c, sc + x - HX, 22)) continue; int fy = GY - rnd(labH(c, sc + x - HX)); R(x, fy, 3, 1, rgb(168, 176, 188)); R(x, fy + 2, 3, 1, rgb(90, 96, 108)); }
  for (int x = -((((int)floorf(sc) % 14) + 14) % 14) - 14; x < W; x += 14) { if (x < x0 || nearGap(c, sc + x - HX, 24)) continue; int fy = GY - rnd(labH(c, sc + x - HX)); R(x, fy + 1, 3, 3, rgb(96, 66, 40)); }
}
static void drawBoulder(float cx, float cy, int r, float ang) {   // a huge round rock, turning as it rolls: craters carried round on its face, a shaded edge, a light top
  const uint16_t Ac = rgb(112, 100, 94), Bc = rgb(88, 78, 74), Dc = rgb(60, 52, 50);
  for (int dy = -r; dy <= r; dy++) { int h = (int)floorf(sqrtf((float)(r * r - dy * dy))); R(cx - h, cy + dy, 2 * h, 1, dy > r * 0.55f ? Dc : dy > r * 0.1f ? Bc : Ac); R(cx - h, cy + dy, 2, 1, Dc); R(cx + h - 2, cy + dy, 2, 1, Dc); }
  for (int i = 0; i < 7; i++) { float th = ang + i * 0.9f, cr = (i % 3) * r * 0.22f + r * 0.2f, px = cosf(th) * cr, py = sinf(th) * cr; if (cosf(th * 0.7f) > -0.2f) { R(cx + px - 3, cy + py - 2, 6, 4, Dc); R(cx + px - 2, cy + py - 3, 4, 1, Bc); } }
  R(cx - r * 0.5f, cy - r * 0.8f, (int)(r * 0.7f), 2, rgb(150, 138, 128));
}
static void drawArrow(float ax, float y) { R(ax, y, 12, 1, rgb(150, 110, 60)); R(ax - 4, y - 1, 4, 3, rgb(190, 196, 206)); R(ax - 2, y, 2, 1, WHITE565); R(ax + 10, y - 1, 3, 3, rgb(214, 60, 50)); R(ax + 11, y, 2, 1, WHITE565); }
static void drawCave(Game& G, const AudioIn& A, double now) {
  const Cave& c = G.cv; const float sc = c.scroll; const CvMv& mv = c.mv; const int dep = c.depth;
  const float xl = HX + 5 + ((mv.live && mv.kind != MV_GATE) ? 0 : c.wx - sc);
  const int kind = c.stage == 2 ? MV_GATE : c.stage == 0 ? MV_DOWN : MV_UP;
  HC.slope = 0; HC.wob = 0; HERO_PULSE = A.pulse;
  if (mv.live && mv.kind == MV_DOWN) {   // dropping a level: the camera follows him down the shaft, this tunnel sliding up and the one below sliding into place
    gOffY = -rnd(mv.d); drawCaveBg(G, now, dep); drawLabFeature(c, xl, MV_DOWN, now);
    gOffY = 135 - rnd(mv.d); drawCaveBg(G, now, dep + 1); gOffY = 0;
    R(xl - 12, GY - mv.d, 24, 135 + 34 - GY, rgb(6, 4, 3));   // the shaft he falls down
  } else if (mv.live && mv.kind == MV_UP) {   // climbing a level: the tunnel slides down the screen and the one above slides into place
    const int dy = rnd(mv.z - 40);
    gOffY = dy - 120; drawCaveBg(G, now, dep - 1);
    gOffY = dy; drawCaveBg(G, now, dep); drawLabFeature(c, xl, MV_UP, now); gOffY = 0;
  } else {
    drawCaveBg(G, now, dep);
    for (int j = 0; j < 30; j++) { int n = rnd(A.eqH[j]); if (n > 7) n = 7; int x = j * PITCH + 1; static const uint16_t FL[5] = {rgb(255, 226, 96), rgb(255, 177, 48), rgb(248, 114, 30), rgb(224, 60, 25), rgb(176, 38, 25)}; for (int k = 0; k < n; k++) { int y = H - SEG * (k + 1) + 1; R(x, y, 6, SEG_H, FL[4]); R(x + 1, y, 4, SEG_H, FL[(k * 5 / (n > 1 ? n : 1)) < 4 ? (k * 5 / (n > 1 ? n : 1)) : 4]); } }   // the flame equalizer along the floor
    if (c.cartS) drawRails(c, sc);
    if (c.cartS == 1 || c.cartS == 2) drawCart((float)rnd(HX + c.cartX - sc), (float)(GY - rnd(labH(c, c.cartX))), now, true, A.level);
    drawLabFeature(c, xl, kind, now);
    for (const CvObs& o : c.obs) if (o.live) { float x = HX + o.wx - sc; if (x > W + 20 || x < -20) continue; int fh = o.kind != CV_LOW ? rnd(labH(c, o.wx)) : 0; gOffY = -fh; drawCaveObs(c, o, x, now); gOffY = 0; }   // things stand on the sloping floor
    for (const CvDrip& d : c.drips) if (d.live) R(d.x, d.y, 1, 3, DRIPC);
    for (const CvSplash& p : c.splash) if (p.live) { int w = 2 + rnd(p.t * 24); R(p.x - (w >> 1), GY - 1, w, 1, DRIPC); }
    if (c.chase.live) drawChasers(G, now, A);
    const int tyA = GY - rnd(labH(c, sc)) - 17;
    if (c.trap.live) { R(W - 5, tyA - 2, 5, 5, rgb(0, 0, 0)); if ((int)floor(now * 16) & 1) { R(W - 4, tyA - 1, 3, 3, rgb(230, 60, 40)); PA(W - 30, tyA - 6, 30, 13, 255, 80, 40, 0.2f); } }   // the slit flashes before it shoots
    for (const auto& a : c.arrows) if (a.live) drawArrow(a.x, (float)tyA);
  }
  if (c.boulder.live && c.boulder.roll) {   // the boulder rolls along the tunnel BEHIND him: he is in a niche cut into the near wall, in front of it
    const float bxw = c.boulder.bw; const int bx = rnd(HX + bxw - sc); int r = (int)fminf(36.0f, (float)(rnd((GY - cey(c, (float)bx)) / 2.0f) - 1)); if (r < 4) r = 4; const int bf = GY - rnd(labH(c, bxw));
    drawBoulder((float)bx, (float)(bf - r), r, bxw / r);
    for (int i = 0; i < 3; i++) PA(bx + r + 4 + i * 9, bf - 5 - i * 3, 9, 5, 160, 140, 118, 0.35f - i * 0.09f);
  }
  const float gbL = (!mv.live) ? (float)(GY - rnd(labH(c, sc))) : (float)GY;   // he stands on the sloping floor
  float y = gbL; HeroMode mode = c.duck ? HM_HIDE : HM_RUN;
  if (c.jump >= 0) { const float u = c.jump / JD; y = gbL - 4 * JH * u * (1 - u); mode = HM_AIR; AIR_U = u; }
  const float ld = c.ld;
  if (mv.live && mv.kind == MV_DOWN) { drawHero(HX, GY - 4, now, HM_PLUNGE); }
  else if (mv.live && mv.kind == MV_GATE) { drawHero((float)(HX + rnd(19 * cl01(mv.t / 1.2f))), GY, now, HM_RUN); fillA(0, 0, W, H, 255, 230, 150, fminf(0.9f, mv.t * 0.8f)); }
  else if (mv.live) { if (mv.mount) drawLedgeHero(mv.q, mv.z, 1, LAB_TOP); else drawLadderHero(HX, mv.z, 1); }
  else {
    if (ld < 0) { const float q = (ld + 0.45f) / 0.45f; y = gbL - rnd(110 * (1 - q) * (1 - q)); mode = HM_PLUNGE; }   // he drops in from the hole above
    else if (ld < 0.4f) mode = HM_HIDE;   // and lands in a crouch, with dust
    if (ld >= 0 && ld < 0.55f) { const float q = ld / 0.55f; for (int sd = -1; sd <= 1; sd += 2) { R(HX + sd * (4 + rnd(q * 14)) - 1, GY - 2 - rnd(q * 3), 3, 2, rgb(126, 116, 100)); R(HX + sd * (2 + rnd(q * 8)) - 1, GY - 1, 2, 1, rgb(160, 150, 130)); } }
    HC.slope = 0.5f * G.sl; HC.wob = G.sl > 0 ? 4 * G.sl : 12 * G.sl;   // uphill he leans in, downhill he leans back
    if (c.cartS == 2) { const float u = cl01(c.bt / 0.5f); AIR_U = 0.5f; drawHero(HX - 1, (float)rnd(gbL - 4 * 9 * u * (1 - u)), now, u < 0.85f ? HM_AIR : HM_CROUCH); drawCart((float)rnd(HX + c.cartX - sc), (float)(GY - rnd(labH(c, c.cartX))), now, true, A.level); }   // the hop into the cart
    else if (c.cart) { float lift = 0; for (int gi = 0; gi < c.nGaps; gi++) { const float u = (sc - (c.gaps[gi] - 38)) / 76; if (u > 0 && u < 1) lift = 4 * 15 * u * (1 - u); } drawHero(HX - 1, (float)(rnd(y - lift) - 1), now, HM_CROUCH); drawCart((float)HX, (float)rnd(y - lift), now, false, A.level); }   // at a broken place the cart flies over the gap; he crouches low in the cart, hands on the rim
    else drawHero(HX, (float)(rnd(y) + ((c.boulder.live && c.boulder.stop) ? 7 : 0)), now, mode);
    HC.slope = 0; HC.wob = 0;
  }
  if (c.boulder.live) {   // the near wall of the niche: two pillars and a lintel in front of the tunnel
    const int nx = rnd(HX + c.boulder.nx - sc), nf = GY - rnd(labH(c, c.boulder.nx)) + 8; const uint16_t P = rgb(48, 38, 36), P2 = rgb(76, 60, 54);
    R(nx - 27, nf - 58, 7, 66, P); R(nx + 20, nf - 58, 7, 66, P); R(nx - 27, nf - 58, 54, 7, P); R(nx - 27, nf - 58, 54, 1, P2); R(nx - 27, nf - 58, 1, 66, P2); R(nx + 26, nf - 58, 1, 66, P2); R(nx - 20, nf - 51, 40, 1, rgb(26, 20, 18));
  }
  for (const Bat& b : c.bats) if (b.live) drawBat(b, A, rgb(150, 100, 190));
  if (!c.duck && !mv.live && !(ld < 0) && !c.cart) { const int ty = rnd(y) - 22 + (c.jump >= 0 ? 0 : ((int)floorf(HC.ph * 2) & 1)); R(HX + 8, ty + 2, 1, 5, rgb(80, 50, 24)); const int fl = trebleFlame(G, now, 1); R(HX + 7, ty - 2 - fl, 3, 4 + fl, rgb(255, 210, 60)); R(HX + 8, ty - 1 - fl, 1, 2, rgb(255, 246, 192)); }   // the torch in his hand
  {   // the torch light: rings, each a step darker than the one inside it
    const float r = 55 + 30 * A.level + G.treble * 8, cx = HX + 8, cy = GY - 20; const float rk[5] = {r, r * 0.83f, r * 0.66f, r * 0.49f, r * 0.32f};
    static const float AL[6] = {0, 0.10f, 0.19f, 0.271f, 0.344f, 0.410f};   // 1 - 0.9^n: n rings darken the dots outside them
    for (int yy = 0; yy < H; yy++) {
      const float dy = yy + 0.5f - cy; float half[5]; for (int k = 0; k < 5; k++) { float q = rk[k] * rk[k] - dy * dy; half[k] = q > 0 ? sqrtf(q) : -1.0f; }
      float ev[10]; int ed[10]; int ne = 0;   // the ring edges on this row: a left edge enters a ring (+1), a right edge leaves it (-1)
      for (int k = 0; k < 5; k++) if (half[k] >= 0) { ev[ne] = cx - half[k]; ed[ne++] = 1; ev[ne] = cx + half[k]; ed[ne++] = -1; }
      for (int i = 1; i < ne; i++) { float v = ev[i]; int dd = ed[i]; int j = i - 1; while (j >= 0 && ev[j] > v) { ev[j + 1] = ev[j]; ed[j + 1] = ed[j]; j--; } ev[j + 1] = v; ed[j + 1] = dd; }
      int inside = 0, xprev = 0;
      for (int i = 0; i <= ne; i++) {
        int xe = i < ne ? (int)ceilf(ev[i]) : W; if (xe > W) xe = W; if (xe < 0) xe = 0;
        if (xe > xprev) { const int n = 5 - inside; if (n > 0) fillA(xprev, yy, xe - xprev, 1, 0, 0, 8, AL[n]); xprev = xe; }
        if (i < ne) inside += ed[i];
      }
    }
  }
  if (c.cart && !mv.live) { float lift = 0; for (int gi = 0; gi < c.nGaps; gi++) { const float u = (sc - (c.gaps[gi] - 38)) / 76; if (u > 0 && u < 1) lift = 4 * 15 * u * (1 - u); } drawCartDust((float)HX, (float)rnd(y - lift + fminf(5.0f, lift * 0.4f)), now, A.level); }   // dust stays visible behind the spinning wheel
}

// ---- The treasury, drawn.
static void drawMural(int k, float x, float y, double now, bool glow) {   // wall paintings in the style of the Aztec codices (the three portraits use the web demo's drawn stand-ins)
  const uint16_t CR = rgb(222, 204, 160), BK = rgb(26, 20, 22), TQ = rgb(30, 130, 112), RD = rgb(200, 48, 38), OR = rgb(226, 128, 40), GD = rgb(244, 196, 56), PALEG = rgb(255, 240, 168);
  R(x, y, 52, 36, BK); R(x + 1, y + 1, 50, 34, CR); R(x + 1, y + 1, 50, 2, RD); R(x + 1, y + 33, 50, 2, RD);   // paper with a red border
  for (int i = 0; i < 12; i++) { R(x + 2 + i * 4, y + 1, 2, 1, BK); R(x + 4 + i * 4, y + 34, 2, 1, BK); }
  if (k == 2) {   // a face between two columns of squares
    const uint16_t c1[3] = {TQ, GD, RD}, c2[3] = {RD, TQ, GD};
    for (int i = 0; i < 6; i++) { R(x + 4, y + 5 + i * 5, 3, 3, c1[i % 3]); R(x + 45, y + 5 + i * 5, 3, 3, c2[i % 3]); }
    R(x + 5, y + 3, 42, 30, WHITE565);
    R(x + 12, y + 7, 28, 23, TQ); R(x + 15, y + 10, 22, 17, GD); R(x + 16, y + 6, 20, 7, RD); R(x + 20, y + 16, 3, 2, BK); R(x + 30, y + 16, 3, 2, BK); R(x + 23, y + 22, 8, 2, RD);
    return; }
  if (k == 4 || k == 5) { R(x + 3, y + 3, 46, 30, OR); R(x + 9, y + 12, 9, 7, GD); R(x + 33, y + 12, 9, 7, GD); R(x + 20, y + 24, 12, 3, TQ); return; }   // the robot and the rabbit portraits' stand-in
  if (k == 3) {   // the Sun Stone, as a few plain blocks like the other paintings: a turquoise ground, an orange disc, a gold face with a red tongue (it glows on the beat)
    R(x + 3, y + 3, 46, 30, TQ); R(x + 12, y + 4, 28, 28, OR); R(x + 8, y + 8, 36, 20, OR); R(x + 15, y + 9, 22, 18, glow ? PALEG : GD); R(x + 13, y + 12, 26, 12, glow ? PALEG : GD);
    R(x + 19, y + 13, 4, 3, BK); R(x + 29, y + 13, 4, 3, BK); R(x + 21, y + 20, 10, 2, BK); R(x + 23, y + 21, 6, 5, RD); R(x + 24, y + 21, 4, 1, CR);
    return; }
  // a stiff profile figure (k 0: turquoise body facing right, k 1: black body facing left): striding legs, a belly disc, a tall feather fan, one arm up with a staff
  const bool fl = k == 1; const uint16_t body = fl ? BK : TQ, fan = fl ? RD : OR; const float ox = x;
  struct Rc { int x, y, w, h; uint16_t c; };
  const Rc rects[] = {{20, 24, 3, 5, body}, {17, 28, 3, 3, body}, {26, 24, 3, 5, body}, {29, 28, 3, 3, body}, {13, 30, 7, 2, RD}, {29, 30, 7, 2, RD},   // legs and sandals
    {19, 21, 11, 3, GD}, {20, 11, 10, 11, body}, {20, 11, 10, 2, RD}, {22, 14, 6, 6, BK},   // loincloth, torso, belt
    {24, 5, 7, 7, body}, {31, 8, 2, 2, body}, {23, 4, 9, 2, RD},   // head, nose, band
    {30, 14, 7, 2, body}, {36, 11, 3, 5, OR}, {20, 14, 2, 4, body}, {14, 17, 7, 2, body}, {10, 16, 4, 5, fan},   // arms: front one raised, back one holding a fan
    {41, 5, 2, 26, TQ}, {40, 3, 4, 3, GD}, {41, 17, 2, 2, RD}, {41, 24, 2, 2, RD}};   // a staff with gold top
  auto X = [&](int lx, int w) { return ox + (fl ? 52 - lx - w : lx); };
  for (const Rc& r : rects) R(X(r.x, r.w) - 1, y + r.y - 1 + 1, r.w + 2, r.h + 2, BK);
  for (int i = 0; i < 5; i++) { const int fx = 17 + i * 3, fh = 5 - ((i == 0 || i == 4) ? 1 : 0); R(X(fx, 2) - 1, y + 3 - fh + 4 - 1, 4, fh + 2, BK); }
  for (const Rc& r : rects) R(X(r.x, r.w), y + r.y + 1, r.w, r.h, r.c);
  for (int i = 0; i < 5; i++) { const int fx = 17 + i * 3, fh = 5 - ((i == 0 || i == 4) ? 1 : 0); R(X(fx, 2), y + 3 - fh + 4, 2, fh, (i & 1) ? RD : fan); }   // the feather fan on the head
  R(X(23, 2), y + 14, 2, 2, BK); R(X(23, 2), y + 14, 1, 1, CR);
  R(X(23, 6), y + 15 + 1, 6, 4, glow ? PALEG : GD); R(X(25, 2), y + 17 + 1, 2, 2, RD);   // the belly disc glows on the beat
  R(X(27, 3), y + 8 + 1, 3, 2, CR); R(X(29, 1), y + 9 + 1, 1, 1, RD); R(X(28, 4), y + 11 + 1, 4, 1, RD);   // eye and mouth
  R(X(41, 2), y + 3, 2, 1, BK);
  (void)now;
}
static void drawGold(float x, float y, double now) {   // a pile of gold bars on a ledge, glinting
  const uint16_t G1 = rgb(252, 208, 64), G2 = rgb(190, 140, 30), G3 = rgb(255, 240, 150);
  for (int r = 0; r < 3; r++) for (int c = 0; c < 3 - r; c++) { const float bx = x - 7 + c * 5 + r * 2.5f; R(bx, y - 3 - r * 3, 5, 3, G1); R(bx, y - 3 - r * 3, 5, 1, G3); R(bx, y - 1 - r * 3, 5, 1, G2); }
  if ((int)floor(now / 0.3) & 1) { R(x + 6, y - 11, 1, 3, WHITE565); R(x + 5, y - 10, 3, 1, WHITE565); }
}
static void drawGuard(float x, float fy, int seed, float ph, bool walk) {   // a guardian at home in the treasury: a few plain blocks like the jars and the mats (no skeleton, no blending), a spear planted beside him
  const uint16_t SK = rgb(150, 92, 58), TN = rgb(236, 150, 60), RD = rgb(214, 50, 40), G1 = rgb(252, 208, 64), WD = rgb(120, 80, 40), OB = rgb(30, 24, 30);
  const uint16_t F1 = (seed % 3 == 0) ? rgb(40, 200, 190) : rgb(30, 170, 80), F2 = (seed % 3 == 1) ? rgb(40, 200, 190) : rgb(214, 50, 40);
  const int st = walk ? ((int)floorf(ph * 4) & 1) : 0;
  R(x + 5, fy - 30, 1, 30, WD); R(x + 4, fy - 34, 3, 4, OB); R(x + 4, fy - 30, 3, 1, RD);   // the spear
  R(x - 3, fy - 5, 2, 5 - st, SK); R(x + 1, fy - 5, 2, 4 + st, SK);   // legs (one steps when he walks)
  R(x - 3, fy - 7, 7, 2, RD); R(x - 3, fy - 13, 7, 6, TN); R(x - 3, fy - 10, 7, 1, WHITE565);   // loincloth, tunic, a pale sash
  R(x - 2, fy - 17, 5, 4, SK); R(x - 2, fy - 18, 5, 1, G1); R(x + 1, fy - 16, 1, 1, OB);   // head with a gold band
  R(x - 2, fy - 21, 1, 3, F1); R(x, fy - 22, 1, 4, F2); R(x + 2, fy - 21, 1, 3, F1);   // feathers
}
static void drawResident(float x, float fy, double now, int kind, int seed, float pulse) {   // the guardians who live in the cavern: some stand guard with a planted spear, some walk their ledge, some sit by a fire, some sleep on a mat
  const uint16_t SK = rgb(150, 92, 58), TN = rgb(236, 150, 60), TQ = rgb(40, 200, 190), RD = rgb(214, 50, 40), GR = rgb(30, 170, 80), G1 = rgb(252, 208, 64);
  if (kind == 0) drawGuard(x, fy, seed, 0, false);
  else if (kind == 1) { const int dir = cosf((float)fmod(now * 0.5 + seed, 6.283185307179586)) > 0 ? 1 : -1; const int px = rnd(x + tsin(now * 0.5 + seed) * 26); if (dir < 0) mirrorOn(px); drawGuard((float)px, fy, seed, (float)fmod(now * 1.1 + seed * 0.3, 1.0), true); if (dir < 0) mirrorOff(); }
  else if (kind == 2) { R(x - 7, fy - 3, 11, 3, SK); R(x - 3, fy - 12, 7, 9, TN); R(x - 3, fy - 8, 7, 1, WHITE565); R(x - 2, fy - 17, 5, 4, SK); R(x + 1, fy - 16, 1, 1, rgb(0, 0, 0)); R(x - 3, fy - 18, 7, 1, G1); R(x - 4, fy - 23, 1, 5, TQ); R(x - 2, fy - 25, 1, 7, RD); R(x, fy - 24, 1, 6, GR); R(x + 2, fy - 22, 1, 4, TQ); seg(x + 3, fy - 9, x + 8 + ((int)floor(now * 2 + seed) & 1), fy - 6, SK, 1); }
  else { const int br = (int)floor(now * 1.2 + seed) & 1; R(x - 10, fy - 2, 22, 2, rgb(190, 60, 50)); R(x - 9, fy - 6 - br, 13, 4, TN); R(x - 15, fy - 5, 6, 3, SK); R(x + 4, fy - 7, 5, 4, SK); R(x + 9, fy - 8, 7, 2, TQ); R(x + 9, fy - 6, 6, 2, RD); R(x + 6, fy - 5, 1, 1, rgb(0, 0, 0)); if ((int)floor(now * 1.5 + seed) & 1) { R(x + 12, fy - 14, 2, 1, WHITE565); R(x + 14, fy - 18, 2, 1, WHITE565); } }   // sleeping, with a drifting z
}
static void drawCavernChasers(const Cavern& C, const CavTab& T, double now, const AudioIn& A, float gb) {   // the guardians behind him; the first wears the carved mask
  const auto& c = C.chase; const float t = c.t;
  for (int i = 2; i >= 0; i--) {
    float x = lp(-26.0f - i * 16, HX - 26.0f - i * 13 + 8 * A.level, sm01(cl01(t / 1.8f))); if (t > 8.5f) x -= (t - 8.5f) * 45;
    if (c.stuck) { const float ex = HX + c.wx0 - 8 - i * 13 - C.scroll; if (ex < x) x = ex; else if (x > ex) x = ex;   // at the edge they stop dead, and the rope leaves them behind
      if (x < -30) continue; if (i == 0) drawMaskedHunter((float)rnd(x), gb, 0); else drawWarrior((float)rnd(x), gb, now, i, 0, A.pulse, true); continue; }
    if (x < -30) continue;
    const float wx = C.scroll + (x - HX); float lift = 0;
    for (int n = 0; n < T.nst; n++) { const CavSt& st = T.st[n]; if (st.lvl != C.lvl) continue; const float hd = st.kind == ST_GAP ? 24.0f : st.kind == ST_PIT ? 34.0f : 14.0f, c0 = (st.kind == ST_GAP || st.kind == ST_PIT) ? (float)st.xs : st.xs + 5.0f, d = fabsf(wx - c0); if (d < hd) lift = fmaxf(lift, 15 * sinf((float)M_PI * (1 - d / hd))); }   // over the gaps and the ladder holes
    const float fy = gb - rnd(lift);
    if (i == 0) drawMaskedHunter((float)rnd(x), fy, HC.ph); else drawWarrior((float)rnd(x), fy, now, i, HC.ph, A.pulse);
  }
}
static void drawCavern(Game& G, const AudioIn& A, double now) {
  const Cavern& C = G.cn; const CavTab& T = cavTab(); const float sc = C.scroll; const auto& m = C.mv;
  const uint16_t ORG = rgb(212, 124, 42), ORD = rgb(150, 84, 26);
  R(0, 0, W, H, rgb(8, 7, 10));
  float hy = (float)CFL(C.lvl);   // the camera follows his floor, the ladder climb and the fall
  if (m.live && m.kind == CM_UP) hy = lp((float)CFL(C.lvl), (float)CFL(C.lvl - 1), cl01(m.t / 2.0f)); else if (m.live && (m.kind == CM_DROP || m.kind == CM_MISS)) hy = fminf((float)CWATER, m.from + 300 * m.t * m.t);
  const int cam = rnd(96 - hy); auto vis = [&](float y) { return y + cam > -50 && y + cam < H + 12; };
  gOffY = cam; HC.slope = 0; HC.wob = 0; HERO_PULSE = A.pulse;
  for (int lv = 0; lv < NF; lv++) { if (!vis((float)(CFL(lv) - 22))) continue; for (int k = (int)floorf(sc * 0.4f / 130) - 1; k < (int)floorf(sc * 0.4f / 130) + 3; k++) { const int x = rnd(k * 130 - sc * 0.4f + 10 + (lv & 1) * 65), mm = (((k + lv) % 6) + 6) % 6; drawMural(mm, (float)x, (float)(CFL(lv) - 40), now, A.pulse > 0.5f); } }   // wall paintings, one row per floor, drifting slowly
  for (int lv = 0; lv < NF; lv++) { const int F = CFL(lv); if (!vis((float)F)) continue; for (int x = 0; x < W; x += 4) { const float wx = sc + x - HX; if (!cavSolid(lv, wx)) continue; const int j = ((((int)floorf(wx / 4)) * 7) % 3 + 3) % 3; R(x, F, 4, 4, ORG); R(x, F + 4, 4, 3 + j, ORD); } }   // the ledges: orange rock, a ragged underside
  for (int i = 0; i < T.nst; i++) { const CavSt& st = T.st[i]; if (st.kind != ST_LADUP) continue; const int xl = rnd(HX + st.xs + 5 - sc); if (xl < -10 || xl > W + 10) continue; const int top = CFL(st.lvl - 1) - 14, bot = CFL(st.lvl); if (!vis((float)top) && !vis((float)bot)) continue;
    R(xl - 4, top, 1, bot - top, rgb(176, 120, 60)); R(xl + 4, top, 1, bot - top, rgb(176, 120, 60)); for (int yy = bot; yy > top; yy -= 6) R(xl - 4, yy, 9, 1, rgb(220, 160, 80)); }   // the ladders: through a hole in the floor above
  for (int lv = 0; lv < NF; lv++) { const int F = CFL(lv); if (!vis((float)F)) continue;   // what lives on each floor: crates, chests, braziers, jars, mats
    for (int k = (int)floorf((sc - HX) / 170) - 1; k < (int)floorf((sc - HX + W) / 170) + 2; k++) { if (((k + lv) & 1) != 0) continue; const int kk = k + lv * 3 + 20; const float wx = k * 170.0f + 60; const int x = rnd(HX + wx - sc);
      bool skip = !cavSolid(lv, wx); for (int i = 0; i < T.nst && !skip; i++) if (fabsf(T.st[i].xs + 5 - wx) < 40 && (T.st[i].lvl == lv || T.st[i].kind == ST_LADUP)) skip = true; for (int i = 0; i < T.ng && !skip; i++) if (T.gold[i].lvl == lv && fabsf(T.gold[i].x - wx) < 36) skip = true; if (skip) continue;
      const int km5 = kk % 5;
      if (km5 == 3) { R(x - 8, F - 8, 9, 8, rgb(130, 84, 40)); R(x - 8, F - 8, 9, 1, rgb(176, 120, 60)); R(x - 8, F - 4, 9, 1, rgb(90, 56, 26)); R(x + 1, F - 6, 8, 6, rgb(130, 84, 40)); R(x + 1, F - 6, 8, 1, rgb(176, 120, 60)); R(x - 5, F - 15, 8, 7, rgb(130, 84, 40)); R(x - 5, F - 15, 8, 1, rgb(176, 120, 60)); R(x - 2, F - 5, 2, 2, rgb(252, 208, 64)); }   // stacked crates of treasure
      else if (km5 == 4) { R(x - 7, F - 6, 14, 6, rgb(130, 84, 40)); R(x - 7, F - 6, 14, 1, rgb(176, 120, 60)); R(x - 7, F - 9, 14, 3, rgb(176, 120, 60)); R(x - 7, F - 11, 14, 2, rgb(252, 208, 64)); R(x - 4, F - 12, 3, 1, rgb(255, 240, 150)); R(x + 1, F - 12, 5, 1, rgb(252, 208, 64)); R(x - 1, F - 4, 3, 3, rgb(252, 208, 64)); if ((int)floor(now * 3 + kk) & 1) { R(x + 4, F - 16, 1, 3, WHITE565); R(x + 3, F - 15, 3, 1, WHITE565); } }   // an open chest brimming with gold
      else if (kk % 3 == 0) { const int fl = trebleFlame(G, now, (float)kk); PA(x - 18, F - 30, 36, 30, 255, 150, 40, 0.10f + 0.04f * G.treble); R(x - 1, F - 9, 2, 9, ORD); R(x - 4, F - 11, 8, 3, rgb(120, 70, 30)); R(x - 3, F - 17 - fl, 6, 6 + fl, rgb(252, 160, 40)); R(x - 1, F - 15 - fl, 2, 4, rgb(255, 240, 160)); }   // a brazier: someone lives here
      else if (km5 == 1) { R(x - 3, F - 8, 7, 8, rgb(176, 92, 52)); R(x - 2, F - 10, 5, 2, rgb(176, 92, 52)); R(x - 3, F - 5, 7, 1, rgb(40, 190, 180)); R(x + 4, F - 6, 2, 4, rgb(176, 92, 52)); }   // a clay jar with a painted band
      else { R(x - 8, F - 2, 16, 2, rgb(190, 60, 50)); R(x - 8, F - 2, 16, 1, rgb(252, 208, 64)); R(x - 6, F - 3, 12, 1, rgb(40, 190, 180)); }   // a woven mat
      if (lv > 0 && (kk & 2)) { const int Tp = CFL(lv - 1); R(x - 3, Tp + 8, 7, 16, rgb(196, 56, 46)); R(x - 3, Tp + 8, 7, 2, rgb(252, 208, 64)); R(x - 1, Tp + 12, 3, 3, rgb(40, 190, 180)); R(x - 3, Tp + 24, 3, 3, rgb(196, 56, 46)); R(x + 1, Tp + 24, 3, 3, rgb(196, 56, 46)); } } }   // and a banner hangs from the ledge above
  // a crowd of guardians at home on every floor, a little back from the edge so that he runs in front of them
  for (int lv = 0; lv < NF; lv++) { if (!vis((float)CFL(lv))) continue; for (int k = (int)floorf((sc - HX) / 48) - 1; k < (int)floorf((sc - HX + W) / 48) + 2; k++) {
      const uint32_t hh = (uint32_t)(k + lv * 131) * 2654435761u; if (((hh >> 8) % 20) > 2) continue; const float wx = k * 48.0f + 12 + (hh >> 12) % 24; const int x = rnd(HX + wx - sc);
      if (x < -30 || x > W + 30 || !cavSolid(lv, wx - 14) || !cavSolid(lv, wx + 14) || !cavSolid(lv, wx)) continue; bool skip = false;
      for (int i = 0; i < T.nst && !skip; i++) if (fabsf(T.st[i].xs + 5 - wx) < 34) skip = true; for (int i = 0; i < T.ng && !skip; i++) if (T.gold[i].lvl == lv && fabsf(T.gold[i].x - wx) < 20) skip = true; if (skip) continue;
      static const int KIND[6] = {0, 1, 2, 3, 0, 1}; drawResident((float)x, (float)(CFL(lv) - 1), now, KIND[(hh >> 16) % 6], (int)((hh >> 20) % 9), A.pulse); } }
  for (int i = 0; i < T.ng; i++) { const CavGoldP& p = T.gold[i]; if (C.taken[i]) continue; const int x = rnd(HX + p.x - sc); if (x > -10 && x < W + 10 && vis((float)CFL(p.lvl))) drawGold((float)x, (float)CFL(p.lvl), now); }
  R(0, CWATER, W, 300, rgb(30, 50, 200)); for (int x = 0; x < W; x += 4) { const int wy = CWATER + rnd(tsin((x + now * 40) * 0.12) * 1); R(x, wy, 4, 2, rgb(110, 140, 255)); }   // the river below
  { const int wy0 = (int)floorf((-cam - 10) / 6.0f) * 6; R(W - 12, wy0, 12, CWATER - wy0, rgb(150, 110, 50)); for (int y = wy0; y < CWATER; y += 6) R(W - 12 + (((y % 5) + 5) % 5), y, 5, 3, rgb(110, 76, 32)); }   // the rock wall at the right
  if (C.chase.live && !m.live) { drawCavernChasers(C, T, now, A, (float)CFL(C.lvl));
    if (C.chase.attack > 0 && C.chase.attack < 0.28f) { const int ay = CFL(C.lvl) - 19; seg((float)(HX - 25), (float)(ay + 4), (float)(HX + 3), (float)ay, rgb(190, 160, 104), 1); R(HX + 2, ay - 2, 4, 3, rgb(216, 224, 226)); } }   // the nearest guardian jabs at the intruder
  for (int i = 0; i < T.nst; i++) { const CavSt& st = T.st[i]; if (st.kind != ST_ROPE) continue; const int xa = rnd(HX + st.xs - sc); if (xa < -50 || xa > W + 50 || !vis((float)CFL(st.lvl))) continue;   // a rope hung
    const int att = CFL(st.lvl) - 44; float ex = (float)(xa + rnd(3 * tsin(now * 2))), ey = (float)(att + 36);
    if (m.live && m.kind == CM_SWING && st.lvl == C.lvl) { const float dx = st.xs - sc, fy = att + sqrtf(fmaxf(1.0f, 36 * 36 - fminf(35 * 35, dx * dx))) + 23; ex = HX + 3; ey = fy - 23; }
    R(xa - 3, att - 4, 6, 4, ORD); seg((float)xa, (float)att, ex, ey, rgb(200, 160, 96), 1); for (int k = 1; k < 6; k++) R((float)rnd(lp((float)xa, ex, k / 6.0f)) - 1, (float)rnd(lp((float)att, ey, k / 6.0f)), 2, 1, rgb(140, 100, 56)); R(ex - 1, ey, 3, 3, rgb(200, 160, 96)); }
  float y = (float)CFL(C.lvl); HeroMode mode = C.duck ? HM_HIDE : HM_RUN;
  if (m.live && (m.kind == CM_DROP || m.kind == CM_MISS)) { const float yy = m.from + 300 * m.t * m.t; y = fminf((float)(CWATER + 2), (float)rnd(yy)); if (yy > CWATER) R(HX - 8, CWATER - 2, 16, 3, WHITE565); drawHero(HX, y, now, HM_PLUNGE); }
  else if (m.live && m.kind == CM_SWING) { const float dx = m.xs - sc; SWING_K = fmaxf(-1.0f, fminf(1.0f, dx / 29)); drawHero(HX, (float)rnd(CFL(C.lvl) - 44 + sqrtf(fmaxf(1.0f, 36 * 36 - fminf(35 * 35, dx * dx))) + 23), now, HM_SWING); }
  else if (m.live) { const float q = cl01(m.t / 2.0f); const int hy2 = rnd(lp((float)CFL(C.lvl), (float)CFL(C.lvl - 1), q)), z = 40 + (CFL(C.lvl) - hy2); gOffY = cam + hy2 - GY; drawLadderHero(HX, (float)z, 1); gOffY = cam; }
  else { if (C.jump >= 0) { const float u = C.jump / C.jd; y = CFL(C.lvl) - 4 * C.jh * u * (1 - u); mode = HM_AIR; AIR_U = u; } drawHero(HX, (float)rnd(y), now, mode); }
  for (const Spark& s : C.sparks) if (s.live) R(s.x, s.y, 2, 2, ((int)floor(s.t * 20) & 1) ? WHITE565 : rgb(252, 208, 64));
  gOffY = 0;
  for (const Bat& b : C.bats) if (b.live) drawBat(b, A);
  if (C.t < 3.2f) { const float a = fminf(fminf(1.0f, C.t / 0.5f), (3.2f - C.t) / 0.6f); fillA(60, 22, 120, 14, 0, 0, 0, 0.55f * fmaxf(0.0f, a)); }   // a caption as he arrives (the words are drawn by the sketch)
}
static bool gameCaption(const Game& G, const char** text, float* alpha) {   // the few words that may appear on the game screen
  if (G.scene == SC_CAVERN && G.cn.t < 3.2f) { *text = "THE TREASURY"; *alpha = fminf(fminf(1.0f, G.cn.t / 0.5f), (3.2f - G.cn.t) / 0.6f); return true; }
  return false;
}

static void drawBeach(Game& G, const AudioIn& A, double now) {
  const Beach& b = G.bc;
  const float sc = b.scroll;
  if (b.forest) drawForestBack(G, A, now); else {
  drawOpenSky(b, A, now);
  Ri(0, GY, W, 4, rgb(226, 184, 130)); Ri(0, GY + 4, W, H - GY - 4, rgb(190, 146, 100)); Ri(0, GY + 14, W, 12, rgb(162, 120, 82));   // on the beach the road is sand
  for (int i = -1; i < 28; i++) {   // grass tufts, shells and darker patches, each fixed to the world, so that the ground is seen to move
    int k = (int)floorf((sc - HX) / 9) + i; float x = HX + k * 9 - sc;
    uint32_t hh = (uint32_t)k * 2654435761u; int a = (hh >> 8) % 5, bb = (hh >> 16) % 7;
    if (a < 2) { uint16_t gc = rgb(150, 160, 70); R(x, GY - 2, 1, 2, gc); R(x + 2, GY - 3, 1, 3, gc); R(x + 4, GY - 2, 1, 2, gc); }
    if (bb == 0) { R(x, GY + 7, 3, 2, rgb(250, 240, 230)); R(x + 1, GY + 7, 1, 1, rgb(240, 170, 160)); } else if (bb == 3) R(x + 1, GY + 11, 3, 2, rgb(170, 140, 90)); else if (bb == 5) R(x, GY + 5, 2, 1, rgb(190, 160, 108));
  }
  }
  if (b.tree.live) drawTree(b.tree, b, A, now);
  HERO_PULSE = A.pulse;
  if (b.rel.live) { const int x = rnd(HX + b.rel.wx - b.scroll); if (x > -20 && x < W + 20) { if (b.rel.shift > 2) drawHole((float)x); drawRelic(x + b.rel.shift, now, b.rel.taken, b.rel.clue); } }
  for (const Obs& o : b.obs) if (o.live) { float x = obsX(b, o); if (x > W + 20) continue; const int gy = GY - ghr(b, o.wx);
    if (o.kind == OB_LOG) drawLog(x, gy); else if (o.kind == OB_NUT) drawNut(o, x, b, now); else if (o.kind == OB_TIGER) drawTiger(x, (float)gy, now, obsTtl(b, o)); else if (o.kind == OB_BEAR) drawBear(x, (float)gy, now, obsTtl(b, o)); else if (o.kind == OB_POND) drawPond(o, x, now, obsTtl(b, o)); else drawBeachTurtle(x, now); }
  if (G.hide && G.hideT > 0.8f) drawJaguar(W + 30 - (G.hideT - 0.8f) * 75, now);
  if (b.hunter.live) drawJungleHunter(b.hunter, b);
  const float gb = (float)(GY - ghr(b, sc));   // where he stands: the ground under him
  float y = gb; HeroMode mode = G.hide ? (G.hideT < 5.0f ? HM_HIDE : HM_IDLE) : HM_RUN;
  if (b.jump >= 0) { float u = b.jump / b.jd; y = gb - 4 * b.jh * u * (1 - u); mode = HM_AIR; AIR_U = u; }
  const float VL = 50, AY = 2; auto feetP = [&](float dx) { return AY + sqrtf(VL * VL - fminf(VL * VL - 1, dx * dx)) + 23; };   // the vine: anchored in the canopy, 50 px long; his hands are 23 px above his feet
  bool postJump = false; const Obs* hang = nullptr;
  if (b.swing.live) {
    const Obs& so = b.obs[b.swing.o]; const float dx = obsTtl(b, so) * V; hang = &so;
    SWING_K = fmaxf(-1.0f, fminf(1.0f, dx / 40));
    if (dx > 40) y = lp(gb, feetP(40), sm01(cl01((67.5f - dx) / 27.5f)));
    else if (dx > -40) y = feetP(dx);
    else { y = lp(feetP(-40), gb, powf(cl01(b.swing.pt / 0.4f), 2)); postJump = true; }
    mode = postJump ? HM_AIR : HM_SWING; if (postJump) AIR_U = 0.8f;
  }
  for (const Obs& o : b.obs) if (o.live && o.wide) {   // the vine hangs from the canopy over every wide pond
    const float xa = obsX(b, o); if (xa < -60 || xa > W + 60) continue;
    float ex = xa + rnd(4 * tsin(now * 2)), ey = AY + VL;
    if (&o == hang && !postJump) { const float hx = HX + 3, hy = y - 23, d = hypotf(hx - xa, hy - AY); if (d > VL) { ex = xa + (hx - xa) * VL / d; ey = AY + (hy - AY) * VL / d; } else { ex = hx; ey = hy; } }
    else if (&o == hang) { ex = xa + 12; ey = AY + 48; }
    seg(xa, AY, ex, ey, rgb(70, 150, 40), 1); for (int k = 1; k < 6; k++) R(lp(xa, ex, k / 6.0f) - 1, lp(AY, ey, k / 6.0f), 2, 1, rgb(40, 100, 28)); R(ex - 1, ey, 3, 3, rgb(70, 150, 40));
  }
  float hxx = HX;
  if (b.find.live) { mode = b.find.t >= 3.2f ? HM_PUSH : HM_FIND; y = gb; FIND_T = b.find.t; }
  if (b.fall.live) { const float t = b.fall.t; hxx = lp((float)HX, HX + b.rel.wx - b.scroll, sm01(cl01(t / 0.35f))); y = gb + rnd(90 * t * t); mode = HM_PLUNGE; }
  HC.slope = 0.5f * G.sl; HC.wob = G.sl > 0 ? 4 * G.sl : 12 * G.sl;   // uphill he leans in, downhill he leans back
  drawHero(hxx, (float)rnd(y), now, mode);
  if (b.find.live) { const float t = b.find.t; if (t < 0.9f) { R(HX + 2, y - 36, 2, 5, rgb(255, 210, 60)); R(HX + 2, y - 30, 2, 2, rgb(255, 210, 60)); } else if (t >= 1.7f && t < 3.2f) drawIdol(HX + 3, y - 31, now, true); }
  HC.slope = 0; HC.wob = 0;
  if (G.hide) drawBush(HX + 2, fminf(1.0f, G.hideT / 0.6f) * (G.hideT > 5.0f ? fmaxf(0.0f, 1 - (G.hideT - 5.0f) / 0.8f) : 1.0f));
  if (b.bat.live) drawBat(b.bat, A);
}

// One frame of the picture: the stage, the dark of a quiet part, the fade, and the level dots.
static void gameRender(Game& G, const AudioIn& A, uint16_t* fb, double now) {
  FB = fb; gMirror = false;
  if (G.scene == SC_LINER) drawLiner(G, A, now); else if (G.scene == SC_WALL) drawWall(G, A, now); else if (G.scene == SC_VINES) drawVines(G, now); else if (G.scene == SC_BOAT) drawBoatfall(G, A, now); else if (G.scene == SC_RIVER || G.scene == SC_WATERWAY) drawRiver(G, A, now); else if (G.scene == SC_FOUNTAIN) drawFountain(G, now); else if (G.scene == SC_SKYFALL) drawSkyfall(G, now); else if (G.scene == SC_PYRAMID) drawPyramid(G, now); else if (G.scene == SC_CAVE) drawCave(G, A, now); else if (G.scene == SC_CAVERN) drawCavern(G, A, now); else drawBeach(G, A, now);   // the beach and the forest share the hero, the obstacles and the events
  const float kScene = (G.scene == SC_LINER || G.scene == SC_BEACH) ? 0.45f : (G.scene == SC_BOAT || G.scene == SC_CAVE || G.scene == SC_FOUNTAIN || G.scene == SC_SKYFALL || G.scene == SC_PYRAMID) ? 0.0f : 1.0f;   // the web demo: less dark at sea and on the beach, none on the brook
  float dark = (A.sounding ? fmaxf(0.0f, 0.5f - A.level * 0.9f) : 0.55f) * kScene;   // the quieter it gets, the darker
  G.visualDark = G.visualDark < 0 ? dark : G.visualDark + (dark - G.visualDark) * (1.0f - powf(1.0f - 0.018f, 60.0f * G.dt));
  if (G.visualDark > 0) fillA(0, 0, W, H, 0, 0, 12, G.visualDark);
  if (G.fade > 0) fillA(0, 0, W, H, 0, 0, 0, G.fade < 1 ? G.fade : 1);
  for (int i = 0; i < 6; i++) Ri(W - 10 - i * 5, 125, 3, 3, (A.pulse > 0.5f && i == 0) ? rgb(255, 210, 60) : (A.level * 6 > i ? rgb(40, 170, 50) : rgb(40, 50, 40)));
}

// The name of the stage for the status line.
static const char* gameStageName(const Game& G) { return G.scene == SC_BEACH ? "BEACH" : G.scene == SC_FOREST ? "FOREST 2" : G.scene == SC_WALL ? "WALL" : G.scene == SC_VINES ? "VINES" : G.scene == SC_BOAT ? "BOAT+FALLS" : G.scene == SC_RIVER ? "RIVER" : G.scene == SC_FOREST3 ? "FOREST" : G.scene == SC_CAVE ? "TUNNEL" : G.scene == SC_CAVERN ? "TREASURY" : G.scene == SC_WATERWAY ? "WATERWAY" : G.scene == SC_FOUNTAIN ? "FOUNTAIN" : G.scene == SC_SKYFALL ? "SKYFALL" : G.scene == SC_PYRAMID ? "PYRAMID" : "LINER"; }

}  // namespace beatadv
