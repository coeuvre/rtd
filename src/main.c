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

    stbtt_fontinfo font;

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
        printf("Failed to load font: %s\n", path);
        return 1;
    }
    fseek(font, 0, SEEK_END);
    size_t size = (size_t)ftell(font);
    fseek(font, 0, SEEK_SET);

    unsigned char *fontBuf = malloc(size);
    fread(fontBuf, 1, size, font);
    fclose(font);

    stbtt_InitFont(&context->font, fontBuf, 0);

#define BITMAP_WIDTH 555
#define BITMAP_HEIGHT 512
    unsigned char bitmap[BITMAP_WIDTH * BITMAP_HEIGHT];
    int firstChars = 32;
    context->numBakedchars = 95; // ASCII 32..126
    context->bakedchars = malloc(sizeof(stbtt_bakedchar) * context->numBakedchars);

    if (stbtt_BakeFontBitmap(fontBuf, 0, 32.0f, bitmap, BITMAP_WIDTH, BITMAP_HEIGHT,
                             firstChars, context->numBakedchars,
                             context->bakedchars) <= 0) {
        printf("Failed to bake font bitmap\n");
        return 1;
    }

    context->texFont = CreateTextureFromMemory(context->renderContext, bitmap, BITMAP_WIDTH, BITMAP_HEIGHT, BITMAP_WIDTH, IMAGE_CHANNEL_A);
//    float scale = stbtt_ScaleForPixelHeight(&context->font, 1000.0f);
//    int width, height, xoff, yoff;
//    unsigned char *bitmap = stbtt_GetCodepointBitmap(&context->font, scale, scale, 'A', &width, &height, &xoff, &yoff);
//    context->texFont = CreateTextureFromMemory(context->renderContext, bitmap, width, height, (size_t) width, IMAGE_CHANNEL_A);

    free(fontBuf);

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

static void drawText(RenderContext *renderContext, stbtt_fontinfo *font, float fontSize, float x, float y, const char *text, V4 color) {
    float scale = stbtt_ScaleForPixelHeight(font, fontSize);
//
//    int ascentInt, descentInt, lineGap;
//    stbtt_GetFontVMetrics(font, &ascentInt, &descentInt, &lineGap);
//
//    float ascent = ascentInt * scale;
//    float descent = descentInt * scale;

    for (int i = 0; i < strlen(text); ++i) {
        int codePoint = text[i];

        int width, height, xoff, yoff;
        unsigned char *bitmap = stbtt_GetCodepointBitmap(font, scale, scale, codePoint, &width, &height, &xoff, &yoff);
        if (bitmap != NULL) {
//            for (int v = 0; v < height; ++v) {
//                for (int u = 0; u < width; ++u) {
//                    putchar(" .:ioVM@"[bitmap[v * width + u] >> 5]);
//                }
//                putchar('\n');
//            }
            Texture *texture = CreateTextureFromMemory(renderContext, bitmap, width, height, width, IMAGE_CHANNEL_A);
            drawTexture(renderContext, MakeBBox2MinSize(MakeV2(x, y), MakeV2(width, height)),
                        texture, MakeBBox2FromTexture(texture), color, ZeroV4());
            DestroyTexture(renderContext, &texture);
            stbtt_FreeBitmap(bitmap, 0);
        }

        int axInt;
        stbtt_GetCodepointHMetrics(font, codePoint, &axInt, 0);
        float ax = axInt * scale;

        x += ax;

        int kernInt = stbtt_GetCodepointKernAdvance(font, codePoint, text[i + 1]);
        float kern = kernInt * scale;

        x += kern;
    }
}

static void Render(GameContext *context) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawTexture(context->renderContext, MakeBBox2(MakeV2(0.0f, 0.0f), MakeV2(WINDOW_WIDTH, WINDOW_HEIGHT)),
                context->texBackground, MakeBBox2FromTexture(context->texBackground),
                OneV4(), ZeroV4());


//    drawText(context->renderContext, &context->font, 32.0f, 0.0f, 0.0f, "Hello World!", MakeV4(1.0f, 1.0f, 0.0f, 1.0f));

    drawTexture(context->renderContext, MakeBBox2FromTexture(context->texFont),
                context->texFont, MakeBBox2FromTexture(context->texFont),
                MakeV4(1.0f, 0.0f, 0.0f, 1.0f), ZeroV4());
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
