//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef PIXEL_BUFFER_H_
#define PIXEL_BUFFER_H_

#include <gdk/gdk.h>  // GdkRGBA

typedef struct pixelbuffer {
    int width;
    int height;
    GdkRGBA *data;
    GdkRGBA backgroundColor;
} PixelBuffer;

void pixelbuffer_set_pixel(PixelBuffer *buf, int x, int y, GdkRGBA color);

GdkRGBA pixelbuffer_get_pixel(PixelBuffer *buf, int x, int y);

void pixelbuffer_set_all_pixels(PixelBuffer *buf, GdkRGBA color);

PixelBuffer pixelbuffer_new(int width, int height);

void pixelbuffer_destroy(PixelBuffer *buf);

PixelBuffer pixelbuffer_copy(PixelBuffer *original);


#endif  // PIXEL_BUFFER_H_
