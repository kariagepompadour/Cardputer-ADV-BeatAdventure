// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
/*
  ba_gfx.h — BeatAdventure: the drawing layer over the 240 x 135 RGB565 frame buffer (byte-swapped, as in M5Canvas's buffer).

  The web demo draws with Canvas 2D: fillRect, a few transforms, alpha. This is the small set of that the game needs, with the same
  rounding (JavaScript's Math.round) so the same numbers land on the same pixels:
    R()      a filled rectangle at a rounded position, clipped; mirrored when a mirror is on (the web demo's translate + scale(-1, 1))
    seg()    a thick line: a t x t brush walked from one end to the other
    shade()  a colour scaled toward black
    fillA()  a rectangle blended over the frame buffer with an alpha (the dark overlay, the fade)
  No Arduino headers; also used by the host preview (host/preview.cpp).
*/
#pragma once
#include <stdint.h>
#include <math.h>

namespace beatadv {

constexpr int W = 240, H = 135;
constexpr uint16_t swap565(uint16_t c) { return (uint16_t)((c << 8) | (c >> 8)); }
struct Col { uint8_t r, g, b; };
constexpr uint16_t px565(Col c) { return swap565((uint16_t)(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3))); }
constexpr uint16_t rgb(int r, int g, int b) { return px565(Col{(uint8_t)r, (uint8_t)g, (uint8_t)b}); }
constexpr Col shadeC(Col c, float k) { return Col{(uint8_t)(c.r * k + 0.5f), (uint8_t)(c.g * k + 0.5f), (uint8_t)(c.b * k + 0.5f)}; }   // the web demo's shade() with FAR = 1
constexpr uint16_t WHITE565 = 0xFFFF;

static uint16_t* FB = nullptr;           // the frame buffer being drawn into
static float gAlpha = 1.0f;              // the web demo's globalAlpha: below 1, R() blends
static bool gMirror = false;             // draw mirrored about x = gMirrorX (the web demo's translate(x, 0); scale(-1, 1); translate(-x, 0))
static int gMirrorX = 0;
static int gOffY = 0;                    // draw everything this many rows lower (the web demo's translate(0, dy): the tunnel sliding past)
static int gClipB = H;                   // draw nothing below this row (the web demo's clip(): a crocodile's head is cut off by the water)
static bool gRot = false;                // draw rotated (the web demo's translate(ox, oy); rotate(a)): R() fills the rotated rectangle pixel by pixel
static float gRotCos = 1, gRotSin = 0, gRotX = 0, gRotY = 0;

static inline int rnd(float v) { return (int)floorf(v + 0.5f); }    // JavaScript's Math.round
static inline float tsin(double x) { return sinf((float)fmod(x, 6.283185307179586)); }   // sin of a time-based phase without losing float precision
static inline float cl01(float f) { return f < 0 ? 0 : f > 1 ? 1 : f; }
static inline float sm01(float f) { return f * f * (3 - 2 * f); }
static inline float lp(float a, float b, float t) { return a + (b - a) * t; }

