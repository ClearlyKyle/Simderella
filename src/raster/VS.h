#ifndef __VS_H__
#define __VS_H__

#include "cglm/cglm.h"

typedef struct
{
    vec3 position;
    vec3 normal;
    vec3 colour;
    vec2 tex;
} VertexShaderAttributes_t;

typedef struct
{
    mat4 MVP;
} UniformData_t;

static inline void VERTEX_SHADER(VertexShaderAttributes_t *attributes,
                                 void                     *uniforms,
                                 void                     *output_to_fragment_shader,
                                 vec4                      out_vertex)
{
    output_to_fragment_shader = NULL;

    UniformData_t *uni = (UniformData_t *)uniforms;

    vec4 pos = {attributes->position[0], attributes->position[1], attributes->position[2], 1.0f};
    glm_mat4_mulv(uni->MVP, pos, out_vertex);
}

#endif // __VS_H__