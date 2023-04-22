#ifndef __TEX_H__
#define __TEX_H__

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdbool.h>

typedef struct
{
    int            w, h, bpp;
    unsigned char *data;
} texture_t;

texture_t Texture_Load(const char *file_path, bool alpha)
{
    texture_t t = {0};

    // textures oriented tha same as you view them in paint
    stbi_set_flip_vertically_on_load(1);

    int format = alpha ? STBI_rgb_alpha : STBI_rgb;

    unsigned char *data = stbi_load(file_path, &t.w, &t.h, &t.bpp, format);
    if (!data)
    {
        fprintf(stderr, "Loading image : %s\n", stbi_failure_reason());
        assert(data);
        return t;
    }

    t.data = data;

    return t;
}

static inline void Texture_Print_Info(const texture_t t)
{
    fprintf(stderr, "Texture width  : %d\n", t.w);
    fprintf(stderr, "Texture height : %d\n", t.h);
    fprintf(stderr, "Texture bbp    : %d\n", t.bpp);
}

#endif // __TEX_H__