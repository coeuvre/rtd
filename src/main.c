#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <glad/glad.h>

#include "renderer.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

const char VERTEX_SHADER[] = {
    #include "shaders/test.vert.gen"
};

const char FRAGMENT_SHADER[] = {
    #include "shaders/test.frag.gen"
};

static GLuint CompileShader(GLenum type, const char *source) {
    GLuint result = glCreateShader(type);

    glShaderSource(result, 1, &source, 0);

    glCompileShader(result);

    GLint success;
    glGetShaderiv(result, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        result = 0;

        char buf[512];
        glGetShaderInfoLog(result, sizeof(buf), 0, buf);
        printf("Failed to compile shader: %s\n", buf);
    }

    return result;
}

static GLuint CompileProgram(const char *vertex_shader_source,
                             const char *fragment_shader_source) {
    GLuint result = 0;

    GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_shader_source);
    if (vertex_shader) {
        GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_shader_source);
        if (fragment_shader) {
            result = glCreateProgram();
            glAttachShader(result, vertex_shader);
            glAttachShader(result, fragment_shader);
            glLinkProgram(result);

            GLint success;
            glGetProgramiv(result, GL_LINK_STATUS, &success);
            if (success != GL_TRUE) {
                result = 0;

                char buf[512];
                glGetProgramInfoLog(result, sizeof(buf), 0, buf);
                printf("Failed to link program: %s\n", buf);
            }
        }
    }

    return result;
}

typedef struct GameContext {
    SDL_Window *window;
    SDL_GLContext *glcontext;
    Renderer *renderer;

    int is_running;

    int num_bakedchars;
    stbtt_bakedchar *bakedchars;

    GLuint ftex;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint program;
} GameContext;

typedef struct GameState {
    int placeholder;
} GameState;

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
    int first_chars = 32;
    context->num_bakedchars = 95; // ASCII 32..126
    context->bakedchars = malloc(sizeof(stbtt_bakedchar) * context->num_bakedchars);

    if (stbtt_BakeFontBitmap(buf, 0, 32.0f, bitmap, BITMAP_WIDTH, BITMAP_HEIGHT,
                             first_chars, context->num_bakedchars,
                             context->bakedchars) <= 0) {
        printf("Failed to bake font bitmap\n");
        return 1;
    }

    free(buf);
    fclose(font);

    glGenTextures(1, &context->ftex);
    glBindTexture(GL_TEXTURE_2D, context->ftex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, BITMAP_WIDTH, BITMAP_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

    return 0;
}

static int InitWindowAndOpenGL(GameContext *context) {
    // Use this function to set an OpenGL window attribute before window creation.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    context->window = SDL_CreateWindow(
            "RTD",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            SDL_WINDOW_OPENGL
    );

    if (!context->window) {
        printf("Failed to create SDL window: %s\n", SDL_GetError());
        return 1;
    }

    context->glcontext = SDL_GL_CreateContext(context->window);

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

    if (InitWindowAndOpenGL(context) != 0) {
        return 1;
    }

    context->renderer = InitRenderer(WINDOW_WIDTH, WINDOW_HEIGHT);

#if 0
    if (LoadFont(context) != 0) {
        return 1;
    }

    float vertices[] = {
        // positions          // colors           // texture coords
        0.5f, 0.5f, 0.0f,     1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
        0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,   1.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f,    1.0f, 1.0f, 0.0f,   0.0f, 1.0f,  // top left
    };

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };

    glGenVertexArrays(1, &context->vao);
    glGenBuffers(1, &context->vbo);
    glGenBuffers(1, &context->ebo);

    glBindVertexArray(context->vao);
    glBindBuffer(GL_ARRAY_BUFFER, context->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    context->program = CompileProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!context->program) {
        return 1;
    }
    glUseProgram(context->program);
    glUniform1i(glGetUniformLocation(context->program, "texture0"), 0);
#endif
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
    ClearScreen(context->renderer, 1.0f, 1.0f, 1.0f, 1.0f);
    // glBindTexture(GL_TEXTURE_2D, context->ftex);

    // glUseProgram(context->program);
    // glBindVertexArray(context->vao);
    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void WaitForNextFrame(GameContext *context) {
    (void)context;

    SDL_GL_SwapWindow(context->window);
}

static int RunMainLoop(GameContext *context) {
    GameState state = {0};

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

    GameContext context = {0};

    if (SetupGame(&context) != 0) {
        return 1;
    }

    return RunMainLoop(&context);
}
