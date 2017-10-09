#ifndef RTD_RENDERER_H
#define RTD_RENDERER_H

#include <stdlib.h>

#include "cgmath.h"
#include "image.h"

typedef struct RenderContext RenderContext;

typedef struct Texture {
    int width;
    int height;
    void *internal;
} Texture;

extern RenderContext *CreateRenderContext(int width, int height);
extern Texture *LoadTextureFromImage(RenderContext *renderContext, Image *image);
extern void drawTexture(RenderContext *context, BBox2 dstBBox, Texture *tex, BBox2 srcBBox);

static inline BBox2 MakeBBox2FromTexture(Texture *tex) {
    BBox2 result = MakeBBox2(MakeV2(0.0f, 0.0f), MakeV2(tex->width, tex->height));
    return result;
}

static inline Texture *LoadTexture(RenderContext *renderContext, const char *filename) {
    Image *image = LoadImage(filename);

    if (image == NULL) {
        return NULL;
    }

    Texture *tex = LoadTextureFromImage(renderContext, image);

    DestroyImage(&image);

    return tex;
}

#endif // RTD_RENDERER_H