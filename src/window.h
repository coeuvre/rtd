#ifndef RTD_WINDOW_H
#define RTD_WINDOW_H

typedef struct Window {
    const char *title;
    int width;
    int height;
    int drawableWidth;
    int drawableHeight;
    void *internal;
} Window;

extern Window *CreateGameWindow(const char *title, int width, int height);
extern void SwapWindowBuffers(Window *window);

#endif // RTD_WINDOW_H