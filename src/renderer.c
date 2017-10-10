#include "renderer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <glad/glad.h>

const char VERTEX_SHADER[] = {
#include "shaders/test.vert.gen"
};

const char FRAGMENT_SHADER[] = {
#include "shaders/test.frag.gen"
};

struct RenderContext {
    T2 projection;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint program;
    GLint MVPLocation;
};

typedef struct GLTexture {
    GLuint id;
} GLTexture;

typedef struct VertexAttrib {
    F pos[3];
    F texCoord[2];
    F color[4];
    F tint[4];
} VertexAttrib;

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

static void UploadImageToGPU(const unsigned char *data, int width, int height, int stride, ImageChannel channel, GLuint *tex) {
    unsigned char *processedData = malloc((size_t) stride * height);

    // Flip image vertically
    unsigned char *dstRow = processedData;
    const unsigned char *srcRow = data + stride * (height - 1);

    for (int y = 0; y < height; ++y) {
        memcpy(dstRow, srcRow, stride);
        dstRow += stride;
        srcRow -= stride;
    }

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint internalFormat = 0;
    GLenum format = 0;

    switch (channel) {
        case IMAGE_CHANNEL_RGBA:
            assert(stride == 4 * width);
            internalFormat = GL_SRGB8_ALPHA8;
            format = GL_RGBA;
            break;
        case IMAGE_CHANNEL_A: {
            assert(stride == width);
            internalFormat = GL_R8;
            format = GL_RED;

            GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        } break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, processedData);

    free(processedData);
}


extern RenderContext *CreateRenderContext(int width, int height) {
    RenderContext *context = malloc(sizeof(RenderContext));

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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) offsetof(VertexAttrib, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) offsetof(VertexAttrib, texCoord));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) offsetof(VertexAttrib, color));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) offsetof(VertexAttrib, tint));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    // Compile Program
    context->program = CompileGLProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    if (!context->program) {
        free(context);
        return NULL;
    }
    glUseProgram(context->program);
    glUniform1i(glGetUniformLocation(context->program, "texture0"), 0);
    context->MVPLocation = glGetUniformLocation(context->program, "MVP");

    return context;
}

extern Texture *CreateTextureFromMemory(RenderContext *renderContext, const unsigned char *data, int width, int height, int stride, ImageChannel channel) {
    (void) renderContext;

    Texture *tex = malloc(sizeof(Texture));
    GLTexture *glTex = malloc(sizeof(struct GLTexture));
    tex->width = width;
    tex->height = height;
    tex->internal = glTex;

    UploadImageToGPU(data, width, height, stride, channel, &glTex->id);

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

    V2 texSize = MakeV2(tex->width, tex->height);
    BBox2 texBBox = MakeBBox2(HadamardDivV2(srcBBox.min, texSize), HadamardDivV2(srcBBox.max, texSize));
    VertexAttrib vertices[] = {
            dstBBox.max.x, dstBBox.max.y, 0.0f, texBBox.max.x, texBBox.max.y, color.r, color.g, color.b, color.a, tint.r, tint.g, tint.b, tint.a,   // top right
            dstBBox.max.x, dstBBox.min.y, 0.0f, texBBox.max.x, texBBox.min.y, color.r, color.g, color.b, color.a, tint.r, tint.g, tint.b, tint.a,   // bottom right
            dstBBox.min.x, dstBBox.min.y, 0.0f, texBBox.min.x, texBBox.min.y, color.r, color.g, color.b, color.a, tint.r, tint.g, tint.b, tint.a,   // bottom left
            dstBBox.min.x, dstBBox.max.y, 0.0f, texBBox.min.x, texBBox.max.y, color.r, color.g, color.b, color.a, tint.r, tint.g, tint.b, tint.a,   // top left
    };

    unsigned int indices[] = {  // note that we start from 0!
            0, 1, 3,   // first triangle
            1, 2, 3    // second triangle
    };

    glBindBuffer(GL_ARRAY_BUFFER, renderContext->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderContext->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glTex->id);

    glUseProgram(renderContext->program);
    GLM4 MVP = MakeGLM4FromT2(renderContext->projection);
    glUniformMatrix4fv(renderContext->MVPLocation, 1, GL_FALSE, MVP.m);

    glBindVertexArray(renderContext->vao);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}