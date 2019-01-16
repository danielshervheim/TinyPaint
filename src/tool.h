//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef TOOL_H_
#define TOOL_H_

#include <gdk/gdk.h>  // GdkRGBA
#include "pixel_buffer.h"  // PixelBuffer

typedef enum tooltype {
    PENCIL,
    BRUSH,
    MARKER,
    SPRAYCAN,
    FLOODFILL,
    ERASER
} ToolType;

// todo: this is updated when the tool changes, or the radius or color changes
typedef struct tool {
    ToolType tooltype;
    int radius;
    GdkRGBA color;
    double *mask;

    // if true, tool is applied when the user is not moving the mouse
    int applyWhenStationary;

    // if true, tool is only applied on stroke start (not stroke continue or end)
    int isStamp;

    // 1.0 fills every pixel between the current and previous mouse positions
    // 0.0 fills no pixel between the current and previous mouse positions
    double fillRate;
} Tool;



void tool_apply_to_pixelbuffer(Tool *tool, PixelBuffer *buffer, int x, int y);

void tool_reset_parameters(Tool *tool);

void tool_update_mask(Tool *tool);

Tool tool_new();

#endif  // TOOL_H_
