#ifndef __MAT4X4_H__
#define __MAT4X4_H__

#include <immintrin.h>
#include "cglm/cglm.h"

// DASH Maths

#if defined(_MSC_VER)
#ifdef DASH_STATIC
#define DASH_EXPORT
#elif defined(DASH_EXPORTS)
#define DASH_EXPORT __declspec(dllexport)
#else
#define DASH_EXPORT __declspec(dllimport)
#endif
#define DASH_INLINE __forceinline
#else
#define DASH_EXPORT __attribute__((visibility("default")))
#define DASH_INLINE static inline __attribute((always_inline))
#endif

typedef __m128 mat4x4[4];

// TODO: Remove this and dirrectly construct to mat4x4
DASH_INLINE
void mat4_to_mat4x4(const mat4 m, mat4x4 res)
{
    res[0] = _mm_load_ps(m[0]);
    res[1] = _mm_load_ps(m[1]);
    res[2] = _mm_load_ps(m[2]);
    res[3] = _mm_load_ps(m[3]);
}

DASH_INLINE
void dash_mat_copy(const mat4x4 src, mat4x4 dest)
{
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
}

DASH_INLINE __m128 mat4x4_mul_m128(const mat4x4 mat, const __m128 vec)
{
#if 1 /* Use alternative method */
    const __m128 v0 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0, 0, 0, 0));
    const __m128 v1 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1));
    const __m128 v2 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 2, 2, 2));
    const __m128 v3 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3, 3, 3, 3));

    return _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(v0, mat[0]),
            _mm_mul_ps(v1, mat[1])),
        _mm_add_ps(
            _mm_mul_ps(v2, mat[2]),
            _mm_mul_ps(v3, mat[3])));
#else /* Use DASH method */
    __m128 v0 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 v1 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 v2 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 v3 = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3, 3, 3, 3));

    __m128 x1;
    x1 = _mm_mul_ps(mat[3], v3);
    x1 = glmm_fmadd(mat[2], v2, x1);
    x1 = glmm_fmadd(mat[1], v1, x1);
    x1 = glmm_fmadd(mat[0], v0, x1);
    return x1;
#endif
}

DASH_INLINE
void dash_make_identity(mat4x4 m)
{
    m[0] = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
    m[1] = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
    m[2] = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
    m[3] = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
}

DASH_INLINE
void dash_translate_make(mat4x4 m, float x, float y, float z)
{
    dash_make_identity(m);
    m[3] = _mm_setr_ps(x, y, z, 1.0f);
}

CGLM_INLINE
void dash_rotate_make(mat4x4 mat, const float angle, vec3 axis)
{
    CGLM_ALIGN(8)
    vec3        axisn, v, vs;
    mat4        m = GLM_MAT4_IDENTITY_INIT;
    const float c = cosf(angle);

    glm_vec3_normalize_to(axis, axisn);
    glm_vec3_scale(axisn, 1.0f - c, v);
    glm_vec3_scale(axisn, sinf(angle), vs);

    glm_vec3_scale(axisn, v[0], m[0]);
    glm_vec3_scale(axisn, v[1], m[1]);
    glm_vec3_scale(axisn, v[2], m[2]);

    m[0][0] += c;
    m[1][0] -= vs[2];
    m[2][0] += vs[1];
    m[0][1] += vs[2];
    m[1][1] += c;
    m[2][1] -= vs[0];
    m[0][2] -= vs[1];
    m[1][2] += vs[0];
    m[2][2] += c;

    m[0][3] = m[1][3] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.0f;
    m[3][3]                                                   = 1.0f;

    mat4_to_mat4x4(m, mat);
}

DASH_INLINE
void dash_mul_rot(const mat4x4 m1, const mat4x4 m2, mat4x4 dest)
{
    __m128 v0 = _mm_mul_ps(glmm_splat_x(m2[0]), m1[0]);
    __m128 v1 = _mm_mul_ps(glmm_splat_x(m2[1]), m1[0]);
    __m128 v2 = _mm_mul_ps(glmm_splat_x(m2[2]), m1[0]);

    v0 = glmm_fmadd(glmm_splat_y(m2[0]), m1[1], v0);
    v1 = glmm_fmadd(glmm_splat_y(m2[1]), m1[1], v1);
    v2 = glmm_fmadd(glmm_splat_y(m2[2]), m1[1], v2);

    v0 = glmm_fmadd(glmm_splat_z(m2[0]), m1[2], v0);
    v1 = glmm_fmadd(glmm_splat_z(m2[1]), m1[2], v1);
    v2 = glmm_fmadd(glmm_splat_z(m2[2]), m1[2], v2);

    dest[0] = v0;
    dest[1] = v1;
    dest[2] = v2;
    dest[3] = m1[3];
}

DASH_INLINE
void dash_rotate(mat4x4 m, float angle, vec3 axis)
{
    mat4x4 rot;
    dash_rotate_make(rot, angle, axis);
    dash_mul_rot(m, rot, m);
}