// A filled rectangle at (x, y) rounded, w x h, clipped to the screen.
static void fillA(int x, int y, int w, int h, int r, int g, int b, float a);
static inline void R(float xf, float yf, int w, int h, uint16_t c) {
  int x = rnd(xf), y = rnd(yf) + gOffY;
  if (gRot) {   // the rectangle in the rotated frame: test each screen pixel of its bounding box
    const float cx[4] = {(float)x, (float)(x + w), (float)x, (float)(x + w)}, cy[4] = {(float)y, (float)y, (float)(y + h), (float)(y + h)};
    float mnx = 1e9f, mxx = -1e9f, mny = 1e9f, mxy = -1e9f;
    for (int i = 0; i < 4; i++) { float wx = gRotX + gRotCos * cx[i] - gRotSin * cy[i], wy = gRotY + gRotSin * cx[i] + gRotCos * cy[i]; if (wx < mnx) mnx = wx; if (wx > mxx) mxx = wx; if (wy < mny) mny = wy; if (wy > mxy) mxy = wy; }
    int px0 = (int)floorf(mnx), px1 = (int)ceilf(mxx), py0 = (int)floorf(mny), py1 = (int)ceilf(mxy);
    if (px0 < 0) px0 = 0; if (py0 < 0) py0 = 0; if (px1 > W) px1 = W; if (py1 > H) py1 = H;
    for (int py = py0; py < py1; py++) for (int pxl = px0; pxl < px1; pxl++) {
      float dx = pxl + 0.5f - gRotX, dy = py + 0.5f - gRotY, lx = gRotCos * dx + gRotSin * dy, ly = -gRotSin * dx + gRotCos * dy;
      if (lx >= x && lx < x + w && ly >= y && ly < y + h) FB[py * W + pxl] = c;
    }
    return;
  }
  if (gMirror) x = 2 * gMirrorX - x - w;
  int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y, x1 = x + w > W ? W : x + w, y1 = y + h > H ? H : y + h;
  if (y1 > gClipB) y1 = gClipB;
  if (x1 <= x0 || y1 <= y0) return;   // wholly off the screen
  if (gAlpha < 0.999f) { uint16_t d = swap565(c); fillA(x0, y0, x1 - x0, y1 - y0, (d >> 11) << 3, ((d >> 5) & 63) << 2, (d & 31) << 3, gAlpha); return; }
  const uint32_t cc = (uint32_t)c | ((uint32_t)c << 16);
  for (int yy = y0; yy < y1; yy++) {   // two pixels at a time (the buffer's rows are 4-byte aligned)
    uint16_t* p = FB + yy * W + x0;
    int n = x1 - x0;
    if (n > 0 && (((uintptr_t)p) & 2)) { *p++ = c; n--; }
    uint32_t* q = (uint32_t*)p;
    for (int k = n >> 1; k > 0; k--) *q++ = cc;
    if (n & 1) *(uint16_t*)q = c;
  }
}
static inline void Ri(int x, int y, int w, int h, uint16_t c) { R((float)x, (float)y, w, h, c); }

// A thick line: a t x t brush walked from one end to the other.
static void seg(float xa, float ya, float xb, float yb, uint16_t c, int t) {
  int x0 = rnd(xa), y0 = rnd(ya), x1 = rnd(xb), y1 = rnd(yb);
  int dx = x1 - x0 < 0 ? x0 - x1 : x1 - x0, dy = y1 - y0 < 0 ? y0 - y1 : y1 - y0;
  int n = dx > dy ? dx : dy;
  if (n < 1) n = 1;
  for (int i = 0; i <= n; i++) R(x0 + (float)(x1 - x0) * i / n - (t >> 1), y0 + (float)(y1 - y0) * i / n - (t >> 1), t, t, c);
}

// A rectangle of colour (r, g, b) blended over the frame buffer with alpha a (0..1), in 32 steps: the usual RGB565 trick, the three
// channels spread over one 32-bit word so a pixel costs one multiply for each of the two terms.
static void fillA(int x, int y, int w, int h, int r, int g, int b, float a) {
  if (a <= 0) return;
  int A = (int)(a * 32 + 0.5f);
  if (A <= 0) return;
  if (A > 32) A = 32;
  int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y, x1 = x + w > W ? W : x + w, y1 = y + h > H ? H : y + h;
  if (x1 <= x0 || y1 <= y0) return;
  uint32_t s565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  const uint32_t src = ((s565 | (s565 << 16)) & 0x07E0F81Fu) * (uint32_t)A, ia = (uint32_t)(32 - A);
  for (int yy = y0; yy < y1; yy++) {
    uint16_t* row = FB + yy * W;
    for (int xx = x0; xx < x1; xx++) {
      uint32_t d = swap565(row[xx]);
      d = (d | (d << 16)) & 0x07E0F81Fu;
      d = ((d * ia + src) >> 5) & 0x07E0F81Fu;
      row[xx] = swap565((uint16_t)(d | (d >> 16)));
    }
  }
}

}  // namespace beatadv
