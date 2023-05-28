#ifndef __VS_H__
#define __VS_H__

#include "cglm/cglm.h"

typedef struct
{
    __m128 position;
    vec3   normal;
    vec3   colour;
    vec2   tex;
} VertexShaderAttributes_t;

typedef struct
{
    mat4x4 MVP;
} UniformData_t;

static inline void VERTEX_SHADER(VertexShaderAttributes_t *attributes,
                                 void                     *uniforms,
                                 void                     *output_to_fragment_shader,
                                 __m128                   *out_vertex)
{
    output_to_fragment_shader = NULL;

    UniformData_t *uni = (UniformData_t *)uniforms;

    *out_vertex = mat4x4_mul_m128(uni->MVP, attributes->position);
}

#endif // __VS_H__