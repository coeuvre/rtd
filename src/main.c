#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <glad/glad.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

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

    int numBakedchars;
    stbtt_bakedchar *bakedchars;

    Texture *texBackground;
    Texture *texFont;
} GameContext;


static int LoadFont(GameContext *context) {
#ifdef RTD_WIN32
    char *path = "C:/Windows/Fonts/Arial.ttf";
#else
    char *path = "/Library/Fonts/Arial.ttf";
#endif

    FILE *font = fopen(path, "rb");
    if (!font) {
        printf("Failed to load font file\n");
        return 1;
    }
    fseek(font, 0, SEEK_END);
    size_t size = (size_t)ftell(font);
    fseek(font, 0, SEEK_SET);

    unsigned char *buf = malloc(size);
    fread(buf, 1, size, font);

#define BITMAP_WIDTH 512
#define BITMAP_HEIGHT 512
    unsigned char bitmap[BITMAP_WIDTH * BITMAP_HEIGHT];
    int firstChars = 32;
    context->numBakedchars = 95; // ASCII 32..126
    context->bakedchars = malloc(sizeof(stbtt_bakedchar) * context->numBakedchars);

    if (stbtt_BakeFontBitmap(buf, 0, 32.0f, bitmap, BITMAP_WIDTH, BITMAP_HEIGHT,
                             firstChars, context->numBakedchars,
                             context->bakedchars) <= 0) {
        printf("Failed to bake font bitmap\n");
        return 1;
    }

    free(buf);
    fclose(font);

    Image *image = LoadImageFromGrayBitmap(BITMAP_WIDTH, BITMAP_HEIGHT, BITMAP_WIDTH, bitmap);
    context->texFont = LoadTextureFromImage(context->renderContext, image);
    DestroyImage(&image);

    return 0;
}

static int SetupWindowAndOpenGL(GameContext *context) {
    // Use this function to set an OpenGL window attribute before window creation.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    context->window = SDL_CreateWindow(
        "RTD", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);

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

    return 0;
}

static int SetupGame(GameContext *context) {
    SDL_Init(SDL_INIT_VIDEO);

    if (SetupWindowAndOpenGL(context) != 0) {
        return 1;
    }

    context->renderContext = CreateRenderContext(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (context->renderContext == NULL) {
        return 1;
    }

    context->texBackground = LoadTexture(context->renderContext, "assets/scene1.png");
    if (context->texBackground == NULL) {
        return 1;
    }

    if (LoadFont(context) != 0) {
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

    drawTexture(context->renderContext, MakeBBox2(MakeV2(0.0f, 0.0f), MakeV2(WINDOW_WIDTH, WINDOW_HEIGHT)),
                context->texBackground, MakeBBox2FromTexture(context->texBackground), COLOR_WHITE);

    drawTexture(context->renderContext, MakeBBox2FromTexture(context->texFont),
                context->texFont, MakeBBox2FromTexture(context->texFont), MakeV4(1.0f, 0.0f, 0.0f, 1.0f));
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
