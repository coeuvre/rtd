#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <glad/glad.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#include "cgmath.h"

const char VERTEX_SHADER[] = {
    #include "shaders/test.vert.gen"
};

const char FRAGMENT_SHADER[] = {
    #include "shaders/test.frag.gen"
};

static GLuint CompileGLShader(GLenum type, const char *source) {
    GLuint result = glCreateShader(type);

    glShaderSource(result, 1, &source, 0);

    glCompileShader(result);

    GLint success;
    glGetShaderiv(result, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        char buf[512];
        glGetShaderInfoLog(result, sizeof(buf), 0, buf);
        printf("Failed to compile shader: %s\n", buf);
        result = 0;
    }

    return result;
}

static GLuint CompileGLProgram(const char *vss, const char *fss) {
    GLuint result = 0;

    GLuint vs = CompileGLShader(GL_VERTEX_SHADER, vss);
    if (vs) {
        GLuint fs = CompileGLShader(GL_FRAGMENT_SHADER, fss);
        if (fs) {
            result = glCreateProgram();
            glAttachShader(result, vs);
            glAttachShader(result, fs);
            glLinkProgram(result);

            GLint success;
            glGetProgramiv(result, GL_LINK_STATUS, &success);
            if (success != GL_TRUE) {
                char buf[512];
                glGetProgramInfoLog(result, sizeof(buf), 0, buf);
                printf("Failed to link program: %s\n", buf);
                result = 0;
            }
        }
    }

    return result;
}

typedef struct Image {
    const char *filename;
    unsigned char *data;
    int width;
    int height;
    int comp;
} Image;

typedef struct Texture {
    GLuint id;
    int width;
    int height;
} Texture;

static int LoadImage(Image *image, const char *filename) {
    image->filename = filename;
    image->data = stbi_load(filename, &image->width, &image->height, &image->comp, 0);
    if (image->data != NULL) {
        return 0;
    } else {
        printf("Failed to load image %s", filename);
        return 1;
    }
}

static void UploadImageToGPU(Image *image, GLuint *tex) {
    size_t rowLength = (size_t) image->comp * (size_t) image->width;
    void *data = malloc(image->height * rowLength);

    unsigned char *dst = data;
    unsigned char *src = image->data + (image->height - 1) * rowLength;

    for (int y = image->height - 1; y >= 0; --y) {
        memcpy(dst, src, rowLength);
        dst += rowLength;
        src -= rowLength;
    }

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 image->width, image->height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    free(data);
}

typedef struct VertexAttrib {
    F pos[3];
    F texCoord[2];
} VertexAttrib;

typedef struct RenderContext {
    T2 projection;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint program;
    GLint MVPLocation;
} RenderContext;

typedef struct GameContext {
    SDL_Window *window;
    SDL_GLContext *glContext;

    int isRunning;

    int numBakedchars;
    stbtt_bakedchar *bakedchars;

    Texture texBackground;
    GLuint ftex;

    RenderContext renderContext;
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
    context->numBakedchars = 95; // ASCII 32..126
    context->bakedchars = malloc(sizeof(stbtt_bakedchar) * context->numBakedchars);

    if (stbtt_BakeFontBitmap(buf, 0, 32.0f, bitmap, BITMAP_WIDTH, BITMAP_HEIGHT,
                             first_chars, context->numBakedchars,
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

static void DestroyImage(Image *image) {
    stbi_image_free(image->data);
}

static int SetupRenderContext(RenderContext *context, int width, int height) {
    context->projection = DotT2(MakeT2FromTranslation(MakeV2(-1.0f, -1.0f)),
                                MakeT2FromScale(MakeV2(1.0f / width * 2.0f, 1.0f / height * 2.0f)));

    glViewport(0, 0, width, height);

    // Setup VAO
    glGenVertexArrays(1, &context->vao);
    glGenBuffers(1, &context->vbo);
    glGenBuffers(1, &context->ebo);

    glBindVertexArray(context->vao);
    glBindBuffer(GL_ARRAY_BUFFER, context->vbo);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->ebo);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) offsetof(VertexAttrib, texCoord));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Compile Program
    context->program = CompileGLProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!context->program) {
        return 1;
    }
    glUseProgram(context->program);
    glUniform1i(glGetUniformLocation(context->program, "texture0"), 0);
    context->MVPLocation = glGetUniformLocation(context->program, "MVP");

    return 0;
}

static int LoadTexture(Texture *tex, const char *filename) {
    Image image;

    if (LoadImage(&image, filename) != 0) {
        return 1;
    }

    UploadImageToGPU(&image, &tex->id);
    tex->width = image.width;
    tex->height = image.height;

    DestroyImage(&image);

    return 0;
}

static void drawTexture(RenderContext *context, BBox2 dstBBox, Texture *tex) {
    VertexAttrib vertices[] = {
        dstBBox.max.x, dstBBox.max.y, 0.0f, 1.0f, 1.0f,   // top right
        dstBBox.max.x, dstBBox.min.y, 0.0f, 1.0f, 0.0f,   // bottom right
        dstBBox.min.x, dstBBox.min.y, 0.0f, 0.0f, 0.0f,   // bottom left
        dstBBox.min.x, dstBBox.max.y, 0.0f, 0.0f, 1.0f,   // top left
    };

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };

    glBindBuffer(GL_ARRAY_BUFFER, context->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    glUseProgram(context->program);
    GLM4 MVP = MakeGLM3FromT2(context->projection);
    glUniformMatrix4fv(context->MVPLocation, 1, GL_FALSE, MVP.m);

    glBindVertexArray(context->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static int SetupGame(GameContext *context) {
    SDL_Init(SDL_INIT_VIDEO);

    if (SetupWindowAndOpenGL(context) != 0) {
        return 1;
    }

    if (SetupRenderContext(&context->renderContext, WINDOW_WIDTH, WINDOW_HEIGHT) != 0) {
        return 1;
    }

    LoadTexture(&context->texBackground, "./assets/scene1.png");

    if (LoadFont(context) != 0) {
        return 1;
    }

    return 0;
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

static void Update(GameContext *context, GameState *state) {
}

static void Render(GameContext *context, GameState *state) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawTexture(&context->renderContext,
                MakeBBox(MakeV2(0.0f, 0.0f), MakeV2(WINDOW_WIDTH, WINDOW_HEIGHT)),
                &context->texBackground);
}

static void WaitForNextFrame(GameContext *context) {
    (void)context;

    SDL_GL_SwapWindow(context->window);
}

static int RunMainLoop(GameContext *context) {
    GameState state = {0};

    context->isRunning = 1;

    while (context->isRunning) {
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
