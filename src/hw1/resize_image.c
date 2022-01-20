#include <math.h>
#include "image.h"

// return interpolated channel for provided coordinate
typedef float (*Interpolation)(image, float, float, int);

// resize image using provided interpolation function
image resize(image im, int w, int h, Interpolation interp) {
    image out = make_image(w, h, im.c);
    float a_x = ((float)im.w) / ((float)w);
    float b_x = a_x * 0.5 - 0.5;

    float a_y = ((float)im.h) / ((float)h);
    float b_y = a_y * 0.5 - 0.5;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float x_coord = ((float)x) * a_x + b_x;
            float y_coord = ((float)y) * a_y + b_y;
            for (int c = 0; c < im.c; c++) {
                set_pixel(out, x, y, c, interp(im, x_coord, y_coord, c));
            }
        }
    }
    return out;
}

float nn_interpolate(image im, float x, float y, int c)
{
    return get_pixel(im, round(x), round(y), c);
}

image nn_resize(image im, int w, int h)
{
    return resize(im, w, h, nn_interpolate);
}

float interpolate(float first_value, float second_value, float ratio) {
    return first_value * (1 - ratio) + second_value * ratio;
}

float bilinear_interpolate(image im, float x, float y, int c)
{
    int x_rounded_up = ceil(x);
    int x_rounded_down = floor(x);
    int y_rounded_up = ceil(y);
    int y_rounded_down = floor(y);

    float ratio_x = ((float)x) - x_rounded_down;
    float ratio_y = ((float)y) - y_rounded_down;

    return interpolate(
        interpolate(
            get_pixel(im, x_rounded_down, y_rounded_down, c),
            get_pixel(im, x_rounded_up, y_rounded_down, c),
            ratio_x
        ),
        interpolate(
            get_pixel(im, x_rounded_down, y_rounded_up, c),
            get_pixel(im, x_rounded_up, y_rounded_up, c),
            ratio_x
        ),
        ratio_y
    );
}

image bilinear_resize(image im, int w, int h)
{
    return resize(im, w, h, bilinear_interpolate);
}
