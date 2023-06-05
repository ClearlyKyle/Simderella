#ifndef __SHADERS_H__
#define __SHADERS_H__

#include <math.h>

#include "tex.h"
#include "cglm/cglm.h"
#include "utils/utils.h"

typedef struct
{
    vec3 position;
    vec2 tex;
    // vec3   normal;
    // vec3   colour;
} VertexShaderAttributes_t;

#define NUMBER_OF_VARYING_VE4_ATTRIBUTES 0
#define NUMBER_OF_VARYING_VE3_ATTRIBUTES 0
#define NUMBER_OF_VARYING_VE2_ATTRIBUTES 1

typedef struct
{
    union
    {
        vec4   raw;
        __m128 vec;
    } vec4_attribute[NUMBER_OF_VARYING_VE4_ATTRIBUTES + 1];

    union
    {
        vec3   raw;
        float  pad[4];
        __m128 vec;
    } vec3_attribute[NUMBER_OF_VARYING_VE3_ATTRIBUTES + 1];

    union
    {
        vec2   raw;
        float  pad[4];
        __m128 vec;
    } vec2_attribute[NUMBER_OF_VARYING_VE2_ATTRIBUTES + 1];
} VaryingAttributes_t;

typedef struct
{
    mat4x4     MVP;
    texture_t *diffuse;
} UniformData_t;

typedef struct
{
    texture_t *diffuse;
} VSOutputForFS_t;

#pragma warning(push)
#pragma warning(disable : 4201)

// clang-format off
typedef struct InterpolatedPixel {
    struct Vec4Attributes
    {
        union { vec4 X; __m128 mX; }; 
        union { vec4 Y; __m128 mY; };
        union { vec4 Z; __m128 mZ; };
        union { vec4 W; __m128 mW; };
    } vec4_attribute[NUMBER_OF_VARYING_VE4_ATTRIBUTES + 1];

    struct Vec3Attributes
    {
        union { vec4 X; __m128 mX; };
        union { vec4 Y; __m128 mY; };
        union { vec4 Z; __m128 mZ; };
    } vec3_attribute[NUMBER_OF_VARYING_VE3_ATTRIBUTES + 1];

    struct Vec2Attributes
    {
        union { vec4 X; __m128 mX; };
        union { vec4 Y; __m128 mY; };
    } vec2_attribute[NUMBER_OF_VARYING_VE2_ATTRIBUTES + 1];
} InterpolatedPixel_t;

// clang-format on
#pragma warning(pop)

static inline void VERTEX_SHADER(void                *attributes,
                                 VaryingAttributes_t *varying,
                                 void                *uniforms,
                                 VSOutputForFS_t     *output_to_fragment_shader,
                                 __m128              *out_vertex)
{
    VertexShaderAttributes_t *att_pos = (VertexShaderAttributes_t *)attributes;
    UniformData_t            *uni     = (UniformData_t *)uniforms;

    output_to_fragment_shader->diffuse = uni->diffuse;

    __m128 position = _mm_setr_ps(att_pos->position[0], att_pos->position[1], att_pos->position[2], 1.0f);
    *out_vertex     = mat4x4_mul_m128(uni->MVP, position);

    varying->vec2_attribute[0].raw[0] = att_pos->tex[0];
    varying->vec2_attribute[0].raw[1] = att_pos->tex[1];
}

#define ROUND(X) floorf((X) + 0.5f)
static inline float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static inline float fractional_part(float value)
{
    return value - (int)value;
}

static inline int integer_part(float value)
{
    return (int)value;
}

static inline void lerpColor(uint8_t colour_a[4], uint8_t colour_b[4], float t, float dest[4])
{
    dest[0] = lerp((float)colour_a[0], (float)colour_b[0], t);
    dest[1] = lerp((float)colour_a[1], (float)colour_b[1], t);
    dest[2] = lerp((float)colour_a[2], (float)colour_b[2], t);
    dest[3] = 255;
}

static inline void FRAGMENT_SHADER(InterpolatedPixel_t *interpolated_data,
                                   int                  lane, /* pixel lane in simd vector */
                                   VSOutputForFS_t     *input_from_vertex_shader,
                                   uint8_t              out_frag_colour[4])
{
    ASSERT(input_from_vertex_shader);
    texture_t *tex = input_from_vertex_shader->diffuse;

    float U = interpolated_data->vec2_attribute[0].X[lane];
    float V = interpolated_data->vec2_attribute[0].Y[lane];

#if 1
    U *= (float)tex->w;
    V *= (float)tex->h;

    const uint8_t *const final_colour = Texture_Get_Pixel(*tex, (const int)ROUND(U), (const int)ROUND(V));
#else
    const float u_frac = fractional_part(U * (float)tex->w);
    const float v_frac = fractional_part(V * (float)tex->h);

    U *= (float)tex->w;
    V *= (float)tex->h;

    uint8_t *texel00 = &tex->data[tex->bpp * (integer_part(U) + (tex->w * integer_part(V)))];
    uint8_t *texel01 = &tex->data[tex->bpp * (integer_part(U) + (tex->w * (integer_part(V) + 1)))];
    uint8_t *texel10 = &tex->data[tex->bpp * (integer_part(U) + (tex->w * (1 + integer_part(V))))];
    uint8_t *texel11 = &tex->data[tex->bpp * (integer_part(U) + (tex->w * (1 + integer_part(V) + 1)))];

    float intermediate_colour1[4], intermediate_colour2[4];
    lerpColor(texel00, texel01, u_frac, intermediate_colour1);
    lerpColor(texel10, texel11, u_frac, intermediate_colour2);

    uint8_t final_colour[4];
    final_colour[0] = (uint8_t)ROUND(lerp(intermediate_colour1[0], intermediate_colour2[0], v_frac));
    final_colour[1] = (uint8_t)ROUND(lerp(intermediate_colour1[1], intermediate_colour2[1], v_frac));
    final_colour[2] = (uint8_t)ROUND(lerp(intermediate_colour1[2], intermediate_colour2[2], v_frac));
    final_colour[3] = 255;

#endif
    out_frag_colour[0] = final_colour[0];
    out_frag_colour[1] = final_colour[1];
    out_frag_colour[2] = final_colour[2];
    out_frag_colour[3] = 255;
}

#endif // __SHADERS_H__d