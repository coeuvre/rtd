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

static int UploadImageToGPU(Image *image, GLuint *tex) {
    unsigned char *data = malloc(image->stride * image->height);

    // Flip image vertically
    unsigned char *dstRow = data;
    const unsigned char *srcRow = image->data + image->stride * (image->height - 1);

    for (int y = 0; y < image->height; ++y) {
        memcpy(dstRow, srcRow, image->stride);
        dstRow += image->stride;
        srcRow -= image->stride;
    }

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint internalFormat = 0;
    GLenum format = 0;

    switch (image->channel) {
        case IMAGE_CHANNEL_RGBA:
            assert(image->stride == 4 * image->width);
            internalFormat = GL_SRGB8_ALPHA8;
            format = GL_RGBA;
            break;
        case IMAGE_CHANNEL_A: {
            assert(image->stride == image->width);
            internalFormat = GL_R8;
            format = GL_RED;

            // Pre-multiplied alpha format
            GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        } break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image->width, image->height, 0, format, GL_UNSIGNED_BYTE, data);

    free(data);

    return 0;
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) offsetof(VertexAttrib, texCoord));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VertexAttrib), (void *) offsetof(VertexAttrib, color));
    glEnableVertexAttribArray(2);

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

extern Texture *LoadTextureFromImage(RenderContext *renderContext, Image *image) {
    (void) renderContext;

    GLTexture *glTex = malloc(sizeof(struct GLTexture));

    if (UploadImageToGPU(image, &glTex->id) != 0) {
        printf("Failed to upload image %s to GPU", image->name);
        free(glTex);
        return NULL;
    }

    Texture *tex = malloc(sizeof(Texture));
    tex->width = image->width;
    tex->height = image->height;
    tex->internal = glTex;

    return tex;
}


extern void drawTexture(RenderContext *context, BBox2 dstBBox, Texture *tex, BBox2 srcBBox, V4 tint) {
    GLTexture *glTex = tex->internal;

    V2 invTexSize = MakeV2(1.0f / tex->width, 1.0f / tex->height);
    BBox2 texBBox = MakeBBox2(HadamardV2(srcBBox.min, invTexSize), HadamardV2(srcBBox.max, invTexSize));
    VertexAttrib vertices[] = {
            dstBBox.max.x, dstBBox.max.y, 0.0f, texBBox.max.x, texBBox.max.y, tint.r, tint.g, tint.b, tint.a,   // top right
            dstBBox.max.x, dstBBox.min.y, 0.0f, texBBox.max.x, texBBox.min.y, tint.r, tint.g, tint.b, tint.a,   // bottom right
            dstBBox.min.x, dstBBox.min.y, 0.0f, texBBox.min.x, texBBox.min.y, tint.r, tint.g, tint.b, tint.a,   // bottom left
            dstBBox.min.x, dstBBox.max.y, 0.0f, texBBox.min.x, texBBox.max.y, tint.r, tint.g, tint.b, tint.a,   // top left
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
    glBindTexture(GL_TEXTURE_2D, glTex->id);

    glUseProgram(context->program);
    GLM4 MVP = MakeGLM4FromT2(context->projection);
    glUniformMatrix4fv(context->MVPLocation, 1, GL_FALSE, MVP.m);

    glBindVertexArray(context->vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}