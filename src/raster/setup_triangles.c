#include "renderer.h"
#include "utils/utils.h"

#define COMPUTE_AREA_IN_RASTER

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

    Trianges_To_Be_Rastered_Counter = 0;

    __m128              collected_vertices[4][3] = {0};
    VaryingAttributes_t collected_varying[4][3]  = {0};
    // tinyobj_vertex_index_t collected_indices[4][3];

    const size_t starting_index = 0;
    const size_t ending_index   = RenderState.index_buffer_length;
    const size_t vertex_stride  = RenderState.vertex_stride;

    ASSERT(RenderState.index_buffer_length > 0);
    ASSERT(vertex_stride > 0);

    const int *const index_buffer  = RenderState.index_buffer;
    uint32_t        *vertex_buffer = (uint32_t *)RenderState.vertex_buffer;

    for (size_t vert_idx = starting_index; vert_idx < ending_index; /* blank */)
    {
        size_t number_of_collected_triangles = 0;
        for (size_t i = 0; i < 4; i++)
        {
            if (vert_idx == ending_index)
                break;

            CHECK_ARRAY_BOUNDS(vert_idx, RenderState.index_buffer_length);

            /* Get 3 indices from the index buffer */
            const int vert0_index = (const int)index_buffer[vert_idx + 0];
            const int vert1_index = (const int)index_buffer[vert_idx + 1];
            const int vert2_index = (const int)index_buffer[vert_idx + 2];

            uint32_t *pVertIn0 = (uint32_t *)&vertex_buffer[vertex_stride * vert0_index];
            uint32_t *pVertIn1 = (uint32_t *)&vertex_buffer[vertex_stride * vert1_index];
            uint32_t *pVertIn2 = (uint32_t *)&vertex_buffer[vertex_stride * vert2_index];

            CHECK_ARRAY_BOUNDS(vertex_stride * vert0_index, RenderState.vertex_buffer_length);
            CHECK_ARRAY_BOUNDS(vertex_stride * vert1_index, RenderState.vertex_buffer_length);
            CHECK_ARRAY_BOUNDS(vertex_stride * vert2_index, RenderState.vertex_buffer_length);

            __m128 out_vertex0 = {0};
            __m128 out_vertex1 = {0};
            __m128 out_vertex2 = {0};
            VERTEX_SHADER((void *)pVertIn0, &collected_varying[i][0], RenderState.vertex_shader_uniforms, &RenderState.data_from_vertex_shader, &out_vertex0);
            VERTEX_SHADER((void *)pVertIn1, &collected_varying[i][1], RenderState.vertex_shader_uniforms, &RenderState.data_from_vertex_shader, &out_vertex1);
            VERTEX_SHADER((void *)pVertIn2, &collected_varying[i][2], RenderState.vertex_shader_uniforms, &RenderState.data_from_vertex_shader, &out_vertex2);

            collected_vertices[i][0] = mat4x4_mul_m128(RenderState.view_port_matrix, out_vertex0);
            collected_vertices[i][1] = mat4x4_mul_m128(RenderState.view_port_matrix, out_vertex1);
            collected_vertices[i][2] = mat4x4_mul_m128(RenderState.view_port_matrix, out_vertex2);

            ++number_of_collected_triangles;
            vert_idx += 3;
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

        // Z[0] = _mm_mul_ps(Z[0], W[0]);
        // Z[1] = _mm_mul_ps(Z[1], W[1]);
        // Z[2] = _mm_mul_ps(Z[2], W[2]);

#ifndef COMPUTE_AREA_IN_RASTER
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

                tri->varying[0] = collected_varying[mask_idx][0];
                tri->varying[1] = collected_varying[mask_idx][1];
                tri->varying[2] = collected_varying[mask_idx][2];

#ifndef COMPUTE_AREA_IN_RASTER
                tri->area = invTriArea.m128_f32[mask_idx];
            }
#endif
        }
    }
    // LOGERR("Number of triangles to be rastered : %zu\n", Trianges_To_Be_Rastered_Counter);
}