DASH_INLINE
void dash_mat_mul_mat(const mat4x4 mlef, const mat4x4 mrig, mat4x4 dest)
{
#if 0
        __m128 row1 = _mm_load_ps(&B[0]);
    __m128     row2 = _mm_load_ps(&B[4]);
    __m128     row3 = _mm_load_ps(&B[8]);
    __m128     row4 = _mm_load_ps(&B[12]);

    for (int i = 0; i < 4; i++)
    {
        __m128 brod1 = _mm_set1_ps(A[4 * i + 0]);
        __m128 brod2 = _mm_set1_ps(A[4 * i + 1]);
        __m128 brod3 = _mm_set1_ps(A[4 * i + 2]);
        __m128 brod4 = _mm_set1_ps(A[4 * i + 3]);
        __m128 row   = _mm_add_ps(
              _mm_add_ps(
                  _mm_mul_ps(brod1, row1),
                  _mm_mul_ps(brod2, row2)),
              _mm_add_ps(
                  _mm_mul_ps(brod3, row3),
                  _mm_mul_ps(brod4, row4)));
        _mm_store_ps(&Output_Matrix[4 * i], row);
    }
#elif 0
    mat4x4 res;
    mat4x4 tmp = *A; // Is this needed?

    _MM_TRANSPOSE4_PS(tmp.rows[0], tmp.rows[1], tmp.rows[2], tmp.rows[3]);

    for (int i = 0; i < 4; i++)
    {
        res.rows[i] = _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(tmp.rows[i], tmp.rows[i], _MM_SHUFFLE(0, 0, 0, 0)), B->rows[0]),
                _mm_mul_ps(_mm_shuffle_ps(tmp.rows[i], tmp.rows[i], _MM_SHUFFLE(1, 1, 1, 1)), B->rows[1])),
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(tmp.rows[i], tmp.rows[i], _MM_SHUFFLE(2, 2, 2, 2)), B->rows[2]),
                _mm_mul_ps(_mm_shuffle_ps(tmp.rows[i], tmp.rows[i], _MM_SHUFFLE(3, 3, 3, 3)), B->rows[3])));
    }
    return res;
#else
    __m128 v0 = _mm_mul_ps(glmm_splat_x(mrig[0]), mlef[0]);
    __m128 v1 = _mm_mul_ps(glmm_splat_x(mrig[1]), mlef[0]);
    __m128 v2 = _mm_mul_ps(glmm_splat_x(mrig[2]), mlef[0]);
    __m128 v3 = _mm_mul_ps(glmm_splat_x(mrig[3]), mlef[0]);

    v0 = glmm_fmadd(glmm_splat_y(mrig[0]), mlef[1], v0);
    v1 = glmm_fmadd(glmm_splat_y(mrig[1]), mlef[1], v1);
    v2 = glmm_fmadd(glmm_splat_y(mrig[2]), mlef[1], v2);
    v3 = glmm_fmadd(glmm_splat_y(mrig[3]), mlef[1], v3);

    v0 = glmm_fmadd(glmm_splat_z(mrig[0]), mlef[2], v0);
    v1 = glmm_fmadd(glmm_splat_z(mrig[1]), mlef[2], v1);
    v2 = glmm_fmadd(glmm_splat_z(mrig[2]), mlef[2], v2);
    v3 = glmm_fmadd(glmm_splat_z(mrig[3]), mlef[2], v3);

    v0 = glmm_fmadd(glmm_splat_w(mrig[0]), mlef[3], v0);
    v1 = glmm_fmadd(glmm_splat_w(mrig[1]), mlef[3], v1);
    v2 = glmm_fmadd(glmm_splat_w(mrig[2]), mlef[3], v2);
    v3 = glmm_fmadd(glmm_splat_w(mrig[3]), mlef[3], v3);

    dest[0] = v0;
    dest[1] = v1;
    dest[2] = v2;
    dest[3] = v3;
#endif
}

DASH_INLINE
void Raster_View_Matrix(mat4x4 view, vec3 eye)
{
    vec3 lookat = {0.0f, 0.0f, 0.0f};
    vec3 up     = {0.0f, 1.0f, 0.0f};

    mat4 tmp_view;
    glm_lookat(eye, lookat, up, tmp_view);

    mat4_to_mat4x4(tmp_view, view);
}

DASH_INLINE
void Raster_Projection_Matrix(mat4x4 proj, const int width, const int height)
{
    float nearPlane = 0.1f;
    float farPlane  = 100.f;
    float aspect    = (float)width / (float)height;
    float FOVh      = 60.0f; // Horizontal FOV

    mat4 tmp_proj;
    glm_perspective(glm_rad(FOVh), aspect, nearPlane, farPlane, tmp_proj);

    mat4_to_mat4x4(tmp_proj, proj);
}

DASH_INLINE
void Raster_View_Port_Matrix(mat4x4 view_port, const float width, const float height)
{
    mat4 vp;
    glm_mat4_identity(vp);

    const float half_width = 0.5f * width;
    const float half_heigh = 0.5f * height;

    vp[0][0] = half_width;
    vp[1][1] = -half_heigh;

    vp[3][0] = half_width;
    vp[3][1] = half_heigh;

    mat4_to_mat4x4(vp, view_port);

    // 0.5f * (float)SCREENW,  0.0f,                    0.0f,   0.0f,
    // 0.0f,                  -0.5f * (float)SCREENH,   0.0f,   0.0f,
    // 0.0f,                   0.0f,                    1.0f,   0.0f,
    // 0.5f * (float)SCREENW,  0.5f * (float)SCREENH,   0.0f,   1.0f);
}
#endif // __MAT4X4_H__