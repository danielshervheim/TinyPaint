//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef FILTER_H_
#define FILTER_H_

#include <gdk/gdk.h>  // GdkRGBA
#include "pixel_buffer.h"  // PixelBuffer

typedef enum filtertype {
    SATURATION,
    CHANNELS,
    INVERT,
    BRIGHTNESSCONTRAST,
    GAUSSIANBLUR,
    MOTIONBLUR,
    SHARPEN,
    EDGEDETECT,
    POSTERIZE,
    THRESHOLD
} FilterType;

typedef struct saturation_params {
    double scale;
} SaturationParams;

typedef struct channels_params {
    double r_scale;
    double g_scale;
    double b_scale;
} ChannelsParams;

// invert filter has no params

typedef struct brightness_contrast_params {
    double brightness_scale;
    double contrast_scale;
} BrightnessContrastParams;

typedef struct gaussian_blur_params {
    int radius;
} GaussianBlurParams;

typedef struct motion_blur_params {
    int radius;
    double angle;
} MotionBlurParams;

typedef struct sharpen_params {
    int radius;
} SharpenParams;

// edge detect has no params

typedef struct posterize_params {
    int num_bins;
} PosterizeParams;

typedef struct threshold_params {
    double cutoff;
} ThresholdParams;

/* Applies a basic filter to the input buffer. */
void apply_basic_filter_to_pixelbuffer(FilterType type, void *params, PixelBuffer *buffer);

/* Applies a convolution filter to the input buffer. */
void apply_convolution_filter_to_pixelbuffer(FilterType type, void *params, PixelBuffer *buffer);

#endif  // FILTER_H_
