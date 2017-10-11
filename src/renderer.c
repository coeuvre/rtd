#include "renderer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <glad/glad.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

const char DRAW_TEXTURE_VERTEX_SHADER[] = {
#include "shaders/draw_texture.vert.gen"
};

const char DRAW_TEXTURE_FRAGMENT_SHADER[] = {
#include "shaders/draw_texture.frag.gen"
};

typedef struct DrawTextureVertexAttrib {
    F pos[3];
    F texCoord[2];
    F color[4];
    F tint[4];
} DrawTextureVertexAttrib;

typedef struct DrawTextureProgram {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint program;
    GLint MVPLocation;
} DrawTextureProgram;

struct RenderContext {
    T2 projection;
    float pointToPixel;
    DrawTextureProgram drawTextureProgram;
};

typedef struct GLTexture {
    GLuint id;
} GLTexture;

typedef struct FontInternal {
    void *buf;
    stbtt_fontinfo info;
} FontInternal;

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

static void SetupDrawTextureProgram(DrawTextureProgram *drawTextureProgram) {
    // Setup VAO
    glGenVertexArrays(1, &drawTextureProgram->vao);
    glGenBuffers(1, &drawTextureProgram->vbo);
    glGenBuffers(1, &drawTextureProgram->ebo);

    glBindVertexArray(drawTextureProgram->vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawTextureProgram->vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawTextureProgram->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, texCoord));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, color));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, tint));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    // Compile Program
    drawTextureProgram->program = CompileGLProgram(DRAW_TEXTURE_VERTEX_SHADER, DRAW_TEXTURE_FRAGMENT_SHADER);
    if (!drawTextureProgram->program) {
        exit(EXIT_FAILURE);
    }
    glUseProgram(drawTextureProgram->program);
    glUniform1i(glGetUniformLocation(drawTextureProgram->program, "texture0"), 0);
    drawTextureProgram->MVPLocation = glGetUniformLocation(drawTextureProgram->program, "MVP");

}

static void UploadImageToGPU(Texture *tex, const unsigned char *data, int width, int height, int stride, ImageChannel channel) {
    GLTexture *glTex = tex->internal;

    glGenTextures(1, &glTex->id);
    glBindTexture(GL_TEXTURE_2D, glTex->id);

    tex->actualWidth = (int) NextPow2F(width);
    tex->actualHeight = height;

    int texStride = 0;
    GLint numberOfPixels = 0;
    GLint internalFormat = 0;
    GLenum format = 0;

    switch (channel) {
        case IMAGE_CHANNEL_RGBA: {
            texStride = tex->actualWidth * 4;
            numberOfPixels = tex->actualWidth;

            internalFormat = GL_SRGB8_ALPHA8;
            format = GL_RGBA;
        } break;
        case IMAGE_CHANNEL_A: {
            texStride = (int) (CeilF(tex->actualWidth / 4.0f) * 4.0f);
            numberOfPixels = texStride;

            internalFormat = GL_R8;
            format = GL_RED;

            GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        } break;
    }

    assert(stride <= texStride);
    assert(tex->actualWidth % 2 == 0);
    assert(texStride % 4 == 0);

    size_t texBufLen = (size_t) texStride * tex->actualHeight;
    unsigned char *texBuf = malloc(texBufLen);
    memset(texBuf, 0, texBufLen);

    unsigned char *dstRow = texBuf;
    const unsigned char *srcRow = data + stride * (height - 1);

    // Flip image vertically
    for (int y = 0; y < height; ++y) {
        memcpy(dstRow, srcRow, (size_t) stride);
        dstRow += texStride;
        srcRow -= stride;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, numberOfPixels);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tex->actualWidth, tex->actualHeight, 0, format, GL_UNSIGNED_BYTE, texBuf);

    free(texBuf);
}


extern RenderContext *CreateRenderContext(int windowWidth, int windowHeight, int drawableWidth, int drawableHeight) {
    RenderContext *renderContext = malloc(sizeof(RenderContext));

    renderContext->pointToPixel = drawableWidth / windowWidth;

    renderContext->projection = DotT2(MakeT2FromTranslation(MakeV2(-1.0f, -1.0f)),
                                      MakeT2FromScale(MakeV2(1.0f / windowWidth * 2.0f, 1.0f / windowHeight * 2.0f)));

    glViewport(0, 0, drawableWidth, drawableHeight);

    glEnable(GL_BLEND);
    // Pre-multiplied alpha format
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    // Render at linear color space
    glEnable(GL_FRAMEBUFFER_SRGB);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    SetupDrawTextureProgram(&renderContext->drawTextureProgram);

    return renderContext;
}

