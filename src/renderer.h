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

typedef struct Font {
    const char *name;
    void *internal;
} Font;

extern RenderContext *CreateRenderContext(int width, int height);

extern Texture *CreateTextureFromMemory(RenderContext *renderContext, const unsigned char *data, int width, int height, int stride, ImageChannel channel);
extern void DestroyTexture(RenderContext *renderContext, Texture **texture);
extern void drawTexture(RenderContext *renderContext, BBox2 dstBBox, Texture *tex, BBox2 srcBBox, V4 color, V4 tint);

extern Font *LoadFont(RenderContext *renderContext, const char *filename);
extern void drawText(RenderContext *renderContext, Font *font, float size, float x, float y, const char *text, V4 color);

static inline BBox2 MakeBBox2FromTexture(Texture *tex) {
    BBox2 result = MakeBBox2(MakeV2(0.0f, 0.0f), MakeV2(tex->width, tex->height));
    return result;
}

static inline Texture *CreateTextureFromImage(RenderContext *renderContext, Image *image) {
    return CreateTextureFromMemory(renderContext, image->data, image->width, image->height, image->stride,
                                   image->channel);
}

static inline Texture *LoadTexture(RenderContext *renderContext, const char *filename) {
    Image *image = LoadImageFromFilename(filename);

    if (image == NULL) {
        return NULL;
    }

    Texture *tex = CreateTextureFromImage(renderContext, image);

    DestroyImage(&image);

    return tex;
}

#endif // RTD_RENDERER_H