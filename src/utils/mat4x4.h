#ifndef __MAT4X4_H__
#define __MAT4X4_H__

#include <immintrin.h>
#include "cglm/cglm.h"

typedef __m128 mat4x4[4];

inline __m128 mat4x4_mul_m128(const mat4x4 mat, const __m128 vec)
{
#if 1 /* Use alternative method */
    __m128 x1 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0, 0, 0, 0));
    x1        = _mm_mul_ps(x1, mat[0]);

    __m128 tmp = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1));
    tmp        = _mm_mul_ps(tmp, mat[1]);

    x1  = _mm_add_ps(x1, tmp);
    tmp = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 2, 2, 2));

    tmp = _mm_mul_ps(tmp, mat[2]);
    x1  = _mm_add_ps(x1, tmp);

    x1 = _mm_add_ps(x1, mat[3]);
#else /* Use CGLM method */
    __m128 v0 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 v1 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 v2 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 v3 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3, 3, 3, 3));

    __m128 x1;
    x1 = _mm_mul_ps(mat[3], v3);
    x1 = glmm_fmadd(mat[2], v2, x1);
    x1 = glmm_fmadd(mat[1], v1, x1);
    x1 = glmm_fmadd(mat[0], v0, x1);
#endif
    return x1;
}

// TODO: Remove this and dirrectly construct to mat4x4
static inline void mat4_to_m128(const mat4 m, mat4x4 res)
{
    res[0] = _mm_load_ps(m[0]);
    res[1] = _mm_load_ps(m[1]);
    res[2] = _mm_load_ps(m[2]);
    res[3] = _mm_load_ps(m[3]);
}

static inline void Raster_View_Matrix(mat4 view, vec3 eye)
{
    vec3 lookat = {0.0f, 0.0f, 0.0f};
    vec3 up     = {0.0f, 1.0f, 0.0f};

    glm_lookat(eye, lookat, up, view);
}

static inline void Raster_Projection_Matrix(mat4 proj, const int width, const int height)
{
    float nearPlane = 0.1f;
    float farPlane  = 100.f;
    float aspect    = (float)width / (float)height;
    float FOVh      = 60.0f; // Horizontal FOV

    glm_perspective(glm_rad(FOVh), aspect, nearPlane, farPlane, proj);
}

static inline void Raster_View_Port_Matrix(mat4 vp, const float width, const float height)
{
    glm_mat4_identity(vp);

    const float half_width = 0.5f * width;
    const float half_heigh = 0.5f * height;

    vp[0][0] = half_width;
    vp[1][1] = -half_heigh;

    vp[3][0] = half_width;
    vp[3][1] = half_heigh;

    // 0.5f * (float)SCREENW,  0.0f,                    0.0f,   0.0f,
    // 0.0f,                  -0.5f * (float)SCREENH,   0.0f,   0.0f,
    // 0.0f,                   0.0f,                    1.0f,   0.0f,
    // 0.5f * (float)SCREENW,  0.5f * (float)SCREENH,   0.0f,   1.0f);
}
#endif // __MAT4X4_H__