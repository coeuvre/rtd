#include "renderer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <glad/glad.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

const char DRAW_TEXTURE_VERTEX_SHADER[] = {
#include "shader/draw_texture.vert.gen"
};

const char DRAW_TEXTURE_FRAGMENT_SHADER[] = {
#include "shader/draw_texture.frag.gen"
};

typedef struct DrawTextureVertexAttrib {
    F transform0[3];
    F transform1[3];
    F transform2[3];
    F pos[2];
    F texCoord[2];
    F color[4];
} DrawTextureVertexAttrib;

typedef struct DrawTextureProgram {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint program;
    GLint MVPLocation;
} DrawTextureProgram;



const char DRAW_RECT_VERTEX_SHADER[] = {
#include "shader/draw_rect.vert.gen"
};

const char DRAW_RECT_FRAGMENT_SHADER[] = {
#include "shader/draw_rect.frag.gen"
};

typedef struct DrawRectVertexAttrib {
    F transform0[3];
    F transform1[3];
    F transform2[3];
    F pos[2];
    F texCoord[2];
    F color[4];
    F roundRadius[2];
    F thickness[2];
    F borderColor[4];
} DrawRectVertexAttrib;

typedef struct DrawRectProgram {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint program;
    GLint MVPLocation;
} DrawRectProgram;

typedef struct RenderContextInternal {
    DrawTextureProgram drawTextureProgram;
    DrawRectProgram drawRectProgram;
} RenderContextInternal;

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
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawTextureProgram->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, transform0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, transform1));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, transform2));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, pos));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, texCoord));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(DrawTextureVertexAttrib), (void *) offsetof(DrawTextureVertexAttrib, color));
    glEnableVertexAttribArray(5);

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

static void SetupDrawRectProgram(DrawRectProgram *drawRectProgram) {
    // Setup VAO
    glGenVertexArrays(1, &drawRectProgram->vao);
    glGenBuffers(1, &drawRectProgram->vbo);
    glGenBuffers(1, &drawRectProgram->ebo);

    glBindVertexArray(drawRectProgram->vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawRectProgram->vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawRectProgram->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, transform0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, transform1));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, transform2));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, pos));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, texCoord));
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, color));
    glEnableVertexAttribArray(5);

    glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, roundRadius));
    glEnableVertexAttribArray(6);

    glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, thickness));
    glEnableVertexAttribArray(7);

    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(DrawRectVertexAttrib), (void *) offsetof(DrawRectVertexAttrib, borderColor));
    glEnableVertexAttribArray(8);

    glBindVertexArray(0);

    // Compile Program
    drawRectProgram->program = CompileGLProgram(DRAW_RECT_VERTEX_SHADER, DRAW_RECT_FRAGMENT_SHADER);
    if (!drawRectProgram->program) {
        exit(EXIT_FAILURE);
    }
    glUseProgram(drawRectProgram->program);
    drawRectProgram->MVPLocation = glGetUniformLocation(drawRectProgram->program, "MVP");
}

// TODO(coeuvre): Allow to define filter mode
static void UploadImageToGPU(Texture *tex, const unsigned char *data, int width, int height, int stride, ImageChannel channel) {
    GLTexture *glTex = tex->internal;

    glGenTextures(1, &glTex->id);
    glBindTexture(GL_TEXTURE_2D, glTex->id);

    tex->actualWidth = (int) NextPow2F((float) width);
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, numberOfPixels);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tex->actualWidth, tex->actualHeight, 0, format, GL_UNSIGNED_BYTE, texBuf);

    free(texBuf);
}


extern RenderContext *CreateRenderContext(int width, int height, float pointToPixel) {
    RenderContext *rc = malloc(sizeof(RenderContext));
    RenderContextInternal *renderContextInternal = malloc(sizeof(RenderContextInternal));
    rc->internal = renderContextInternal;

    rc->width = (float) width;
    rc->height = (float) height;
    rc->pointToPixel = pointToPixel;
    rc->pixelToPoint = 1.0f / pointToPixel;
    rc->drawCallCount = 0;
    rc->projection = DotT2(MakeT2FromTranslation(MakeV2(-1.0f, -1.0f)),
                           MakeT2FromScale(MakeV2(1.0f / width * 2.0f,
                                                  1.0f / height * 2.0f)));
    rc->camera = IdentityT2();

    glViewport(0, 0, (GLsizei) (width * pointToPixel), (GLsizei) (height * pointToPixel));

    glEnable(GL_BLEND);
    // Pre-multiplied alpha format
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    // Render at linear color space
    glEnable(GL_FRAMEBUFFER_SRGB);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    SetupDrawTextureProgram(&renderContextInternal->drawTextureProgram);
    SetupDrawRectProgram(&renderContextInternal->drawRectProgram);

    return rc;
}

