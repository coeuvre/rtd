#ifndef RTD_TIME_H
#define RTD_TIME_H

#include <stdint.h>

typedef uint64_t Tick;

extern Tick GetCurrentTick(void);
extern float TickToSecond(Tick tick);

typedef struct FPSCounter {
    Tick lastTick;
    float duration;
    int counter;
    int fps;
} FPSCounter;

extern void InitFPSCounter(FPSCounter *fpsCounter);
extern void CountOneFrame(FPSCounter *fpsCounter);

#endif // RTD_TIME_H