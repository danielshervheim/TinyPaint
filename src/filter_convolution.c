//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "filter.h"

#include "kernel.h"
#include "utilities.h"

#include <gdk/gdk.h>  // GdkRGBA
#include <math.h>  //



//
// CONVOLUTION KERNEL creation methods
//

Kernel create_gaussian_blur_kernel(GaussianBlurParams *params) {
    Kernel tmp = kernel_new(params->radius);

    for (int y = 0; y < tmp.edgeLength; y++) {
        for (int x = 0; x < tmp.edgeLength; x++) {
            double distanceFromCenter = sqrt(pow(x - tmp.radius, 2.0) + pow(y - tmp.radius, 2.0));
            kernel_set_value(&tmp, x, y, double_gaussian(distanceFromCenter, tmp.radius));
        }
    }

    kernel_normalize(&tmp);
    return tmp;
}

Kernel create_motion_blur_kernel(MotionBlurParams *params) {
    Kernel tmp = kernel_new(params->radius);

    for (int y = 0; y < tmp.edgeLength; y++) {
        for (int x = 0; x < tmp.edgeLength; x++) {
            // set to 0 initially
            kernel_set_value(&tmp, x, y, 0.0);

            // calculate the offset, so the center value is at 0,0
            int u = x - tmp.radius;
            int v = y - tmp.radius;

            // a fudge factor to determine what percent of a pixel must be on the
            // line to consider it as part of the kernel
            double epsilon = 0.75;

            if (params->angle < 0.25*M_PI || params->angle > 0.75*M_PI) {
                if (double_abs(v - (u * tan(params->angle))) < epsilon) {
                    kernel_set_value(&tmp, x, y, 1.0);
                }
            }
            else {
                // solve for x rather than y, else limits of tan tend towards infinity
                if (double_abs(u - (v / tan(params->angle))) < epsilon) {
                    kernel_set_value(&tmp, x, y, 1.0);
                }
            }
        }
    }

    kernel_normalize(&tmp);
    return tmp;
}

Kernel create_sharpen_kernel(SharpenParams *params) {
    Kernel tmp = create_gaussian_blur_kernel((GaussianBlurParams *)params);

    kernel_scale(&tmp, -1.0);

    kernel_set_value(&tmp, tmp.radius, tmp.radius,
        kernel_get_value(&tmp, tmp.radius, tmp.radius) + 2);

    return tmp;
}

Kernel create_edge_detect_kernel() {
    Kernel tmp = kernel_new(1);

    // kernel_new defaults to 1.0 all over the kernel
    kernel_scale(&tmp, -1.0);
    kernel_set_value(&tmp, tmp.radius, tmp.radius, pow(tmp.edgeLength, 2.0) - 1);

    return tmp;
}



//
// CONVOLUTION FILTER application method
//
void apply_convolution_filter_to_pixelbuffer(FilterType type, void *params, PixelBuffer *buffer) {
    Kernel kernel;

    if (type == GAUSSIANBLUR) {
        kernel = create_gaussian_blur_kernel((GaussianBlurParams *)params);
    }
    else if (type == MOTIONBLUR) {
        kernel = create_motion_blur_kernel((MotionBlurParams *)params);
    }
    else if (type == SHARPEN) {
        kernel = create_sharpen_kernel((SharpenParams *)params);
    }
    else if (type == EDGEDETECT) {
        kernel = create_edge_detect_kernel();
    }

    // convolution filter requires a copy of the pixelbuffer
    PixelBuffer copy = pixelbuffer_copy(buffer);

    for (int y = 0; y < buffer->height; y++) {
        for (int x = 0; x < buffer->width; x++) {
            // accumulator
            GdkRGBA accum = {0.0, 0.0, 0.0, 1.0};

            // convolve the kernel over the current pixel
            for (int v = 0; v < kernel.edgeLength; v++) {
                for (int u = 0; u < kernel.edgeLength; u++) {
                    // calculate the position of the current kernel value on the buffer
                    int u_onBuffer = x + (u - kernel.radius);
                    int v_onBuffer = y + (v - kernel.radius);

                    // and clamp it to be within the buffer bounds
                    u_onBuffer = int_clamp(u_onBuffer, 0, buffer->width-1);
                    v_onBuffer = int_clamp(v_onBuffer, 0, buffer->height-1);

                    // calculate the value of the current pixel convolved
                    GdkRGBA currentValue = GdkRGBA_scale(pixelbuffer_get_pixel(&copy,
                        u_onBuffer, v_onBuffer), kernel_get_value(&kernel, u, v));

                    accum = GdkRGBA_add(accum, currentValue);
                }
            }

            // and set the updated pixel
            pixelbuffer_set_pixel(buffer, x, y, GdkRGBA_clamp(accum, 0.0, 1.0));
        }
    }

    pixelbuffer_destroy(&copy);
    kernel_destroy(&kernel);
}
