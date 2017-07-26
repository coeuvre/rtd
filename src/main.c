#include <stdio.h>

#include <SDL2/SDL.h>

static int RunMainLoop(void) {
    int is_running = 1;

    SDL_Event event;
    while (is_running) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    is_running = 0;
                    break;
                }

                case SDL_KEYDOWN: {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        is_running = 0;
                    }

                    break;
                }

                default: break;
            }
            if (event.type == SDL_QUIT) {
                is_running = 0;
            }
        } else {

        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "RTD",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
    );

    if (window == NULL) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return 1;
    }

    return RunMainLoop();
}