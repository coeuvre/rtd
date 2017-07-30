#include <stdio.h>

#include <SDL2/SDL.h>

#include <glad/glad.h>

#ifdef GLAD_DEBUG
void pre_gl_call(const char *name, void *funcptr, int len_args, ...) {
    printf("Calling: %s (%d arguments)\n", name, len_args);
}
#endif

typedef struct GameContext {
    SDL_Window *sdl_window;
    SDL_GLContext sdl_gl_context;
    int is_running;
} GameContext;

typedef struct GameState {

} GameState;

static int SetupGame(GameContext *context) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    context->sdl_window = SDL_CreateWindow(
        "RTD",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL
    );

    if (context->sdl_window == NULL) {
        printf("Failed to create SDL window: %s\n", SDL_GetError());
        return 1;
    }

    context->sdl_gl_context = SDL_GL_CreateContext(context->sdl_window);

    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader(&SDL_GL_GetProcAddress)) {
        printf("Failed to load OpenGL\n");
        return 1;
    }

#ifdef GLAD_DEBUG
    glad_set_pre_callback(pre_gl_call);
    glad_debug_glClear = glad_glClear;
    printf("Glad Debug Mode\n");
#endif

    if (GLVersion.major < 3) {
        printf("Your system doesn't support OpenGL >= 3!\n");
        return 1;
    }

    printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION),
           glGetString(GL_SHADING_LANGUAGE_VERSION));

    return 0;
}

static void ProcessSystemEvent(GameContext *context) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                context->is_running = 0;
                break;
            }

            case SDL_KEYDOWN: {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    context->is_running = 0;
                }

                break;
            }

            default: break;
        }
    }
}

static void Update(GameContext *context, GameState *state) {
}

static void Render(GameContext *context, GameState *state) {
    glClear(GL_COLOR_BUFFER_BIT);
}

static void WaitForNextFrame(GameContext *context) {
    (void)context;

    SDL_GL_SwapWindow(context->sdl_window);
}

static int RunMainLoop(GameContext *context) {
    GameState state = {};

    context->is_running = 1;

    while (context->is_running) {
        ProcessSystemEvent(context);
        Update(context, &state);
        Render(context, &state);
        WaitForNextFrame(context);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    GameContext context = {};

    if (SetupGame(&context) != 0) {
        return 1;
    }

    return RunMainLoop(&context);
}