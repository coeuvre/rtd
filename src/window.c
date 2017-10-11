#include "window.h"

#include <SDL2/SDL.h>

#include <glad/glad.h>

typedef struct WindowInternal {
    SDL_Window *sdlWindow;
    SDL_GLContext *sdlGLContext;
} WindowInternal;

extern Window *CreateGameWindow(const char *title, int width, int height) {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            printf("Failed to init subsystem SDL_INIT_VIDEO: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }
    }

    Window *window = malloc(sizeof(Window));
    WindowInternal *windowInternal = malloc(sizeof(WindowInternal));
    window->title = title;
    window->width = width;
    window->height = height;
    window->internal = windowInternal;

    // Use this function to set an OpenGL window attribute before window creation.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    windowInternal->sdlWindow = SDL_CreateWindow(
            title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

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

    SDL_GL_GetDrawableSize(windowInternal->sdlWindow, &window->drawableWidth, &window->drawableHeight);
    printf("Drawable size: %dx%d\n", window->drawableWidth, window->drawableHeight);

    return window;
}

extern void SwapWindowBuffers(Window *window) {
    WindowInternal *windowInternal = window->internal;
    SDL_GL_SwapWindow(windowInternal->sdlWindow);
}