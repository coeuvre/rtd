#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <glad/glad.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#include "cgmath.h"
#include "renderer.h"

typedef struct GameState {
    int isRunning;
} GameState;

typedef struct GameContext {
    SDL_Window *window;
    SDL_GLContext *glContext;

    RenderContext *renderContext;
    GameState gameState;

    Font *font;

    Texture *texBackground;
} GameContext;

static int SetupWindowAndOpenGL(GameContext *context) {
    SDL_Init(SDL_INIT_VIDEO);

    // Use this function to set an OpenGL window attribute before window creation.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    context->window = SDL_CreateWindow(
        "RTD", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!context->window) {
        printf("Failed to create SDL window: %s\n", SDL_GetError());
        return 1;
    }

    context->glContext = SDL_GL_CreateContext(context->window);

    SDL_GL_SetSwapInterval(1);

    if (gladLoadGLLoader(&SDL_GL_GetProcAddress) == 0) {
        printf("Failed to load OpenGL\n");
        return 1;
    }

#ifdef GLAD_DEBUG
    printf("Glad Debug Mode\n");
#endif

    if (GLVersion.major < 3) {
        printf("Your system doesn't support OpenGL >= 3!\n");
        return 1;
    }

    printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    int width, height;
    SDL_GL_GetDrawableSize(context->window, &width, &height);
    printf("Drawable size: %dx%d", width, height);

    context->renderContext = CreateRenderContext(width, height);
    if (context->renderContext == NULL) {
        return 1;
    }

    return 0;
}

static int SetupGame(GameContext *context) {
    if (SetupWindowAndOpenGL(context) != 0) {
        return 1;
    }

    context->texBackground = LoadTexture(context->renderContext, "assets/scene1.png");
    if (context->texBackground == NULL) {
        return 1;
    }

#ifdef RTD_WIN32
    char *font = "C:/Windows/Fonts/Arial.ttf";
#else
    char *font = "/Library/Fonts/Arial.ttf";
#endif
    context->font = LoadFont(context->renderContext, font);
    if (context->font == NULL) {
        return 1;
    }

    return 0;
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
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawTexture(context->renderContext, MakeBBox2FromTexture(context->texBackground),
                context->texBackground, MakeBBox2FromTexture(context->texBackground),
                OneV4(), ZeroV4());

    drawText(context->renderContext, context->font, 32.0f, 0.0f, WINDOW_HEIGHT - 40, "Hello World! HLIJijgklWAV", MakeV4(1.0f, 1.0f, 1.0f, 1.0f));
}

static void WaitForNextFrame(GameContext *context) {
    SDL_GL_SwapWindow(context->window);
}

static int RunMainLoop(GameContext *context) {
    GameState *state = &context->gameState;

    state->isRunning = 1;

    while (state->isRunning) {
        ProcessSystemEvent(context);
        Update(context);
        Render(context);
        WaitForNextFrame(context);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    GameContext context = {0};

    if (SetupGame(&context) != 0) {
        return 1;
    }

    return RunMainLoop(&context);
}
