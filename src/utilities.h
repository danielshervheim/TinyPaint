//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <gdk/gdk.h>  // GdkRGBA

//
// DOUBLE functions
//

/* Returns the absolute value of 'value' */
double double_abs(double value);

/* Returns 'value' clamped between 'min' and 'max' */
double double_clamp(double value, double min, double max);

/* Returns the std. dev. of x */
double double_gaussian(double x, double sigma);

/* Returns the interpolation of x and y by t */
double double_lerp(double x, double y, double t);

/* Returns the larger of x and y */
double double_max(double x, double y);

/* Returns the smaller of x and y */
double double_min(double x, double y);



//
// GdkRGBA functions
//

/* color+color : Returns 'a' added channel-wise to 'b' */
GdkRGBA GdkRGBA_add(GdkRGBA a, GdkRGBA b);

/* Returns 'color' clamped channel-wise between 'min' and 'max' */
GdkRGBA GdkRGBA_clamp(GdkRGBA color, double min, double max);

/* color/color : Returns 'a' divided channel-wise by 'b' */
GdkRGBA GdkRGBA_divide(GdkRGBA a, GdkRGBA b);

/* color==color : Returns true if 'a' equals 'b' (within the threshold) */
int GdkRGBA_equals(GdkRGBA a, GdkRGBA b, double threshold);

/* Returns the interpolation of 'a' and 'b' by 't' */
GdkRGBA GdkRGBA_lerp(GdkRGBA a, GdkRGBA b, double t);

/* Returns the perceived "brightness" of 'color' */
double GdkRGBA_luminance(GdkRGBA color);

/* Returns the channel-wise minimum of 'a' and 'b' */
GdkRGBA GdkRGBA_min(GdkRGBA a, GdkRGBA b);

/* color*color : Returns 'a' multiplied channel-wise by 'b' */
GdkRGBA GdkRGBA_multiply(GdkRGBA a, GdkRGBA b);

/* color*scalar : Returns 'color' multiplied channel-wise by 'scale' */
GdkRGBA GdkRGBA_scale(GdkRGBA color, double scale);

/* color-color : Returns 'b' subtracted channel-wise from 'a' */
GdkRGBA GdkRGBA_subtract(GdkRGBA a, GdkRGBA b);



//
// INT functions
//

/* Returns 'value' clamped between 'min' and 'max' */
int int_clamp(int value, int min, int max);



//
// STRING / CHAR functions
//

/* Returns whether "string" ends with "end" */
int string_ends_with(const char *string, const char *end);



//
// GL functions
//

/* Loads and compiles the shader at "filename" and returns a GL handle to the shader. */
int load_and_compile_shader(char* filename, int shaderType);

/* Loads the file at "filename" and returns a buffer containing the file contents.
Also returns the length of the buffer in the "length" parameter. */
char* load_file(char* filename, int* length);

#endif  // UTILITIES_H_
