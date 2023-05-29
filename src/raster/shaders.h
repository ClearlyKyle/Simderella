#ifndef __SHADERS_H__
#define __SHADERS_H__

#include "tex.h"
#include "cglm/cglm.h"

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
} InterpolatedPixel_t ;

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

static inline void FRAGMENT_SHADER(InterpolatedPixel_t *interpolated_data,
                                   int                  lane, /* pixel lane in simd vector */
                                   VSOutputForFS_t     *input_from_vertex_shader,
                                   ivec4                out_frag_colour)
{
    assert(input_from_vertex_shader);
    texture_t *tex = input_from_vertex_shader->diffuse;

    float U = interpolated_data->vec2_attribute[0].X[lane];
    float V = interpolated_data->vec2_attribute[0].Y[lane];

    glm_clamp(U, 0.0f, 1.0f);
    glm_clamp(V, 0.0f, 1.0f);

    U *= (float)(tex->w - 1);
    V *= (float)(tex->h - 1);

    uint8_t *pixel = Texture_Get_Pixel(*tex, (const int)U, (const int)V);

    out_frag_colour[0] = pixel[2];
    out_frag_colour[1] = pixel[1];
    out_frag_colour[2] = pixel[0];
    out_frag_colour[3] = 255;
}

#endif // __SHADERS_H__d