//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "pixel_buffer.h"



PixelBuffer pixelbuffer_new(int width, int height) {
    PixelBuffer tmp;
    tmp.width = width;
    tmp.height = height;
    tmp.data = malloc(sizeof(GdkRGBA) * width * height);
    return tmp;
}

void pixelbuffer_destroy(PixelBuffer *buf) {
    buf->width = -1;
    buf->height = -1;
    free(buf->data);
}

PixelBuffer pixelbuffer_copy(PixelBuffer *original) {
    PixelBuffer copy = pixelbuffer_new(original->width, original->height);
    for (int y = 0; y < copy.height; y++) {
        for (int x = 0; x < copy.width; x++) {
            pixelbuffer_set_pixel(&copy, x, y, pixelbuffer_get_pixel(original, x, y));
        }
    }
    copy.backgroundColor = original->backgroundColor;
    return copy;
}

void pixelbuffer_set_pixel(PixelBuffer *buf, int x, int y, GdkRGBA color) {
    buf->data[y*buf->width+x] = color;
}

GdkRGBA pixelbuffer_get_pixel(PixelBuffer *buf, int x, int y) {
    return buf->data[y*buf->width+x];
}

void pixelbuffer_set_all_pixels(PixelBuffer *buf, GdkRGBA color) {
    for (int x = 0; x < buf->width; x++) {
        for (int y = 0; y < buf->height; y++) {
            pixelbuffer_set_pixel(buf, x, y, color);
        }
    }
    buf->backgroundColor = color;
}
