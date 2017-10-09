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

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

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

typedef enum ImageSource {
    IMAGE_SOURCE_FILE,
    IMAGE_SOURCE_BITMAP,
} ImageSource;

typedef enum ImageChannel {
    IMAGE_CHANNEL_RGBA,
    IMAGE_CHANNEL_A,
} ImageChannel;

// Use float to store each channel of bitmap, ranging from 0.0 to 1.0.
// The image is processed with sRGB decode and pre-multiplied alpha.
typedef struct Image {
    ImageSource source;
    ImageChannel channel;
    const char *name;
    int width;
    int height;
    size_t stride;
    float *data;
} Image;

typedef struct Texture {
    GLuint id;
    int width;
    int height;
} Texture;

static inline BBox2 MakeBBox2FromTexture(Texture *tex) {
    BBox2 result = MakeBBox2(MakeV2(0.0f, 0.0f), MakeV2(tex->width, tex->height));
    return result;
}

static void ProcessImage(float *dst, const unsigned char *src, int width, int height, size_t dstStride, size_t srcStride, ImageChannel channel) {
    float gamma = 2.2f;

    // Flip image vertically
    unsigned char *dstRow = (unsigned char *) dst;
    const unsigned char *srcRow = src + srcStride * (height - 1);

    for (int y = 0; y < height; ++y) {
        float *dstColor = (float *) dstRow;
        const unsigned char *srcColor = srcRow;

        for (int x = 0; x < width; ++x) {
            switch (channel) {
                case IMAGE_CHANNEL_RGBA: {
                    unsigned char srcR = *srcColor++;
                    unsigned char srcG = *srcColor++;
                    unsigned char srcB = *srcColor++;
                    unsigned char srcA = *srcColor++;

                    float r = powf(srcR / 255.0f, gamma);
                    float g = powf(srcG / 255.0f, gamma);
                    float b = powf(srcB / 255.0f, gamma);
                    float a = srcA / 255.0f;

                    // pre-multiply alpha
                    r = r * a;
                    g = g * a;
                    b = b * a;
                    a = a * 1.0f;

                    *dstColor++ = r;
                    *dstColor++ = g;
                    *dstColor++ = b;
                    *dstColor++ = a;
                } break;

                case IMAGE_CHANNEL_A: {
                    unsigned char srcA = *srcColor++;
                    float a = srcA / 255.0f;
                    *dstColor++ = a;
                } break;
            }
        }

        dstRow += dstStride;
        srcRow -= srcStride;
    }
}

static int LoadImage(Image *image, const char *filename) {
    image->source = IMAGE_SOURCE_FILE;
    image->channel = IMAGE_CHANNEL_RGBA;
    image->name = filename;

    unsigned char *data = stbi_load(filename, &image->width, &image->height, NULL, 4);
    if (data == NULL) {
        printf("Failed to load image %s\n", filename);
        return 1;
    }

    image->stride = sizeof(float) * 4 * image->width;
    image->data = malloc(image->stride * image->height);

    ProcessImage(image->data, data, image->width, image->height, image->stride, (size_t) 4 * image->width, image->channel);

    stbi_image_free(data);

    return 0;
}

static int LoadImageFromAlphaBitmap(Image *image, int width, int height, size_t stride, const unsigned char *data) {
    if (stride < width) {
        printf("Invalid bitmap data: stride < width\n");
        return 1;
    }
    image->source = IMAGE_SOURCE_BITMAP;
    image->channel = IMAGE_CHANNEL_A;
    image->name = "ALPHA BITMAP";
    image->width = width;
    image->height = height;
    image->stride = sizeof(float) * image->width;

    image->data = malloc(image->stride * height);

    ProcessImage(image->data, data, width, height, image->stride, stride, IMAGE_CHANNEL_A);

    return 0;
}

static void DestroyImage(Image *image) {
    free(image->data);
}

