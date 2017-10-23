#include "window.h"

#ifdef PLATFORM_WIN32
#include <windows.h>

extern int _stdcall SetProcessDPIAware(void);
#endif

#include <stdlib.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#include <glad/glad.h>

typedef struct WindowInternal {
    SDL_Window *sdlWindow;
    SDL_GLContext *sdlGLContext;
} WindowInternal;

static void SetupWindowDPI(Window *window) {
#ifdef PLATFORM_WIN32
#else
    WindowInternal *windowInternal = window->internal;
    int drawableWidth, drawableHeight;
    SDL_GL_GetDrawableSize(windowInternal->sdlWindow, &drawableWidth, &drawableHeight);
    window->pointToPixel = drawableWidth * 1.0f / window->width;
#endif
}

extern Window *CreateGameWindow(const char *title, int width, int height) {
    Window *window = malloc(sizeof(Window));
    WindowInternal *windowInternal = malloc(sizeof(WindowInternal));
    window->title = title;
    window->width = width;
    window->height = height;
    window->internal = windowInternal;

#ifdef PLATFORM_WIN32
    SetProcessDPIAware();

    HDC screen = GetDC(0);
    int dpiX = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC (0, screen);

    window->pointToPixel = dpiX / 96.0f;
    window->width = (int) (window->width * window->pointToPixel);
    window->height = (int) (window->height * window->pointToPixel);
#endif

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            printf("Failed to init subsystem SDL_INIT_VIDEO: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }
    }

    // Use this function to set an OpenGL window attribute before window creation.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    windowInternal->sdlWindow = SDL_CreateWindow(
            title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            window->width, window->height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!windowInternal->sdlWindow) {
        printf("Failed to create SDL window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    windowInternal->sdlGLContext = SDL_GL_CreateContext(windowInternal->sdlWindow);

    SDL_GL_SetSwapInterval(1);

    if (gladLoadGLLoader(&SDL_GL_GetProcAddress) == 0) {
        printf("Failed to load OpenGL\n");
        exit(EXIT_FAILURE);
    }

#ifdef GLAD_DEBUG
    printf("Glad Debug Mode\n");
#endif

    if (GLVersion.major < 3) {
        printf("Your system doesn't support OpenGL >= 3!\n");
        exit(EXIT_FAILURE);
    }

    printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    SetupWindowDPI(window);

    printf("Window size: %dx%d\n", window->width, window->height);
    printf("DPI Scale factor: %f\n", window->pointToPixel);

    return window;
}

extern void SwapWindowBuffers(Window *window) {
    WindowInternal *windowInternal = window->internal;
    SDL_GL_SwapWindow(windowInternal->sdlWindow);
}