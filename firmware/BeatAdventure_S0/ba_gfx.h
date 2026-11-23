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
static bool gMirror = false;             // draw mirrored about x = gMirrorX (the web demo's translate(x, 0); scale(-1, 1); translate(-x, 0))
static int gMirrorX = 0;

static inline int rnd(float v) { return (int)floorf(v + 0.5f); }    // JavaScript's Math.round
static inline float cl01(float f) { return f < 0 ? 0 : f > 1 ? 1 : f; }
static inline float sm01(float f) { return f * f * (3 - 2 * f); }
static inline float lp(float a, float b, float t) { return a + (b - a) * t; }

// A filled rectangle at (x, y) rounded, w x h, clipped to the screen.
static inline void R(float xf, float yf, int w, int h, uint16_t c) {
  int x = rnd(xf), y = rnd(yf);
  if (gMirror) x = 2 * gMirrorX - x - w;
  int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y, x1 = x + w > W ? W : x + w, y1 = y + h > H ? H : y + h;
  for (int yy = y0; yy < y1; yy++) {
    uint16_t* row = FB + yy * W;
    for (int xx = x0; xx < x1; xx++) row[xx] = c;
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

// A rectangle of colour (r, g, b) blended over the frame buffer with alpha a (0..1).
static void fillA(int x, int y, int w, int h, int r, int g, int b, float a) {
  if (a <= 0) return;
  if (a > 1) a = 1;
  int A = (int)(a * 256), x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y, x1 = x + w > W ? W : x + w, y1 = y + h > H ? H : y + h;
  int sr = r >> 3, sg = g >> 2, sb = b >> 3;
  for (int yy = y0; yy < y1; yy++) {
    uint16_t* row = FB + yy * W;
    for (int xx = x0; xx < x1; xx++) {
      uint16_t d = swap565(row[xx]);
      int dr = d >> 11, dg = (d >> 5) & 63, db = d & 31;
      dr += ((sr - dr) * A) >> 8; dg += ((sg - dg) * A) >> 8; db += ((sb - db) * A) >> 8;
      row[xx] = swap565((uint16_t)((dr << 11) | (dg << 5) | db));
    }
  }
}

}  // namespace beatadv