static int UploadImageToGPU(Image *image, GLuint *tex) {
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint internalFormat = 0;
    GLenum format = 0;

    switch (image->channel) {
        case IMAGE_CHANNEL_RGBA:
            assert(image->stride == sizeof(float) * 4 * image->width);
            internalFormat = GL_RGBA8;
            format = GL_RGBA;
            break;
        case IMAGE_CHANNEL_A: {
            assert(image->stride == sizeof(float) * image->width);
            internalFormat = GL_R8;
            format = GL_RED;

            // Pre-multiplied alpha format
            GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        } break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image->width, image->height, 0, format, GL_FLOAT, image->data);

    return 0;
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

typedef struct GameState {
    int isRunning;
} GameState;

typedef struct GameContext {
    SDL_Window *window;
    SDL_GLContext *glContext;

    RenderContext renderContext;
    GameState gameState;

    int numBakedchars;
    stbtt_bakedchar *bakedchars;

    Texture texBackground;
    Texture texFont;
} GameContext;

static int LoadTextureFromImage(Texture *tex, Image *image) {
    if (UploadImageToGPU(image, &tex->id) != 0) {
        printf("Failed to upload image %s to GPU", image->name);
        return 1;
    }

    tex->width = image->width;
    tex->height = image->height;

    return 0;
}

static int LoadTexture(Texture *tex, const char *filename) {
    Image image;

    if (LoadImage(&image, filename) != 0) {
        return 1;
    }

    if (LoadTextureFromImage(tex, &image) != 0) {
        DestroyImage(&image);
        return 1;
    }

    DestroyImage(&image);

    return 0;
}

static void drawTexture(RenderContext *context, BBox2 dstBBox, Texture *tex, BBox2 srcBBox) {
    V2 invTexSize = MakeV2(1.0f / tex->width, 1.0f / tex->height);
    BBox2 texBBox = MakeBBox2(HadamardV2(srcBBox.min, invTexSize), HadamardV2(srcBBox.max, invTexSize));
    VertexAttrib vertices[] = {
            dstBBox.max.x, dstBBox.max.y, 0.0f, texBBox.max.x, texBBox.max.y,   // top right
            dstBBox.max.x, dstBBox.min.y, 0.0f, texBBox.max.x, texBBox.min.y,   // bottom right
            dstBBox.min.x, dstBBox.min.y, 0.0f, texBBox.min.x, texBBox.min.y,   // bottom left
            dstBBox.min.x, dstBBox.max.y, 0.0f, texBBox.min.x, texBBox.max.y,   // top left
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
    GLM4 MVP = MakeGLM4FromT2(context->projection);
    glUniformMatrix4fv(context->MVPLocation, 1, GL_FALSE, MVP.m);

    glBindVertexArray(context->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

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

    Image image;
    LoadImageFromAlphaBitmap(&image, BITMAP_WIDTH, BITMAP_HEIGHT, BITMAP_WIDTH, bitmap);
    LoadTextureFromImage(&context->texFont, &image);
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

static int SetupRenderContext(RenderContext *context, int width, int height) {
    context->projection = DotT2(MakeT2FromTranslation(MakeV2(-1.0f, -1.0f)),
                                MakeT2FromScale(MakeV2(1.0f / width * 2.0f, 1.0f / height * 2.0f)));

    glViewport(0, 0, width, height);

    glEnable(GL_BLEND);
    // Pre-multiplied alpha format
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    // Render at linear color space
    glEnable(GL_FRAMEBUFFER_SRGB);

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

static int SetupGame(GameContext *context) {
    SDL_Init(SDL_INIT_VIDEO);

    if (SetupWindowAndOpenGL(context) != 0) {
        return 1;
    }

    if (SetupRenderContext(&context->renderContext, WINDOW_WIDTH, WINDOW_HEIGHT) != 0) {
        return 1;
    }

    if (LoadTexture(&context->texBackground, "assets/scene1.png") != 0) {
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

    drawTexture(&context->renderContext, MakeBBox2(MakeV2(0.0f, 0.0f), MakeV2(WINDOW_WIDTH, WINDOW_HEIGHT)),
                &context->texBackground, MakeBBox2FromTexture(&context->texBackground));

    drawTexture(&context->renderContext, MakeBBox2FromTexture(&context->texFont),
                &context->texFont, MakeBBox2FromTexture(&context->texFont));
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
