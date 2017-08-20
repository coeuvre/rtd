#include "renderer.h"

#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>

struct Renderer {
    int width;
    int height;

    size_t ntextures;
    Texture *textures[1024];
};

struct Texture {
    void *pixels;
    int width;
    int stride;
    int height;
    int type;
    GLuint texid;
};

Renderer *InitRenderer(int width, int height) {
    Renderer *renderer = malloc(sizeof(Renderer));

    renderer->ntextures = 0;

    glViewport(0, 0, width, height);

    return renderer;
}

void ClearScreen(Renderer *renderer, float r, float g, float a, float b) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}
