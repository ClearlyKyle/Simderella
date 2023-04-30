#ifndef __TEX_H__
#define __TEX_H__

#include <stdbool.h>
#include <stdio.h>

#include "stb_image.h"

typedef struct
{
    int            w, h, bpp;
    unsigned char *data;
} texture_t;

texture_t Texture_Load(const char *file_path, int bbp);

static inline unsigned char *Texture_Get_Pixel(const texture_t t, const int x, const int y)
{
    return t.data + ((x + t.w * y) * t.bpp);
}

static inline void Texture_Print_Info(const texture_t t)
{
    fprintf(stderr, "Texture width  : %d\n", t.w);
    fprintf(stderr, "Texture height : %d\n", t.h);
    fprintf(stderr, "Texture bbp    : %d\n", t.bpp);
}

static inline void Texture_Destroy(texture_t *t)
{
    if (t->data)
    {
        free(t->data);
        t->data = NULL;
    }
    *t = (texture_t){0};
}

#endif // __TEX_H__