extern void ClearDrawing(RenderContext *renderContext) {
    (void) renderContext;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

extern Texture *CreateTextureFromMemory(RenderContext *renderContext, const unsigned char *data, int width, int height, int stride, ImageChannel channel) {
    (void) renderContext;

    Texture *tex = malloc(sizeof(Texture));
    GLTexture *glTex = malloc(sizeof(struct GLTexture));
    tex->width = width;
    tex->height = height;
    tex->internal = glTex;

    UploadImageToGPU(tex, data, width, height, stride, channel);

    return tex;
}

extern void DestroyTexture(RenderContext *renderContext, Texture **ptr) {
    (void) renderContext;

    Texture *texture = *ptr;
    GLTexture *glTexture = texture->internal;

    glDeleteTextures(1, &glTexture->id);

    free(glTexture);
    free(texture);

    *ptr = NULL;
}

extern void drawTexture(RenderContext *renderContext, BBox2 dstBBox, Texture *tex, BBox2 srcBBox, V4 color, V4 tint) {
    GLTexture *glTex = tex->internal;

    V2 texSize = MakeV2(tex->actualWidth, tex->actualHeight);
    BBox2 texBBox = MakeBBox2(HadamardDivV2(srcBBox.min, texSize), HadamardDivV2(srcBBox.max, texSize));
    DrawTextureVertexAttrib vertices[] = {
            dstBBox.max.x, dstBBox.max.y, 0.0f, texBBox.max.x, texBBox.max.y, color.r, color.g, color.b, color.a, tint.r, tint.g, tint.b, tint.a,   // top right
            dstBBox.max.x, dstBBox.min.y, 0.0f, texBBox.max.x, texBBox.min.y, color.r, color.g, color.b, color.a, tint.r, tint.g, tint.b, tint.a,   // bottom right
            dstBBox.min.x, dstBBox.min.y, 0.0f, texBBox.min.x, texBBox.min.y, color.r, color.g, color.b, color.a, tint.r, tint.g, tint.b, tint.a,   // bottom left
            dstBBox.min.x, dstBBox.max.y, 0.0f, texBBox.min.x, texBBox.max.y, color.r, color.g, color.b, color.a, tint.r, tint.g, tint.b, tint.a,   // top left
    };

    unsigned int indices[] = {  // note that we start from 0!
            0, 1, 3,   // first triangle
            1, 2, 3    // second triangle
    };

    glBindBuffer(GL_ARRAY_BUFFER, renderContext->drawTextureProgram.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderContext->drawTextureProgram.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glTex->id);

    glUseProgram(renderContext->drawTextureProgram.program);
    GLM4 MVP = MakeGLM4FromT2(renderContext->projection);
    glUniformMatrix4fv(renderContext->drawTextureProgram.MVPLocation, 1, GL_FALSE, MVP.m);

    glBindVertexArray(renderContext->drawTextureProgram.vao);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

extern Font *LoadFont(RenderContext *renderContext, const char *filename) {
    (void) renderContext;

    FILE *file = fopen(filename, "rb");

    if (!file) {
        printf("Failed to load font: %s\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    size_t size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    Font *font = malloc(sizeof(Font));
    font->name = filename;

    FontInternal *fontInternal = malloc(sizeof(FontInternal));
    font->internal = fontInternal;

    fontInternal->buf = malloc(size);

    fread(fontInternal->buf, 1, size, file);
    fclose(file);

    stbtt_InitFont(&fontInternal->info, fontInternal->buf, 0);

    return font;
}

extern void drawText(RenderContext *renderContext, Font *font, float size, float x, float y, const char *text, V4 color) {
    FontInternal *fontInternal = font->internal;
    stbtt_fontinfo *info = &fontInternal->info;

    float scale = stbtt_ScaleForPixelHeight(info, size * renderContext->pointToPixel);
//
//    int ascentInt, descentInt, lineGap;
//    stbtt_GetFontVMetrics(font, &ascentInt, &descentInt, &lineGap);
//
//    float ascent = ascentInt * scale;
//    float descent = descentInt * scale;

    for (int i = 0; i < strlen(text); ++i) {
        int codePoint = text[i];

        int width, height, xoff, yoff;
        unsigned char *bitmap = stbtt_GetCodepointBitmap(info, scale, scale, codePoint, &width, &height, &xoff, &yoff);
        if (bitmap != NULL) {
            Texture *texture = CreateTextureFromMemory(renderContext, bitmap, width, height, width, IMAGE_CHANNEL_A);
            drawTexture(renderContext, MakeBBox2MinSize(MakeV2(x + xoff / renderContext->pointToPixel,
                                                               y - (height + yoff) / renderContext->pointToPixel),
                                                        MakeV2(width / renderContext->pointToPixel,
                                                               height / renderContext->pointToPixel)),
                        texture, MakeBBox2FromTexture(texture), color, ZeroV4());
            DestroyTexture(renderContext, &texture);
            stbtt_FreeBitmap(bitmap, 0);
        }

        int axInt;
        stbtt_GetCodepointHMetrics(info, codePoint, &axInt, 0);
        float ax = axInt * scale / renderContext->pointToPixel;

        x += ax;

        int kernInt = stbtt_GetCodepointKernAdvance(info, codePoint, text[i + 1]);
        float kern = kernInt * scale / renderContext->pointToPixel;

        x += kern;
    }
}