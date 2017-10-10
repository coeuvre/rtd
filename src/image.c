#include "image.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static inline unsigned char encodeSRGB(float val) {
    return (unsigned char) (powf(val, 1.0f / 2.2f) * 255.0f + 0.5f);
}

static inline float decodeSRGB(unsigned char val) {
    return powf(val / 255.0f, 2.2f);
}

static void PreMultiplyAlpha(Image *image) {
    unsigned char *row = image->data;

    for (int y = 0; y < image->height; ++y) {
        unsigned char *src = row;
        unsigned char *dst = row;

        for (int x = 0; x < image->width; ++x) {
            assert(image->channel == IMAGE_CHANNEL_RGBA);

            unsigned char srcR = *src++;
            unsigned char srcG = *src++;
            unsigned char srcB = *src++;
            unsigned char srcA = *src++;

            float r = decodeSRGB(srcR);
            float g = decodeSRGB(srcG);
            float b = decodeSRGB(srcB);
            float a = srcA / 255.0f;

            // pre-multiply alpha
            r = r * a;
            g = g * a;
            b = b * a;
            a = a * 1.0f;

            *dst++ = encodeSRGB(r);
            *dst++ = encodeSRGB(g);
            *dst++ = encodeSRGB(b);
            *dst++ = (unsigned char) (a * 255.0f + 0.5f);
        }

        row += image->stride;
    }
}

extern Image *LoadImageFromFilename(const char *filename) {
    Image *image = malloc(sizeof(Image));

    image->source = IMAGE_SOURCE_FILE;
    image->channel = IMAGE_CHANNEL_RGBA;
    image->name = filename;
    image->data = stbi_load(filename, &image->width, &image->height, NULL, 4);
    if (image->data == NULL) {
        printf("Failed to load image %s\n", filename);
        free(image);
        return NULL;
    }
    image->stride = (size_t) 4 * image->width;

    PreMultiplyAlpha(image);

    return image;
}

extern Image *LoadImageFromGrayBitmap(int width, int height, size_t stride, const unsigned char *data) {
    assert(stride >= width);

    Image *image = malloc(sizeof(Image));
    image->source = IMAGE_SOURCE_BITMAP;
    image->channel = IMAGE_CHANNEL_A;
    image->name = "ALPHA BITMAP";
    image->width = width;
    image->height = height;
    image->stride = (size_t) image->width;

    image->data = malloc(image->stride * height);
    memcpy(image->data, data, image->stride * height);

    return image;
}

extern void DestroyImage(Image **ptr) {
    Image *image = *ptr;
    switch (image->source) {
        case IMAGE_SOURCE_FILE:
            stbi_image_free(image->data);
            break;
        case IMAGE_SOURCE_BITMAP:
            free(image->data);
            break;
    }

    *ptr = NULL;
}