extern void ClearDrawing(RenderContext *rc) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    rc->drawCallCount = 0;
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

extern void DrawTexture(RenderContext *rc, T2 transform, BBox2 dstBBox,
                        Texture *tex, BBox2 srcBBox, V4 color) {
    if (!tex) {
        return;
    }

    RenderContextInternal *renderContextInternal = rc->internal;
    GLTexture *glTex = tex->internal;

    V2 texSize = MakeV2((float) tex->actualWidth, (float) tex->actualHeight);
    BBox2 texBBox = MakeBBox2(HadamardDivV2(srcBBox.min, texSize), HadamardDivV2(srcBBox.max, texSize));
    GLM3 t = MakeGLM3FromT2(transform);
    DrawTextureVertexAttrib vertices[] = {
        t.m[0], t.m[1], t.m[2], t.m[3], t.m[4], t.m[5], t.m[6], t.m[7], t.m[8], dstBBox.max.x, dstBBox.max.y, texBBox.max.x, texBBox.max.y, color.r, color.g, color.b, color.a,   // top right
        t.m[0], t.m[1], t.m[2], t.m[3], t.m[4], t.m[5], t.m[6], t.m[7], t.m[8], dstBBox.max.x, dstBBox.min.y, texBBox.max.x, texBBox.min.y, color.r, color.g, color.b, color.a,   // bottom right
        t.m[0], t.m[1], t.m[2], t.m[3], t.m[4], t.m[5], t.m[6], t.m[7], t.m[8], dstBBox.min.x, dstBBox.min.y, texBBox.min.x, texBBox.min.y, color.r, color.g, color.b, color.a,   // bottom left
        t.m[0], t.m[1], t.m[2], t.m[3], t.m[4], t.m[5], t.m[6], t.m[7], t.m[8], dstBBox.min.x, dstBBox.max.y, texBBox.min.x, texBBox.max.y, color.r, color.g, color.b, color.a,   // top left
    };

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3,   // second triangle
    };

    glBindBuffer(GL_ARRAY_BUFFER, renderContextInternal->drawTextureProgram.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderContextInternal->drawTextureProgram.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glTex->id);

    glUseProgram(renderContextInternal->drawTextureProgram.program);
    GLM3 MVP = MakeGLM3FromT2(DotT2(rc->projection, rc->camera));
    glUniformMatrix3fv(renderContextInternal->drawTextureProgram.MVPLocation, 1, GL_FALSE, MVP.m);

    glBindVertexArray(renderContextInternal->drawTextureProgram.vao);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    rc->drawCallCount++;
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

extern float GetFontAscent(RenderContext *renderContext, Font *font, float size) {
    if (!font) {
        return 0.0f;
    }

    FontInternal *fontInternal = font->internal;
    stbtt_fontinfo *info = &fontInternal->info;

    float scale = stbtt_ScaleForPixelHeight(info, size * renderContext->pointToPixel);

    int ascentInt;
    stbtt_GetFontVMetrics(info, &ascentInt, 0, 0);

    float ascent = ascentInt * scale * renderContext->pixelToPoint;
    return ascent;
}

extern float GetFontLineHeight(RenderContext *renderContext, Font *font, float size) {
    if (!font) {
        return 0.0f;
    }

    FontInternal *fontInternal = font->internal;
    stbtt_fontinfo *info = &fontInternal->info;

    float scale = stbtt_ScaleForPixelHeight(info, size * renderContext->pointToPixel);

    int ascentInt, descentInt, lineGapInt;
    stbtt_GetFontVMetrics(info, &ascentInt, &descentInt, &lineGapInt);

    float ascent = ascentInt * scale * renderContext->pixelToPoint;
    float descent = descentInt * scale * renderContext->pixelToPoint;
    float lineGap = lineGapInt * scale * renderContext->pixelToPoint;

    return ascent - descent + lineGap;
}

