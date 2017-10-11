#include "time.h"

#include <SDL2/SDL.h>

extern Tick GetCurrentTick(void) {
    return SDL_GetPerformanceCounter();
}

extern double TickToSecond(Tick tick) {
    return tick * 1.0f / SDL_GetPerformanceFrequency();
}

extern void InitFPSCounter(FPSCounter *fpsCounter) {
    fpsCounter->lastTick = GetCurrentTick();
    fpsCounter->duration = 0.0f;
    fpsCounter->counter = 0;
    fpsCounter->fps = 0;
}

extern void CountOneFrame(FPSCounter *fpsCounter) {
    Tick currentTick = GetCurrentTick();
    Tick deltaTick = currentTick - fpsCounter->lastTick;
    fpsCounter->lastTick = currentTick;

    float cost = (float) TickToSecond(deltaTick);
    fpsCounter->duration += cost;
    fpsCounter->counter += 1;
    if (fpsCounter->duration >= 1.0f) {
        fpsCounter->fps = fpsCounter->counter;
        fpsCounter->duration -= 1.0f;
        fpsCounter->counter = 0;
    }
}