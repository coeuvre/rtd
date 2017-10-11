#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#include "cgmath.h"
#include "window.h"
#include "renderer.h"
#include "time.h"

typedef struct GameContext {
    Window *window;
    RenderContext *renderContext;
    FPSCounter fpsCounter;
    int isRunning;

    Font *font;
    Texture *texBackground;
} GameContext;


static void SetupGame(GameContext *context) {
    SDL_Init(0);

    context->window = CreateGameWindow("RTD", WINDOW_WIDTH, WINDOW_HEIGHT);
    context->renderContext = CreateRenderContext(context->window->width, context->window->height, context->window->drawableWidth / context->window->width);

    context->texBackground = LoadTexture(context->renderContext, "assets/scene1.png");

#ifdef RTD_WIN32
    char *font = "C:/Windows/Fonts/Arial.ttf";
#else
    char *font = "/Library/Fonts/Arial.ttf";
#endif
    context->font = LoadFont(context->renderContext, font);
}

static void ProcessSystemEvent(GameContext *context) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                context->isRunning = 0;
                break;
            }

            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    context->isRunning = 0;
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

    float fontSize = 20.0f;
    float ascent = GetFontAscent(renderContext, context->font, fontSize);
    float lineHeight = GetFontLineHeight(renderContext, context->font, fontSize);
    float y = renderContext->height - ascent;
#define BUF_SIZE 128
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "FPS: %d", context->fpsCounter.fps);
    drawText(renderContext, context->font, fontSize, 0.0f, y,
             buf, MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
    drawText(renderContext, context->font, fontSize, 0.0f, y - lineHeight,
             "Hello World! HLIJijgklWAV", MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
}

static int RunMainLoop(GameContext *context) {
    context->isRunning = 1;

    InitFPSCounter(&context->fpsCounter);
    while (context->isRunning) {
        ProcessSystemEvent(context);
        Update(context);
        Render(context);
        SwapWindowBuffers(context->window);
        CountOneFrame(&context->fpsCounter);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    GameContext context = {0};

    SetupGame(&context);

    return RunMainLoop(&context);
}
