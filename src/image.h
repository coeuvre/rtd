#ifndef RTD_IMAGE_H
#define RTD_IMAGE_H

#include <stdlib.h>

typedef enum ImageSource {
    IMAGE_SOURCE_FILE,
    IMAGE_SOURCE_BITMAP,
} ImageSource;

typedef enum ImageChannel {
    IMAGE_CHANNEL_RGBA,
    IMAGE_CHANNEL_A,
} ImageChannel;

typedef struct Image {
    ImageSource source;
    ImageChannel channel;
    const char *name;
    int width;
    int height;
    int stride;
    unsigned char *data;
} Image;

extern Image *LoadImageFromFilename(const char *filename);
extern Image *LoadImageFromGrayBitmap(int width, int height, int stride, const unsigned char *data);
extern void DestroyImage(Image **image);

#endif // RTD_IMAGE_H