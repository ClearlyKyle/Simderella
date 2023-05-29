#ifndef __VS_H__
#define __VS_H__

#include "tex.h"
#include "cglm/cglm.h"

typedef struct
{
    vec3 position;
    vec2 tex;
    // vec3   normal;
    // vec3   colour;
} VertexShaderAttributes_t;

typedef struct
{
    mat4x4     MVP;
    texture_t *diffuse;
} UniformData_t;

static inline void VERTEX_SHADER(void   *attributes,

                                 void   *uniforms,
                                 void   *output_to_fragment_shader,
                                 __m128 *out_vertex)
{
    VertexShaderAttributes_t *att_pos = (VertexShaderAttributes_t *)attributes;
    UniformData_t            *uni     = (UniformData_t *)uniforms;

    output_to_fragment_shader = (void *)uni->diffuse;

    __m128 position = _mm_setr_ps(att_pos->position[0], att_pos->position[1], att_pos->position[2], 1.0f);
    *out_vertex     = mat4x4_mul_m128(uni->MVP, position);
}

#endif // __VS_H__