#include <stdio.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#include "cgmath.h"
#include "window.h"
#include "renderer.h"
#include "time.h"

typedef struct GameContext {
    Window *window;
    RenderContext *rc;
    FPSCounter fpsCounter;
    int isRunning;

    Font *font;
    Texture *texBackground;
} GameContext;


static void SetupGame(GameContext *c) {
    SDL_Init(0);

    c->window = CreateGameWindow("RTD", WINDOW_WIDTH, WINDOW_HEIGHT);
    c->rc = CreateRenderContext(WINDOW_WIDTH, WINDOW_HEIGHT, c->window->pointToPixel);
    c->texBackground = LoadTexture(c->rc, "assets/scene1.png");

#ifdef PLATFORM_WIN32
    char *font = "C:/Windows/Fonts/Arial.ttf";
#else
    char *font = "/Library/Fonts/Arial.ttf";
#endif
    c->font = LoadFont(c->rc, font);
}

static void ProcessSystemEvent(GameContext *c) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                c->isRunning = 0;
                break;
            }

            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    c->isRunning = 0;
                }

                break;
            }

            default: break;
        }
    }
}

static void Update(GameContext *context) {
}

static void Render(GameContext *c) {
    RenderContext *rc = c->rc;

    ClearDrawing(rc);

    DrawTexture(rc, MakeBBox2FromTexture(c->texBackground),
                c->texBackground, MakeBBox2FromTexture(c->texBackground),
                OneV4(), ZeroV4());

    float fontSize = 20.0f;
    float ascent = GetFontAscent(rc, c->font, fontSize);
    float lineHeight = GetFontLineHeight(rc, c->font, fontSize);
    float y = rc->height - ascent;
#define BUF_SIZE 128
    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "FPS: %d", c->fpsCounter.fps);
    DrawLineText(rc, c->font, fontSize, 0.0f, y,
                 buf, MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
    DrawLineText(rc, c->font, fontSize, 0.0f, y - lineHeight,
                 "Hello World! HLIJijgklWAV", MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
}

static int RunMainLoop(GameContext *c) {
    c->isRunning = 1;

    InitFPSCounter(&c->fpsCounter);
    while (c->isRunning) {
        ProcessSystemEvent(c);
        Update(c);
        Render(c);
        SwapWindowBuffers(c->window);
        CountOneFrame(&c->fpsCounter);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    GameContext context = {0};

    SetupGame(&context);

    return RunMainLoop(&context);
}
