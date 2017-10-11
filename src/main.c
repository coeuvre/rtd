#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#include "cgmath.h"
#include "window.h"
#include "renderer.h"

typedef struct GameState {
    int isRunning;
} GameState;

typedef struct GameContext {
    Window *window;
    RenderContext *renderContext;
    GameState gameState;

    Font *font;
    Texture *texBackground;
} GameContext;


static void SetupGame(GameContext *context) {
    SDL_Init(0);

    context->window = CreateGameWindow("RTD", WINDOW_WIDTH, WINDOW_HEIGHT);
    context->renderContext = CreateRenderContext(context->window->width, context->window->height, context->window->drawableWidth / context->window->width);

    context->texBackground = LoadTexture(context->renderContext, "assets/scene1.png");
    if (context->texBackground == NULL) {
        exit(EXIT_FAILURE);
    }

#ifdef RTD_WIN32
    char *font = "C:/Windows/Fonts/Arial.ttf";
#else
    char *font = "/Library/Fonts/Arial.ttf";
#endif
    context->font = LoadFont(context->renderContext, font);
    if (context->font == NULL) {
        exit(EXIT_FAILURE);
    }
}

static void ProcessSystemEvent(GameContext *context) {
    GameState *state = &context->gameState;

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                state->isRunning = 0;
                break;
            }

            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    state->isRunning = 0;
                }

                break;
            }

            default: break;
        }
    }
}

static void Update(GameContext *context) {
}

static void Render(GameContext *context) {
    RenderContext *renderContext = context->renderContext;

    ClearDrawing(renderContext);

    drawTexture(renderContext, MakeBBox2FromTexture(context->texBackground),
                context->texBackground, MakeBBox2FromTexture(context->texBackground),
                OneV4(), ZeroV4());

    drawText(renderContext, context->font, 20.0f, 0.0f, WINDOW_HEIGHT - 20.0f,
             "Hello World! HLIJijgklWAV", MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
}

static int RunMainLoop(GameContext *context) {
    GameState *state = &context->gameState;

    state->isRunning = 1;

    while (state->isRunning) {
        ProcessSystemEvent(context);
        Update(context);
        Render(context);
        SwapWindowBuffers(context->window);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    GameContext context = {0};

    SetupGame(&context);

    return RunMainLoop(&context);
}