extern void DrawLineText(RenderContext *rc, Font *font, float size, float x, float y, const char *text, V4 color) {
    if (!font) {
        return;
    }

    FontInternal *fontInternal = font->internal;
    stbtt_fontinfo *info = &fontInternal->info;

    float scale = stbtt_ScaleForPixelHeight(info, size * rc->pointToPixel);

    for (size_t i = 0; i < strlen(text); ++i) {
        int codePoint = text[i];

        int width, height, xOff, yOff;
        unsigned char *bitmap = stbtt_GetCodepointBitmap(info, scale, scale, codePoint, &width, &height, &xOff, &yOff);
        if (bitmap != NULL) {
            Texture *texture = CreateTextureFromMemory(rc, bitmap, width, height, width, IMAGE_CHANNEL_A);
            DrawTexture(rc, IdentityT2(),
                        MakeBBox2MinSize(MakeV2(x + xOff * rc->pixelToPoint,
                                                y - (height + yOff) * rc->pixelToPoint),
                                         MakeV2(width * rc->pixelToPoint,
                                                height * rc->pixelToPoint)),
                        texture, MakeBBox2FromTexture(texture), color);
            DestroyTexture(rc, &texture);
            stbtt_FreeBitmap(bitmap, 0);
        }

        int axInt;
        stbtt_GetCodepointHMetrics(info, codePoint, &axInt, 0);
        float ax = axInt * scale * rc->pixelToPoint;

        x += ax;

        int kernInt = stbtt_GetCodepointKernAdvance(info, codePoint, text[i + 1]);
        float kern = kernInt * scale * rc->pixelToPoint;

        x += kern;
    }
}

extern void DrawRect(RenderContext *rc, T2 transform, BBox2 bbox, V4 color, F roundRadius, F thickness, V4 borderColor) {
    RenderContextInternal *renderContextInternal = rc->internal;

    V2 size = GetBBox2Size(bbox);
    V2 normalizedRoundRadius = DivV2(roundRadius, size);
    V2 normalizedThickness = DivV2(thickness, size);
    GLM3 t = MakeGLM3FromT2(transform);
    DrawRectVertexAttrib vertices[] = {
        t.m[0], t.m[1], t.m[2], t.m[3], t.m[4], t.m[5], t.m[6], t.m[7], t.m[8], bbox.max.x, bbox.max.y, 1.0f, 1.0f, color.r, color.g, color.b, color.a, normalizedRoundRadius.x, normalizedRoundRadius.y, normalizedThickness.x, normalizedThickness.y, borderColor.r, borderColor.g, borderColor.b, borderColor.a,  // top right
        t.m[0], t.m[1], t.m[2], t.m[3], t.m[4], t.m[5], t.m[6], t.m[7], t.m[8], bbox.max.x, bbox.min.y, 1.0f, 0.0f, color.r, color.g, color.b, color.a, normalizedRoundRadius.x, normalizedRoundRadius.y, normalizedThickness.x, normalizedThickness.y, borderColor.r, borderColor.g, borderColor.b, borderColor.a,  // bottom right
        t.m[0], t.m[1], t.m[2], t.m[3], t.m[4], t.m[5], t.m[6], t.m[7], t.m[8], bbox.min.x, bbox.min.y, 0.0f, 0.0f, color.r, color.g, color.b, color.a, normalizedRoundRadius.x, normalizedRoundRadius.y, normalizedThickness.x, normalizedThickness.y, borderColor.r, borderColor.g, borderColor.b, borderColor.a,  // bottom left
        t.m[0], t.m[1], t.m[2], t.m[3], t.m[4], t.m[5], t.m[6], t.m[7], t.m[8], bbox.min.x, bbox.max.y, 0.0f, 1.0f, color.r, color.g, color.b, color.a, normalizedRoundRadius.x, normalizedRoundRadius.y, normalizedThickness.x, normalizedThickness.y, borderColor.r, borderColor.g, borderColor.b, borderColor.a,  // top left
    };

    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,   // first triangle
        1, 2, 3,   // second triangle
    };

    glBindBuffer(GL_ARRAY_BUFFER, renderContextInternal->drawRectProgram.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderContextInternal->drawRectProgram.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    glUseProgram(renderContextInternal->drawRectProgram.program);
    GLM3 MVP = MakeGLM3FromT2(DotT2(rc->projection, rc->camera));
    glUniformMatrix3fv(renderContextInternal->drawRectProgram.MVPLocation, 1, GL_FALSE, MVP.m);

    glBindVertexArray(renderContextInternal->drawRectProgram.vao);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    rc->drawCallCount++;
}