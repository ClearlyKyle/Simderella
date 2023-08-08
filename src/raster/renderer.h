#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <string.h>
#include <immintrin.h>

#include "obj.h"
#include "utils/mat4x4.h"
#include "shaders.h"

#define IMAGE_W   1024
#define IMAGE_H   512
#define IMAGE_BPP 4

typedef struct
{
    mat4x4 view_port_matrix;

    void *vertex_shader_uniforms;

    int   *index_buffer; // void?
    size_t index_buffer_length;

    void  *vertex_buffer;
    size_t vertex_stride;
    size_t vertex_buffer_length;

    VSOutputForFS_t data_from_vertex_shader; // To pass onto the FS

    uint8_t colour_buffer[IMAGE_W * IMAGE_H * IMAGE_BPP];
    float   depth_buffer[IMAGE_W * IMAGE_H];

} RendererState_t;

extern RendererState_t RenderState;

static inline void Render_Set_Viewport(int width, int height)
{
    Raster_View_Port_Matrix(RenderState.view_port_matrix, (float)width, (float)height);
}

typedef struct
{
    __m128 ss_v0, ss_v1, ss_v2; /* Screen Space */
    // float               area;
    VaryingAttributes_t varying[3];
} RasterData_t;

#define MAX_NUMBER_OF_TRIANGLES_TO_RASTER 4096
extern RasterData_t Trianges_To_Be_Rastered[MAX_NUMBER_OF_TRIANGLES_TO_RASTER];
extern size_t       Trianges_To_Be_Rastered_Counter;

void Raster_Triangles_MT(void);

typedef struct
{
    mat4x4 *cum_matrix;
    mat4x4 *view_port_matrix; // NOTE: Should this be set in RendererState? it doesnt change
    mat4x4 *MVP;

    size_t max_number_of_triangles;
    size_t stride;
    size_t starting_index;
    size_t ending_index;
} SetupData_t;

void Setup_Triangles_For_MT(void);

inline void Framebuffer_Clear_Depth()
{
#if 1 /* Clear the Depth Buffer with set value */
    const __m256 max_depth    = _mm256_set1_ps(FLT_MAX);
    const int    num_pixels   = IMAGE_W * IMAGE_H;
    float       *depth_buffer = RenderState.depth_buffer;

    for (int i = 0; i < num_pixels; i += 8)
        _mm256_store_ps(&depth_buffer[i], max_depth);
#else
    /* kinda a cheese, setting the depth to a large value */
    memset((void *)RenderState.depth_buffer, 0x7777, sizeof(float) * IMAGE_W * IMAGE_H);
#endif
}

inline void Framebuffer_Clear_Both()
{
    // Clear the Colour buffer
    memset((void *)RenderState.colour_buffer, 0, sizeof(uint8_t) * IMAGE_W * IMAGE_H * IMAGE_BPP);

    Framebuffer_Clear_Depth();
}

#endif // __RENDERER_H__