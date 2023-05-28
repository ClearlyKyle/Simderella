#include "renderer.h"
#include "utils/utils.h"

static inline void Compute_Bounding_Box_Screen_Space(vec4 ss_v0, vec4 ss_v1, vec4 ss_v2, ivec4 AABB)
{
    const int maxX = (const int)ceilf(fmaxf(ss_v0[0], fmaxf(ss_v1[0], ss_v2[0])));
    const int minX = (const int)floorf(fminf(ss_v0[0], fminf(ss_v1[0], ss_v2[0])));
    const int maxY = (const int)ceilf(fmaxf(ss_v0[1], fmaxf(ss_v1[1], ss_v2[1])));
    const int minY = (const int)floorf(fminf(ss_v0[1], fminf(ss_v1[1], ss_v2[1])));

    AABB[0] = minX;
    AABB[1] = minY;
    AABB[2] = maxX;
    AABB[3] = maxY;
}

// NOTE: Temporary until going full simd
static inline void Perspective_Divide_Vertex(vec4 vert)
{
    vert[3] = 1.0f / vert[3];
    vert[0] *= vert[3];
    vert[1] *= vert[3];
    vert[2] *= vert[3];
}

static inline void Framebuffer_Clear_Depth(const float depth_value)
{
    // Clear the Depth Buffer
    float *end = &RenderState.depth_buffer[IMAGE_W * IMAGE_H];
    for (float *p = RenderState.depth_buffer; p != end; p++)
        *p = depth_value;
}

#include <string.h>

static inline void Framebuffer_Clear_Both(const float depth_value)
{
    // Clear the Colour buffer
    memset((void *)RenderState.colour_buffer, 0, sizeof(uint8_t) * IMAGE_W * IMAGE_H * IMAGE_BPP);

    Framebuffer_Clear_Depth(depth_value);
}

void Setup_Triangles(void)
{
    Framebuffer_Clear_Both(FLT_MAX);

    VertexShaderAttributes_t attributes[3];

    Trianges_To_Be_Rastered_Counter = 0;

    //__m128                 gathered_vertices[4][3] = {0};
    // tinyobj_vertex_index_t gatehred_indices[4][3];

    ASSERT(RenderState.number_of_indices != 0);
    ASSERT(RenderState.vertex_stride != 0);

    void *output_to_fragment_shader = NULL; // NOTE: Does this only need to be one value?

    for (uint32_t vert_idx = 0; vert_idx < RenderState.number_of_indices; vert_idx += RenderState.vertex_stride)
    {
        float *v0 = &RenderState.vertices[3 * RenderState.indices[vert_idx + 0]];
        float *v1 = &RenderState.vertices[3 * RenderState.indices[vert_idx + 1]];
        float *v2 = &RenderState.vertices[3 * RenderState.indices[vert_idx + 2]];

        attributes[0].position[0] = v0[0];
        attributes[0].position[1] = v0[1];
        attributes[0].position[2] = v0[2];

        attributes[1].position[0] = v1[0];
        attributes[1].position[1] = v1[1];
        attributes[1].position[2] = v1[2];

        attributes[2].position[0] = v2[0];
        attributes[2].position[1] = v2[1];
        attributes[2].position[2] = v2[2];

        vec4 out_vertex0, out_vertex1, out_vertex2;
        VERTEX_SHADER(&attributes[0], RenderState.vertex_shader_uniforms, output_to_fragment_shader, out_vertex0);
        VERTEX_SHADER(&attributes[1], RenderState.vertex_shader_uniforms, output_to_fragment_shader, out_vertex1);
        VERTEX_SHADER(&attributes[2], RenderState.vertex_shader_uniforms, output_to_fragment_shader, out_vertex2);

        vec4 ss_v0, ss_v1, ss_v2;
        glm_mat4_mulv(RenderState.view_port_matrix, out_vertex0, ss_v0);
        glm_mat4_mulv(RenderState.view_port_matrix, out_vertex1, ss_v1);
        glm_mat4_mulv(RenderState.view_port_matrix, out_vertex2, ss_v2);

        /* Perspective Divide */
        Perspective_Divide_Vertex(ss_v0);
        Perspective_Divide_Vertex(ss_v1);
        Perspective_Divide_Vertex(ss_v2);

        // ivec4 AABB; /* minX, minY, maxX, maxY*/
        // Compute_Bounding_Box_Screen_Space(ss_v0, ss_v1, ss_v2, AABB);

        /* Area */
        float area = (ss_v2[0] - ss_v0[0]) * (ss_v1[1] - ss_v0[1]);
        area       = area - ((ss_v0[0] - ss_v1[0] * ss_v0[0] - ss_v2[0]));
        area       = 1.0f / area;

        if (area < 0.0f)
            continue;

        RasterData_t *rtri = &Trianges_To_Be_Rastered[Trianges_To_Be_Rastered_Counter++];

        rtri->ss_v0 = _mm_load_ps(ss_v0);
        rtri->ss_v1 = _mm_load_ps(ss_v1);
        rtri->ss_v2 = _mm_load_ps(ss_v2);
    }

    // printf("Number of triangles to be rastered : %zu\n", Trianges_To_Be_Rastered_Counter);
}