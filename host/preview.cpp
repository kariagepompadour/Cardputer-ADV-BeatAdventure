// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
// Renders the game's frames on a computer, with a steady 120 bpm beat and a moving equalizer standing in for the sound, and writes
// PPM files of the frames you ask for (the pictures to compare with the web demo's). No Cardputer needed.
//   preview OUTDIR t0 t1 t2 ...        (seconds into the game; frames are 1/30 s apart)
//   preview OUTDIR --scene N t0 ...    (start at scene N; later stages)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "../firmware/BeatAdventure_S0/ba_game.h"

using namespace beatadv;

static void writePpm(const char* path, const uint16_t* fb) {
  FILE* f = fopen(path, "wb");
  fprintf(f, "P6\n%d %d\n255\n", W, H);
  for (int i = 0; i < W * H; i++) {
    uint16_t c = swap565(fb[i]);
    unsigned char rgb3[3] = {(unsigned char)((c >> 11) << 3), (unsigned char)(((c >> 5) & 63) << 2), (unsigned char)((c & 31) << 3)};
    fwrite(rgb3, 1, 3, f);
  }
  fclose(f);
}

int main(int argc, char** argv) {
  if (argc < 3) { fprintf(stderr, "usage: preview OUTDIR t0 t1 ...\n"); return 2; }
  const char* dir = argv[1];
  std::vector<double> times;
  int startScene = -1; double quietFrom = -1;
  for (int i = 2; i < argc; i++) {
    if (!strcmp(argv[i], "--quiet") && i + 1 < argc) { quietFrom = atof(argv[++i]); continue; }   // the music goes quiet from this time on
    if (!strcmp(argv[i], "--scene") && i + 1 < argc) { startScene = atoi(argv[++i]); continue; }
    times.push_back(atof(argv[i]));
  }
  static uint16_t fb[W * H];
  static float eq[30];
  Game G;
  gameInit(G, 0);
  if (startScene >= 0) setScene(G, (Scene)startScene, 0);
  const float dt = 1.0f / 30;
  AudioIn A;
  A.sounding = true; A.T = 0.5f; A.eqH = eq;
  double next = 0; size_t ti = 0, nbeat = 0;
  for (double t = 0; ti < times.size(); t += dt) {
    A.beatNow = t >= next; if (A.beatNow) { next += A.T; nbeat++; A.pulse = 1; }
    A.beatN = (long)nbeat;
    A.pulse *= powf(0.9f, 60 * dt);
    A.level = 0.62f + 0.3f * sinf((float)t * 0.7f);
    if (quietFrom >= 0 && t >= quietFrom) A.level = 0.12f;
    A.rms = 0.15f * A.level;
    for (int j = 0; j < 30; j++) { float v = 3.5f + 3.0f * sinf((float)t * 3 + j * 0.45f) * sinf((float)t * 0.9f + j * 0.2f); if (v > eq[j]) eq[j] = v; eq[j] -= 28 * dt; if (eq[j] < 0) eq[j] = 0; }
    gameUpdate(G, A, dt, t);
    if (t + dt / 2 >= times[ti]) {
      gameRender(G, A, fb, t);
      char path[512]; snprintf(path, sizeof path, "%s/frame_%05.1f.ppm", dir, times[ti]);
      writePpm(path, fb);
      printf("t=%.1f scene=%d ph=%d w=%.0f z=%.0f scroll=%.0f jump=%.2f\n", t, (int)G.scene, (int)G.cl.ph, G.cl.w, G.cl.z, G.bc.scroll, G.bc.jump);
      ti++;
    }
  }
  return 0;
}
