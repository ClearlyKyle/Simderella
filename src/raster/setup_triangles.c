#include "renderer.h"
#include "utils/utils.h"

//#define COMPUTE_AREA_IN_RASTER

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

    __m128 collected_vertices[4][3] = {0};
    // tinyobj_vertex_index_t collected_indices[4][3];

    const size_t starting_index = 0;
    const size_t ending_index   = RenderState.number_of_indices;

    assert(RenderState.number_of_indices != 0);
    assert(RenderState.vertex_stride != 0);

    void *output_to_fragment_shader = NULL; // NOTE: Does this only need to be one value?

    for (size_t vert_idx = starting_index; vert_idx < ending_index; /* blank */)
    {
        size_t number_of_collected_triangles = 0;
        for (size_t i = 0; i < 4; i++)
        {
            if (vert_idx == ending_index)
                break;

            CHECK_ARRAY_BOUNDS(vert_idx, RenderState.number_of_indices);

            float *v0 = &RenderState.vertices[3 * RenderState.indices[vert_idx + 0]];
            float *v1 = &RenderState.vertices[3 * RenderState.indices[vert_idx + 1]];
            float *v2 = &RenderState.vertices[3 * RenderState.indices[vert_idx + 2]];

            attributes[0].position = _mm_setr_ps(v0[0], v0[1], v0[2], 1.0f);
            attributes[1].position = _mm_setr_ps(v1[0], v1[1], v1[2], 1.0f);
            attributes[2].position = _mm_setr_ps(v2[0], v2[1], v2[2], 1.0f);

            __m128 out_vertex0 = {0};
            __m128 out_vertex1 = {0};
            __m128 out_vertex2 = {0};
            VERTEX_SHADER(&attributes[0], RenderState.vertex_shader_uniforms, output_to_fragment_shader, &out_vertex0);
            VERTEX_SHADER(&attributes[1], RenderState.vertex_shader_uniforms, output_to_fragment_shader, &out_vertex1);
            VERTEX_SHADER(&attributes[2], RenderState.vertex_shader_uniforms, output_to_fragment_shader, &out_vertex2);

            collected_vertices[i][0] = mat4x4_mul_m128(RenderState.view_port_matrix, out_vertex0);
            collected_vertices[i][1] = mat4x4_mul_m128(RenderState.view_port_matrix, out_vertex1);
            collected_vertices[i][2] = mat4x4_mul_m128(RenderState.view_port_matrix, out_vertex2);

            ++number_of_collected_triangles;
            vert_idx += RenderState.vertex_stride;
        }

        /* Extract the X, Y and W values from 4 traingles  */
        __m128 X[3], Y[3], Z[3], W[3];
        for (int i = 0; i < 3; ++i)
        {
            /* Get 4 verticies at once */
            __m128 tri0_vert_i = collected_vertices[0][i]; // Get vertex i from triangle 0
            __m128 tri1_vert_i = collected_vertices[1][i]; // Get vertex i from triangle 1
            __m128 tri2_vert_i = collected_vertices[2][i]; // Get vertex i from triangle 2
            __m128 tri3_vert_i = collected_vertices[3][i]; // Get vertex i from triangle 3

            _MM_TRANSPOSE4_PS(tri0_vert_i, tri1_vert_i, tri2_vert_i, tri3_vert_i);
            X[i] = tri0_vert_i; // X, X, X, X
            Y[i] = tri1_vert_i; // Y, Y, Y, Y
            Z[i] = tri2_vert_i; // Z, Z, Z, Z
            W[i] = tri3_vert_i; // W, W, W, W
        }

        /* Perspective divison */
        W[0] = _mm_rcp_ps(W[0]);
        W[1] = _mm_rcp_ps(W[1]);
        W[2] = _mm_rcp_ps(W[2]);

        X[0] = _mm_mul_ps(X[0], W[0]);
        X[1] = _mm_mul_ps(X[1], W[1]);
        X[2] = _mm_mul_ps(X[2], W[2]);

        Y[0] = _mm_mul_ps(Y[0], W[0]);
        Y[1] = _mm_mul_ps(Y[1], W[1]);
        Y[2] = _mm_mul_ps(Y[2], W[2]);

        Z[0] = _mm_mul_ps(Z[0], W[0]);
        Z[1] = _mm_mul_ps(Z[1], W[1]);
        Z[2] = _mm_mul_ps(Z[2], W[2]);

#ifndef COMPUTE_AREA_IN_RASTER
        // Why the fuck does this not work?
        const __m128 triArea = _mm_sub_ps(
            _mm_mul_ps(_mm_sub_ps(X[2], X[0]), _mm_sub_ps(Y[1], Y[0])),
            _mm_mul_ps(_mm_sub_ps(X[0], X[1]), _mm_sub_ps(Y[0], Y[2])));
        const __m128 invTriArea = _mm_rcp_ps(triArea);

        // bool isFrontFacing = (area > 0.0f);
        const __m128 is_front_facing = _mm_cmpgt_ps(invTriArea, _mm_setzero_ps());

        const uint16_t mask = (const uint16_t)_mm_movemask_ps(is_front_facing);

        if (mask == 0x00)
            continue;
#else

#endif
        for (uint8_t mask_idx = 0; mask_idx < number_of_collected_triangles; mask_idx++)
        {
#ifndef COMPUTE_AREA_IN_RASTER
            if (mask & (1 << mask_idx)) // Check if the i-th bit is set
            {
#endif
                const size_t raster_triangle_store_idx = Trianges_To_Be_Rastered_Counter++;
                CHECK_ARRAY_BOUNDS(raster_triangle_store_idx, MAX_NUMBER_OF_TRIANGLES_TO_RASTER);

                RasterData_t *tri = &Trianges_To_Be_Rastered[raster_triangle_store_idx];

                /* Projection division... */
                tri->ss_v0 = _mm_setr_ps(X[0].m128_f32[mask_idx], Y[0].m128_f32[mask_idx], Z[0].m128_f32[mask_idx], W[0].m128_f32[mask_idx]);
                tri->ss_v1 = _mm_setr_ps(X[1].m128_f32[mask_idx], Y[1].m128_f32[mask_idx], Z[1].m128_f32[mask_idx], W[1].m128_f32[mask_idx]);
                tri->ss_v2 = _mm_setr_ps(X[2].m128_f32[mask_idx], Y[2].m128_f32[mask_idx], Z[2].m128_f32[mask_idx], W[2].m128_f32[mask_idx]);

#ifndef COMPUTE_AREA_IN_RASTER
                // tri->area = invTriArea.m128_f32[mask_idx];
            }
#endif
        }
    }
    //LOGERR("Number of triangles to be rastered : %zu\n", Trianges_To_Be_Rastered_Counter);
}