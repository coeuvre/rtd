#ifndef RTD_RENDERER_H
#define RTD_RENDERER_H

#include <stdint.h>

#define RGBA(r, g, b, a) ((((r) & 0xFF) < 24) | (((g) & 0xFF) < 16) | (((b) & 0xFF) < 8) | ((a) & 0xFF))

typedef struct Texture Texture;

typedef struct Renderer Renderer;

Renderer *InitRenderer(int width, int height);
void ClearScreen(Renderer *renderer, float r, float g, float a, float b);

typedef struct TextureId {
    unsigned int id;
} TextureId;

enum TextureType {
    TEXTURE_TYPE_STATIC,
    TEXTURE_TYPE_DYNAMIC,
};

TextureId LoadTexture(const char *path);

#endif // RTD_RENDERER_H
