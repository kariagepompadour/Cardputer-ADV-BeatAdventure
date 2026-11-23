// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
/*
  ba_hero.h — BeatAdventure: the explorer, a skeleton drawn as thick lines (a port of the web demo's drawHero and its helpers).

  Not a set of pictures: hip, knee, ankle; shoulder, elbow, posed from a run cycle and drawn with seg(), so the stride reads as a run:
  the knee is nearly straight when the foot lands and folds deeply while the leg swings forward, the arms swing against the legs,
  the body leans in, and the whole body is lowered so the planted foot always touches the ground. Colours: fedora h, skin s, leather
  jacket g, khaki trousers p, boots b. Poses so far: run, idle, and the liner's ladder, ledge and lookout. More come with the stages.
*/
#pragma once
#include "ba_gfx.h"

namespace beatadv {

constexpr int GY = 98, HX = 60;   // the ground line and the hero's x
constexpr float DEG = (float)M_PI / 180.0f, TH = 5.5f, SH = 5.5f, UA = 4.0f, FA = 3.5f, TORSO_L = 7.0f;   // thigh, shin, upper arm, forearm, torso (px)

constexpr Col C_H{96, 58, 26}, C_S{226, 170, 120}, C_G{138, 84, 36}, C_P{196, 168, 108}, C_B{60, 36, 18}, C_K{58, 34, 18};   // fedora, skin, jacket, trousers, boots, hair
constexpr uint16_t P_H = px565(C_H), P_S = px565(C_S), P_G = px565(C_G), P_P = px565(C_P), P_B = px565(C_B), P_K = px565(C_K);
constexpr uint16_t P_S_D = px565(shadeC(C_S, 0.6f)), P_G_D = px565(shadeC(C_G, 0.6f)), P_P_D = px565(shadeC(C_P, 0.6f)), P_B_D = px565(shadeC(C_B, 0.6f));

// What the poses read from the game (the web demo's S.ph, S.sneak, S.slope, S.wob, S.hurt).
struct HeroCtx { float ph = 0, sneak = 0, slope = 0, wob = 0, hurt = 0; };
static HeroCtx HC;

struct Leg { float kx, ky, ax, ay; };
struct Pt { float x, y; };

static Leg legAF(float hx, float hy, float aDeg, float fDeg) {   // a leg from the hip: the thigh at aDeg from straight down (forward +), the knee folded fDeg
  float a = aDeg * DEG, b = a - fDeg * DEG, kx = hx + TH * sinf(a), ky = hy + TH * cosf(a);
  return Leg{kx, ky, kx + SH * sinf(b), ky + SH * cosf(b)};
}
static Leg runLeg(float q, float hx, float hy) {   // q: this leg's phase 0..1; the foot lands (thigh forward, knee straight) at q = 0.25. Tiptoeing: smaller swing, knees always bent
  float sl = HC.slope * 2, up = sl > 0 ? sl : 0, dn = -sl > 0 ? -sl : 0;   // uphill: short steps, the knee driven high; downhill: long reaching steps, the knee kept soft
  float amp = (1 - 0.45f * HC.sneak) * (1 - 0.45f * up + 0.2f * dn), c = cosf(2 * (float)M_PI * q + 0.45f), cp = c > 0 ? c : 0;
  return legAF(hx, hy, 8 + 36 * amp * sinf(2 * (float)M_PI * q) + 6 * up, 10 + 28 * HC.sneak + 82 * amp * cp + 26 * up * cp + 12 * dn);
}
static void drawLeg(const Leg& L, float hx, float hy, bool dark) {
  uint16_t pc = dark ? P_P_D : P_P, bc = dark ? P_B_D : P_B;
  seg(hx, hy, L.kx, L.ky, pc, 2); seg(L.kx, L.ky, L.ax, L.ay, pc, 2); R(L.ax - 1, L.ay - 1, 4, 2, bc);   // the boot points forward
}
static void drawArm(float sx, float sy, float ang, float bend, bool dark) {   // ang from straight down (forward +); the forearm folds by bend; jacket sleeve to the wrist, then a small hand
  uint16_t sc = dark ? P_G_D : P_G, kc = dark ? P_S_D : P_S;
  float ex = sx + UA * sinf(ang), ey = sy + UA * cosf(ang), wx = ex + FA * sinf(ang + bend), wy = ey + FA * cosf(ang + bend);
  seg(sx, sy, ex, ey, sc, 2); seg(ex, ey, wx, wy, sc, 2); R((float)rnd(wx) - 1, (float)rnd(wy) - 1, 2, 2, kc);
}
static Pt armTo(float sx, float sy, float tx, float ty, bool dark, float sg = 1) {   // a two-bone arm reaching a point (the elbow hangs below the line); sg = -1 mirrors the elbow; returns the hand
  float dx = tx - sx, dy = ty - sy, d = hypotf(dx, dy);
  if (d == 0) d = 1;
  const float L = UA + FA - 0.3f;
  if (d > L) { dx *= L / d; dy *= L / d; d = L; }
  float a = (UA * UA - FA * FA + d * d) / (2 * d), h = sqrtf(fmaxf(0, UA * UA - a * a)), ex = sx + dx * a / d - sg * dy / d * h, ey = sy + dy * a / d + sg * dx / d * h;
  uint16_t sc = dark ? P_G_D : P_G; seg(sx, sy, ex, ey, sc, 2); seg(ex, ey, sx + dx, sy + dy, sc, 2);
  R((float)rnd(sx + dx) - 1, (float)rnd(sy + dy) - 1, 2, 2, dark ? P_S_D : P_S);
  return Pt{sx + dx, sy + dy};
}
static void legTo(float hx, float hy, float tx, float ty, bool dark, float sg = 1) {   // a two-bone leg reaching a foot target; the knee points forward
  float dx = tx - hx, dy = ty - hy, d = hypotf(dx, dy);
  if (d == 0) d = 1;
  const float L = TH + SH - 0.3f;
  if (d > L) { dx *= L / d; dy *= L / d; d = L; }
  float a = (TH * TH - SH * SH + d * d) / (2 * d), h = sqrtf(fmaxf(0, TH * TH - a * a)), kx = hx + dx * a / d + sg * dy / d * h, ky = hy + dy * a / d - sg * dx / d * h;
  uint16_t pc = dark ? P_P_D : P_P; seg(hx, hy, kx, ky, pc, 2); seg(kx, ky, hx + dx, hy + dy, pc, 2);
  R((float)rnd(hx + dx) - 1, (float)rnd(hy + dy) - 1, 4, 2, dark ? P_B_D : P_B);
}
// Torso and a head that can turn independently of the running body; (hx, hy) is the hip. Returns the shoulder.
static Pt drawBody(float hx, float hy, float lean, float dip = 0, bool lookBehind = false) {
  float sx = hx + TORSO_L * sinf(lean), sy = hy - TORSO_L * cosf(lean), d = dip, dy = d != 0 ? 2.0f : 0.0f;
  seg(hx, hy, sx, sy, P_G, 4);
  if (lookBehind) { R(sx + 1 + d, sy - 3 + dy, 1, 2, P_K); R(sx - 2 + d, sy - 3 + dy, 2, 3, P_S); R(sx - 2 + d, sy - 2 + dy, 1, 1, P_K); }
  else { R(sx - 1 + d, sy - 3 + dy, 1, 2, P_K); R(sx + d, sy - 3 + dy, 2, 3, P_S); }
  R(sx - 1 + d, sy - 5 + dy, 3, 1, P_H); R(sx + (lookBehind ? -4 : -3) + d, sy - 4 + dy, 8, 1, P_H);   // the brim points toward the face
  return Pt{sx, sy};
}

enum HeroMode : uint8_t { HM_RUN, HM_IDLE };

static void drawHero(float x, float feetY, double now, HeroMode mode) {
  if (HC.hurt > 0 && ((int)floor(now * 20) & 1)) { R(x - 5, feetY - 24, 10, 24, WHITE565); return; }
  const float hx = x, p = HC.ph;
  const bool idle = mode == HM_IDLE;
  const float breathe = sinf((float)(now * 2.2));
  auto gY = [&](float fx) { return feetY - HC.slope * (fx - hx); };   // the ground under a foot: on a slope the foot ahead meets higher ground
  Leg la, lb;
  float hy;
  auto legs = [&](float hyv, Leg& a, Leg& b) {
    if (idle) { a = legAF(hx, hyv, 4, 3); b = legAF(hx, hyv, -4, 3); }
    else { a = runLeg(p, hx, hyv); b = runLeg(p + 0.5f, hx, hyv); }
  };
  { Leg a, b; legs(0, a, b); hy = fminf(gY(a.ax) - a.ay, gY(b.ax) - b.ay) - 1; legs(hy, la, lb); }
  float lean, sw0, sw1, bend0, bend1;
  if (idle) { lean = (3 + breathe + HC.wob) * DEG; sw0 = 8 * DEG; sw1 = -8 * DEG; bend0 = bend1 = 20 * DEG; }
  else {
    lean = (9 + 16 * HC.sneak + HC.wob) * DEG;
    float A = 36 * (1 - 0.45f * HC.sneak), f0 = -sinf(2 * (float)M_PI * p), f1 = -sinf(2 * (float)M_PI * (p + 0.5f));   // each arm swings against the leg on its own side
    sw0 = A * f0 * DEG; sw1 = A * f1 * DEG;
    bend0 = (68 + 32 * f0) * DEG * (1 - 0.25f * HC.sneak); bend1 = (68 + 32 * f1) * DEG * (1 - 0.25f * HC.sneak);
  }
  float sx = hx + TORSO_L * sinf(lean), sy = hy - TORSO_L * cosf(lean) + 1;
  drawLeg(lb, hx, hy, true); drawArm(sx, sy, sw1, bend1, true);   // far leg and far arm first
  drawBody(hx, hy, lean, 0, false);
  drawLeg(la, hx, hy, false); drawArm(sx, sy, sw0, bend0, false);
}

// ---- Poses on the liner: a ladder, a ledge, a lookout. Z is the height of his feet above the lower deck (40 on the deck, 100 on the roof).
static void mirrorOn(int x) { gMirror = true; gMirrorX = x; }
static void mirrorOff() { gMirror = false; }

static void drawLadderHero(float x, float z, int dir) {   // facing the ladder (mirrored when climbing down the other cliff): each rung is a step with the hand and foot of opposite sides, the other pair waits
  const float u = (z - 40) / 6, hipY = GY - 10, lx = x + 5;
  auto rungY = [&](float k) { return GY - (40 + 6 * k - z); };
  auto pair = [&](float s, float& hand, float& foot) {
    float w = u + s, b = floorf(w / 2), f = w / 2 - b, m = sm01(cl01((f - 0.5f) / 0.5f)), kh = 3 + 2 * (b + m);
    hand = rungY(kh); foot = rungY(kh - 3);
  };
  if (dir < 0) mirrorOn((int)x);
  float h0, f0, h1, f1;
  pair(0, h0, f0); pair(1, h1, f1);
  float lean = 12 * DEG, sx = x + TORSO_L * sinf(lean), sy = hipY - TORSO_L * cosf(lean) + 1;
  legTo(x, hipY, lx - 1, f0 - 1, true); armTo(sx, sy, lx + 1, h1, true);   // the far hand and the far foot belong to different pairs
  drawBody(x, hipY, lean);
  legTo(x, hipY, lx - 1, f1 - 1, false); armTo(sx, sy, lx + 1, h0, false);
  mirrorOff();
}
static void drawLedgeHero(float q, float z, int dir, float H0 = 100) {   // H0: the height of the ledge. Pulling himself over a cliff edge (q 0 -> 1): hands flat on the top, chest up, one knee swings onto the ledge, then the other foot, then he stands; played backwards, mirrored, it is lowering himself off the edge
  if (dir < 0) mirrorOn(HX);
  float pq = sm01(cl01((q - 0.5f) / 0.5f)), Ly = GY - (H0 - z), Lx = HX + 6 - 8 * pq, lx = HX + 5;
  float pull = sm01(cl01(q / 0.45f)), sw = sm01(cl01((q - 0.4f) / 0.4f)), up2 = sm01(cl01((q - 0.7f) / 0.3f)), st = sm01(cl01((q - 0.8f) / 0.2f));
  float lean = (12 + 22 * pull - 25 * st) * DEG, hx = HX + 3 * sw * (1 - st), hy = GY - 10, sx = hx + TORSO_L * sinf(lean), sy = hy - TORSO_L * cosf(lean) + 1;
  auto rY = [&](float h) { return GY - (h - z) - 1; };
  legTo(hx, hy, lp(lx - 1, Lx + 1, up2), lp(rY(H0 - 18), Ly - 1, up2), true);
  armTo(sx, sy, lp(Lx + 2, sx, st), lp(Ly - 1, sy + 8, st), true);
  drawBody(hx, hy, lean);
  legTo(hx, hy, lp(lx - 1, Lx + 4, sw), lp(rY(H0 - 12), Ly - 1, sw), false);
  armTo(sx, sy, lp(Lx + 5, sx + 2, st), lp(Ly - 1, sy + 8, st), false);
  mirrorOff();
}
static void drawLookoutHero(float x, float feetY) {   // one boot on a raised deck fitting, hand on that knee, looking toward the horizon
  float hipY = feetY - legAF(x, 0, 4, 3).ay - 1, lean = 8 * DEG;
  float shoulderX = x + TORSO_L * sinf(lean), shoulderY = hipY - TORSO_L * cosf(lean) + 1;
  drawLeg(legAF(x, hipY, -4, 3), x, hipY, true);   // the planted leg uses the usual idle skeleton
  armTo(shoulderX, shoulderY, x - 3, hipY + 1, true, -1);   // hand at the hip
  drawBody(x, hipY, lean);   // exactly the same face, hat and torso scale as the other poses
  seg(x, hipY, x + 4, hipY - 2, P_P, 2);
  seg(x + 4, hipY - 2, x + 8, feetY - 11, P_P, 2);
  R(x + 7, feetY - 12, 4, 2, P_B);   // raised boot rests on the fitting
  armTo(shoulderX, shoulderY, x + 4, hipY - 2, false);   // hand on the raised knee
}

}  // namespace beatadv
