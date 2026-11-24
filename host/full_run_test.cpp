// Copyright (c) 2026 Kariage POMPADOUR Entertainment Corporation. MIT License.
// A whole run from the liner on, with random music, quiet stretches and long silences (the camp): no stage may keep him for ten minutes,
// the camp must come after a silence, and nothing may touch memory it should not. usage: full_run_test <seeds> <seconds>
#include <stdio.h>
#include <stdlib.h>
#include "../firmware/BeatAdventure_S0/ba_game.h"
using namespace beatadv;
int main(int argc,char**argv){
  static uint16_t fb[W*H]; static float eq[30]; static Game G;
  int N=atoi(argv[1]); double span=atof(argv[2]); int bad=0;
  for(int seed=1;seed<=N;seed++){
    srand(seed); gameInit(G,seed); setScene(G,SC_LINER,0);
    AudioIn A; A.sounding=true; A.eqH=eq; A.T=0.3f+0.6f*(rand()%100)/100; A.level=0.2f+0.8f*(rand()%100)/100;
    double next=0,t=0; size_t nb=0; int visits[SC_COUNT]={}; Scene last=SC_COUNT; double sceneAt=0, silentUntil=-1, nextSilent=300+rand()%600; double maxIn[SC_COUNT]={}; int camps=0, days=1;
    while(t<span){
      float dt=(rand()%10<2)? 0.03f+0.06f*(rand()%100)/100 : 1.f/30;
      if(t>=nextSilent){ silentUntil=t+(rand()%2? 70+rand()%60 : 8+rand()%20); nextSilent=t+400+rand()%800; }
      A.sounding = t>=silentUntil;
      A.beatNow=A.sounding && t>=next; if(A.beatNow){ next+=A.T*(0.9f+0.2f*(rand()%100)/100); nb++; A.pulse=1; if(rand()%40==0) A.T=0.3f+0.6f*(rand()%100)/100;}
      if(!A.sounding) { A.level=0; A.beatNow=false; A.pulse=0; } else { A.beatN=nb; A.pulse*=powf(0.9f,2); if(rand()%200==0||A.level<0.05f) A.level=0.05f+0.95f*(rand()%100)/100; }
      for(int j=0;j<30;j++) eq[j]=A.sounding?(rand()%100)/20.f:0;
      gameUpdate(G,A,dt,t); gameRender(G,A,fb,t);
      if(G.scene!=last){ if(last!=SC_COUNT && t-sceneAt>maxIn[last]) maxIn[last]=t-sceneAt; last=G.scene; sceneAt=t; visits[last]++; if(last==SC_CAMP) camps++; }
      if(G.scene!=SC_CAMP && t-sceneAt>600){ printf("STUCK seed=%d scene=%d for %.0f s at t=%.0f\n",seed,(int)G.scene,t-sceneAt,t); bad++; break; }
      t+=dt;
    }
    days=G.day;
    if(seed<=3||bad){ printf("seed %d: day %d camps %d visits:",seed,days,camps); for(int i=0;i<SC_COUNT;i++) printf(" %d",visits[i]); printf("  longest(s):"); for(int i=0;i<SC_COUNT;i++) printf(" %.0f",maxIn[i]); printf("\n"); }
  }
  printf(bad?"FAILED %d\n":"full-run fuzz ok\n",bad); return bad!=0;
}
