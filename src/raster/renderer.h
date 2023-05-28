#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <immintrin.h>

#include "obj.h"
#include "utils/mat4x4.h"
#include "VS.h"
#include "FS.h"

#define IMAGE_W   1024
#define IMAGE_H   512
#define IMAGE_BPP 4

typedef struct
{
    mat4 view_port_matrix;

    void  *vertex_shader_uniforms;
    float *vertices;

    int     *indices;
    uint32_t number_of_indices;

    uint32_t vertex_stride;
    uint32_t number_of_vertices;

    uint8_t colour_buffer[IMAGE_W * IMAGE_H * IMAGE_BPP];
    float   depth_buffer[IMAGE_W * IMAGE_H];

} RendererState_t;

extern RendererState_t RenderState;

static inline void Render_Set_Viewport(int width, int height)
{
    Raster_View_Port_Matrix(RenderState.view_port_matrix, (float)width, (float)height);
}

#include "VS.h"
#include "FS.h"

typedef struct
{
    __m128 ss_v0, ss_v1, ss_v2; /* Screen Space */

    float tex_u[3];
    float tex_v[3];
} RasterData_t;

#define MAX_NUMBER_OF_TRIANGLES_TO_RASTER 4096
extern RasterData_t Trianges_To_Be_Rastered[MAX_NUMBER_OF_TRIANGLES_TO_RASTER];
extern size_t       Trianges_To_Be_Rastered_Counter;

void Raster_Triangles(void);

typedef struct
{
    mat4x4 *cum_matrix;
    mat4x4 *view_port_matrix;
    mat4x4 *MVP;

    size_t max_number_of_triangles;
    size_t stride;
    size_t starting_index;
    size_t ending_index;
} SetupData_t;

void Setup_Triangles(void);

#endif // __RENDERER_H__