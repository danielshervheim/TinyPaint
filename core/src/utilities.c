//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "utilities.h"

#include <math.h>

// for GL stuff.
#include "stdio.h"
#include "stdlib.h"
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif



//
// DOUBLE functions
//

double double_abs(double value) {
    return value < 0 ? -value : value;
}

double double_clamp(double value, double min, double max) {
    return double_min(double_max(value, min), max);
}

double double_gaussian(double x, double sigma) {
    return exp(-(pow(x, 2) / (2 * pow(sigma, 2)) + pow(0, 2) / (2 * pow(sigma, 2))));
}

double double_lerp(double x, double y, double t) {
    return (1.0 - t)*x + t*y;
}

double double_max(double x, double y) {
    return x > y ? x : y;
}

double double_min(double x, double y) {
    return x < y ? x : y;
}



//
// GdkRGBA functions
//

GdkRGBA GdkRGBA_add(GdkRGBA a, GdkRGBA b) {
    a.red += b.red;
    a.green += b.green;
    a.blue += b.blue;
    a.alpha = double_clamp(a.alpha + b.alpha, 0.0, 1.0);
    return a;
}

GdkRGBA GdkRGBA_clamp(GdkRGBA color, double min, double max) {
    color.red = double_clamp(color.red, min, max);
    color.green = double_clamp(color.green, min, max);
    color.blue = double_clamp(color.blue, min, max);
    color.alpha = double_clamp(color.alpha, min, max);
    return color;
}

GdkRGBA GdkRGBA_divide(GdkRGBA a, GdkRGBA b) {
    a.red /= b.red;
    a.green /= b.green;
    a.blue /= b.blue;
    a.alpha = double_clamp(a.alpha / b.alpha, 0.0, 1.0);
    return a;
}

int GdkRGBA_equals(GdkRGBA a, GdkRGBA b, double threshold) {
    if (double_abs(a.red - b.red) > threshold ||
        double_abs(a.green - b.green) > threshold ||
        double_abs(a.blue - b.blue) > threshold ||
        double_abs(a.alpha - b.alpha) > threshold) {
        return 0;
    }
    else {
        return 1;
    }
}

GdkRGBA GdkRGBA_lerp(GdkRGBA a, GdkRGBA b, double t) {
    a.red = double_lerp(a.red, b.red, t);
    a.green = double_lerp(a.green, b.green, t);
    a.blue = double_lerp(a.blue, b.blue, t);
    a.alpha = double_clamp(double_lerp(a.alpha, b.alpha, t), 0.0, 1.0);
    return a;
}

double GdkRGBA_luminance(GdkRGBA color) {
    return color.red*0.2126 + color.green*0.7152 + color.blue*0.0722;
}

GdkRGBA GdkRGBA_min(GdkRGBA a, GdkRGBA b) {
    a.red = double_min(a.red, b.red);
    a.green = double_min(a.green, b.green);
    a.blue = double_min(a.blue, b.blue);
    a.alpha = double_clamp(double_min(a.alpha, b.alpha), 0.0, 1.0);
    return a;
}

GdkRGBA GdkRGBA_multiply(GdkRGBA a, GdkRGBA b) {
    a.red *= b.red;
    a.green *= b.green;
    a.blue *= b.blue;
    a.alpha = double_clamp(a.alpha * b.alpha, 0.0, 1.0);
    return a;
}

GdkRGBA GdkRGBA_scale(GdkRGBA color, double scale) {
    color.red *= scale;
    color.green *= scale;
    color.blue *= scale;
    color.alpha = double_clamp(color.alpha * scale, 0.0, 1.0);
    return color;
}

GdkRGBA GdkRGBA_subtract(GdkRGBA a, GdkRGBA b) {
    a.red -= b.red;
    a.green -= b.green;
    a.blue -= b.blue;
    a.alpha = double_clamp(a.alpha - b.alpha, 0.0, 1.0);
    return a;
}



//
// INT functions
//

int int_clamp(int value, int min, int max) {
    if (value > max) {
        return max;
    }
    else if (value < min) {
        return min;
    }
    else {
        return value;
    }
}



//
// STRING / CHAR functions
//

int string_ends_with(const char *string, const char *end) {
    size_t stringlenth = strlen(string);
    size_t endlength = strlen(end);

    if (endlength > stringlenth) {
        return -1;
    }

    return memcmp(string + (stringlenth - endlength), end, endlength);
}



//
// GL functions.
//

// Source:
// http://schabby.de/shader-loading/
int compile_shader(char* shaderSource, int shaderType) {
	// handle will be non-zero if succefully created.
	int handle = glCreateShader(shaderType);

    // upload code to OpenGL and associate code with shader
    const GLchar* shader_ptr = shaderSource;
    glShaderSource(handle, 1, &shader_ptr, NULL);

	// compile source code into binary
	glCompileShader(handle);

	// acquire compilation status
	int shaderStatus = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &shaderStatus);

	// check whether compilation was successful
	if(shaderStatus == GL_FALSE) {
        char msg[512];
        glGetShaderInfoLog(handle, 512, NULL, &msg);
        printf("%s\n", msg);
        return -1;
	}

    printf("Compiled shader.\n");
	return handle;
}

// Source:
// https://stackoverflow.com/a/2029227
char* load_file(char* filename, int* length) {
    char *source = NULL;
    FILE *fp = fopen(filename, "r");

    if (fp != NULL) {
        // Go to the end of the file.
        if (fseek(fp, 0L, SEEK_END) == 0) {
            // Get the size of the file.
            long bufsize = ftell(fp);
            if (bufsize == -1) {
                // ERROR.
                return -1;
            }

            // Allocate our buffer to that size.
            source = malloc(sizeof(char) * (bufsize + 1));

            // Go back to the start of the file.
            if (fseek(fp, 0L, SEEK_SET) != 0) {
                // ERROR.
                return -1;
            }

            // Read the entire file into memory.
            size_t newLen = fread(source, sizeof(char), bufsize, fp);
            if (ferror(fp) != 0) {
                fputs("Error reading file", stderr);
            }
            else {
                source[newLen++] = '\0';  // Just to be safe.
                *length = newLen;
                return source;
            }
        }
        fclose(fp);
    }
    else {
        printf("ERROR: could not load file %s\n", filename);
    }

    return NULL;
}
