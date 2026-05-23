#pragma once

#include <cstddef>
#include <cstdint>
extern "C" {
typedef struct Image_t Image;

Image *image_get(const char *filename);
uint8_t *image_get_pixels(Image *);
size_t image_get_pixel_len(Image *);
void image_free(Image *);
};
