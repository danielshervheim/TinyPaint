//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef FLOOD_FILL_H_
#define FLOOD_FILL_H_

#include <gdk/gdk.h>  // GdkRGBA
#include "pixel_buffer.h"  // PixelBuffer

/* Fills all pixels of target color connected to (x, y) with replacement color. */
void flood_fill(PixelBuffer *buffer, int x, int y, GdkRGBA target, GdkRGBA replacement);

#endif  // FLOOD_FILL_H_
