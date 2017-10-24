#ifndef RTD_GAME_H
#define RTD_GAME_H

#include "window.h"
#include "renderer.h"
#include "time.h"
#include "cgmath.h"
#include "game_node.h"

typedef struct GameContext {
    Window *window;
    RenderContext *rc;
    FPSCounter fpsCounter;
    int isRunning;

    GameNodeTreeWalker gameNodeTreeWalker;

    Font *font;

    GameNode *rootNode;
} GameContext;

#endif // RTD_GAME_H