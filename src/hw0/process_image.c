#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "image.h"

// clamps val between min and max (max is not inclusive)
int clamp(int val, int min, int max) 
{
    int lowclamp = val < min ? min : val;
    return lowclamp >= max ? max - 1 : lowclamp;
}

// clamps val between min and max inclusive
float float_clamp(float val, float min, float max) 
{
    float lowclamp = val < min ? min : val;
    return lowclamp > max ? max : lowclamp;
}

// get position in im.data of a pixel
int image_index(int x, int y, int c, int w, int h) 
{
    return clamp(x, 0, w) + clamp(y, 0, h) * w + w * h * c;
}

// return float at x, y, and channel
float get_pixel(image im, int x, int y, int c) 
{
    return im.data[image_index(x, y, c, im.w, im.h)];
}

// set float at x, y, and channel
void set_pixel(image im, int x, int y, int c, float v) 
{
    if (x < 0 || x >= im.w || y < 0 || y >= im.h) return;
    im.data[image_index(x, y, c, im.w, im.h)] = v;
}

// return a copy of the provided image with the exact same data field.
image copy_image(image im) 
{
    image copy = make_image(im.w, im.h, im.c);
    memcpy(copy.data, im.data, im.w * im.h * im.c * sizeof(float));
    return copy;
}

typedef void (*PixelTransformation)(float[],float[],void*[]);
void apply_pixel_by_pixel_transformation(image in, image out, PixelTransformation transform, void* params[]) 
{
    assert(in.w == out.w);
    assert(in.h == out.h);

    for (int y = 0; y < in.h; y++) {
        for (int x = 0; x < in.w; x++) {
            float input[in.c];
            float output[out.c];
            for (int c = 0; c < in.c; c++) {
                input[c] = get_pixel(in, x, y, c);
            }
            transform(input, output, params);
            for (int c = 0; c < out.c; c++) {
                set_pixel(out, x, y, c, output[c]);
            }
        }
    }
}

void grayscaler(float input[], float output[], void** params) 
{
    output[0] = input[0] * 0.299 + input[1] * 0.587 + input[2] * 0.114;
}

image rgb_to_grayscale(image im)
{
    assert(im.c == 3);
    image gray = make_image(im.w, im.h, 1);
    apply_pixel_by_pixel_transformation(im, gray, grayscaler, NULL);
    return gray;
}

void channel_shifter(float input[], float output[], void* params[]) {
    for (int i = 0; i < 3; i++) {
        output[i] = input[i];
    }
    output[*((int*)params[0])] += *((float*)params[1]);
}

void shift_image(image im, int c, float v)
{
    assert(im.c == 3);
    void* params[] = {&c, &v};
    apply_pixel_by_pixel_transformation(im, im, channel_shifter, params);
}

void channel_scaler(float input[], float output[], void* params[]) {
    for (int i = 0; i < 3; i++) {
        output[i] = input[i];
    }
    output[*((int*)params[0])] *= *((float*)params[1]);
}

void scale_image(image im, int c, float v) 
{
    assert(im.c == 3);
    void* params[] = {&c, &v};
    apply_pixel_by_pixel_transformation(im, im, channel_scaler, params);
}

void clamper(float input[], float output[], void**params) {
    for (int i = 0; i < 3; i++) {
        output[i] = float_clamp(input[i], *((float*)params[0]), *((float*)params[1]));
    }
}

void clamp_image(image im)
{
    assert(im.c == 3);
    float low = 0.0, high = 1.0;
    void* params[] = {&low, &high};
    apply_pixel_by_pixel_transformation(im, im, clamper, params);
}

// These might be handy
float three_way_max(float a, float b, float c)
{
    return (a > b) ? ( (a > c) ? a : c) : ( (b > c) ? b : c) ;
}

float three_way_min(float a, float b, float c)
{
    return (a < b) ? ( (a < c) ? a : c) : ( (b < c) ? b : c) ;
}

void rgb_to_hsv_converter(float input[], float output[], void* params[]) 
{
    float V = three_way_max(input[0], input[1], input[2]);
    float m = three_way_min(input[0], input[1], input[2]);
    float C = V - m;
    float S = V == 0.0 ? 0.0 : (C / V);
    float _H;
    if (C == 0.0) {
        _H = 0.0;
    } else if (V == input[0]) {
        _H = (input[1] - input[2]) / C;
    } else if (V == input[1]) {
        _H = (input[2] - input[0]) / C + 2.0;
    } else {
        _H = (input[0] - input[1]) / C + 4.0;
    }
    float H = _H < 0.0 ? ((_H / 6.0) + 1.0) : (_H / 6.0);
    output[0] = H;
    output[1] = S;
    output[2] = V;
}

void rgb_to_hsv(image im)
{
    assert(im.c == 3);
    apply_pixel_by_pixel_transformation(im, im, rgb_to_hsv_converter, NULL);
}

void hsv_to_rgb_converter(float input[], float output[], void* params[]) 
{
    float H = input[0];
    float S = input[1];
    float V = input[2];
    if (S == 0.0) {
        output[0] = V;
        output[1] = V;
        output[2] = V;
        return;
    }
    float C = V * S;
    float max_rgb = V;
    float min_rgb = V - C;
    float _H = H * 6.0;
    if (_H >= 0.0 && _H < 1.0) {
        output[0] = max_rgb;
        output[1] = _H * C + min_rgb;
        output[2] = min_rgb;
    } else if (_H >= 1.0 && _H < 2.0) {
        output[0] = -(_H - 2.0) * C + min_rgb;
        output[1] = max_rgb;
        output[2] = min_rgb;
    } else if (_H >= 2.0 && _H < 3.0) {
        output[0] = min_rgb;
        output[1] = max_rgb;
        output[2] = (_H - 2.0) * C + min_rgb;
    } else if (_H >= 3.0 && _H < 4.0) {
        output[0] = min_rgb;
        output[1] = -(_H - 4.0) * C + min_rgb;
        output[2] = max_rgb;
    } else if (_H >= 4.0 && _H < 5.0) {
        output[0] = (_H - 4.0) * C + min_rgb;
        output[1] = min_rgb;
        output[2] = max_rgb;
    } else {
        output[0] = max_rgb;
        output[1] = min_rgb;
        output[2] = -(_H - 6.0) * C + min_rgb;
    }
}

void hsv_to_rgb(image im)
{   
    assert(im.c == 3);
    apply_pixel_by_pixel_transformation(im, im, hsv_to_rgb_converter, NULL);
}