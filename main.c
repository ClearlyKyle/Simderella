#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "raster/graphics.h"
#include "raster/renderer.h"

#include "job_system/js.h"

#include "raster/light.h"
#include "utils/mat4x4.h"
#include "utils/utils.h"

// TODO : Fix jobs full error

static inline void BindVertexBuffer(void *vertex_buffer, const size_t stride)
{
    ASSERT(vertex_buffer);
    ASSERT(stride > 0);

    RenderState.vertex_buffer = vertex_buffer;
    RenderState.vertex_stride = stride;
}

static inline void BindIndexBuffer(int *index_buffer, const size_t length)
{
    ASSERT(index_buffer);
    ASSERT(length > 0);

    RenderState.index_buffer        = index_buffer;
    RenderState.index_buffer_length = length;
}

void Convert_Depth_Buffer_For_Drawing(void)
{
    // Define the minimum and maximum depth values in your depth buffer
    const float minDepth = 0.0f /* Set the minimum depth value */;
    const float maxDepth = 10.0f /* Set the maximum depth value */;

#if 0
    // Iterate over each depth value in the depth buffer
     for (size_t i = 0; i < IMAGE_W * IMAGE_H; ++i)
    {
         const float depthValue = RenderState.depth_buffer[i];

        if (depthValue == FLT_MAX)
            continue;

        // Normalize the depth value between 0 and 1
        const float normalizedDepth = ((depthValue - minDepth) / (maxDepth - minDepth));

        // Map the normalized depth value to the range of 0 to 255
        const uint8_t colorValue = (uint8_t)(normalizedDepth * 255.0f);

        // Assign the color value to the RGBA color buffer
        RenderState.colour_buffer[i * 4 + 0] = colorValue; // Red component
        RenderState.colour_buffer[i * 4 + 1] = colorValue; // Green component
        RenderState.colour_buffer[i * 4 + 2] = colorValue; // Blue component
        RenderState.colour_buffer[i * 4 + 3] = 255;        // Alpha component (fully opaque)
    }
#else
    // Load the minimum and maximum depth values into SIMD registers
    __m128 minDepthVec = _mm_set1_ps(minDepth);
    __m128 maxDepthVec = _mm_set1_ps(maxDepth);

    // Iterate over each depth value in the depth buffer (processing 4 values at a time)
    for (size_t i = 0; i < IMAGE_W * IMAGE_H; i += 4)
    {
        // Load 4 depth values from the depth buffer into a SIMD register
        __m128 depthVec = _mm_load_ps(&RenderState.depth_buffer[i]);

        __m128 cmp = _mm_cmpeq_ps(depthVec, _mm_set1_ps(FLT_MAX));

        if (_mm_testz_si128(_mm_cvtps_epi32(cmp), _mm_cvtps_epi32(cmp)) == 0)
            continue;

        // Normalize the depth values between 0 and 1
        __m128 normalizedDepthVec = _mm_div_ps(_mm_sub_ps(depthVec, minDepthVec), _mm_sub_ps(maxDepthVec, minDepthVec));

        // Map the normalized depth values to the range of 0 to 255
        __m128i colorValueVec = _mm_cvtps_epi32(_mm_mul_ps(normalizedDepthVec, _mm_set1_ps(255.0f)));

        // Convert the packed integer values to 8-bit unsigned integers
        __m128i colorValueU8Vec = _mm_packus_epi32(colorValueVec, colorValueVec);
        colorValueU8Vec         = _mm_packus_epi16(colorValueU8Vec, colorValueU8Vec);

        // Store the color values into the RGBA color buffer
        _mm_storeu_si128((__m128i *)&RenderState.colour_buffer[i], colorValueU8Vec);
    }
#endif
}

int main(int argc, char *argv[])
{
    argc = 0;
    argv = NULL;

    DEBUG_MODE_PRINT;

    if (!Reneder_Startup("Simderella", IMAGE_W, IMAGE_H))
    {
        fprintf(stderr, "Error with Reneder_Startup\n");
        return EXIT_FAILURE;
    }

    jobs_init();

    /* Load a object */
    struct Mesh obj = Mesh_Load("../../res/Wooden Box/wooden crate.obj");
    // struct Mesh obj = Mesh_Load("../../res/Teapot/teapot.obj");
    // struct Mesh obj = Mesh_Load("../../res/sponza/sponza.obj"); // Big boy
    // struct Mesh obj = Mesh_Load("../../res/sponza_small/sponza.obj"); // Big boy
    // struct Mesh obj = Mesh_Load("../../res/dragon.obj"); // Big boy

    vec3 cam_position = {0.0f, 0.0f, 3.5f};

    mat4x4 view, proj;
    Raster_View_Matrix(view, cam_position);
    Raster_Projection_Matrix(proj, IMAGE_W, IMAGE_H);

    timer_t rasterizer_timer;
    Timer_Start(&rasterizer_timer);

    vec3 light_position = {0.0f, 0.0f, 6.0f};
    vec3 diffuse_colour = {0.2f, 0.2f, 0.2f};

    struct ShadingData sd = {0};
    sd.diffuse_colour     = &diffuse_colour;
    sd.cam_position       = &cam_position;
    sd.light_position     = &light_position;
    sd.ambient_amount     = 0.3f;
    sd.specular_amount    = 1.0f;
    sd.shininess          = 128.0f;

    // Side quest to get vertex index data...
    float *vertex_data = malloc(sizeof(float) * obj.attribute.num_faces * 5); // 3 for vert, 2 for tex
    int   *index_data  = malloc(sizeof(int) * obj.attribute.num_faces);
    for (size_t i = 0; i < obj.attribute.num_faces; i++)
    {
        tinyobj_vertex_index_t face = obj.attribute.faces[i];
        index_data[i]               = (int)i; // NOTE : We are not creating unique vertex data here

        vertex_data[i * 5 + 0] = obj.attribute.vertices[face.v_idx * 3 + 0];
        vertex_data[i * 5 + 1] = obj.attribute.vertices[face.v_idx * 3 + 1];
        vertex_data[i * 5 + 2] = obj.attribute.vertices[face.v_idx * 3 + 2];
        vertex_data[i * 5 + 3] = obj.attribute.texcoords[face.vt_idx * 2 + 0];
        vertex_data[i * 5 + 4] = obj.attribute.texcoords[face.vt_idx * 2 + 1];
    }

    /* Convert obj format to {posX, posY, posZ}{texU, texV} */

    UniformData_t uniform_data;
    uniform_data.diffuse = obj.diffuse_tex;

    RenderState.vertex_shader_uniforms = (void *)&uniform_data;

    BindIndexBuffer(index_data, obj.attribute.num_faces);
    BindVertexBuffer((void *)vertex_data, 5);

    RenderState.vertex_buffer_length = obj.attribute.num_faces * 5;

    Render_Set_Viewport(IMAGE_W, IMAGE_H);

    float fTheta              = 0.0f;
    bool  render_depth_buffer = false;
    while (global_renderer.running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if ((SDL_QUIT == event.type) ||
                (SDL_KEYDOWN == event.type && SDL_SCANCODE_ESCAPE == event.key.keysym.scancode))
            {
                global_renderer.running = false;
                break;
            }
            if (SDL_KEYDOWN == event.type && SDL_SCANCODE_D == event.key.keysym.scancode)
            {
                render_depth_buffer = !render_depth_buffer;
                break;
            }
        }

        fTheta += (float)Timer_Get_Elapsed_MS(&rasterizer_timer) / 32.0f;

        { // Update the MVP matrix for the Vertex Shader
            mat4x4 model;
            dash_translate_make(model, 0.0f, 0.0f, 0.0f);
            dash_rotate(model, glm_rad(fTheta), (vec3){0.0f, 1.0f, 0.0f});

            mat4x4 MVP;
            dash_mat_mul_mat(view, model, MVP);
            dash_mat_mul_mat(proj, MVP, MVP);

            dash_mat_copy(MVP, uniform_data.MVP);
        }

        /* Update Scene here */
        Setup_Triangles_For_MT();
        Raster_Triangles_MT();

        // Update the pixels of the surface with the color buffer data
        ASSERT(global_renderer.screen_num_pixels == IMAGE_W * IMAGE_H * IMAGE_BPP);

        if (render_depth_buffer) /* Draw Depth buffer */
            Convert_Depth_Buffer_For_Drawing();

/* Draw Colour buffer */
#ifdef GRAPHICS_USE_SDL_RENDERER
        SDL_UpdateTexture(global_renderer.texture, NULL, RenderState.colour_buffer, IMAGE_W * IMAGE_BPP);
        SDL_RenderCopy(global_renderer.renderer, global_renderer.texture, NULL, NULL);
        SDL_RenderPresent(global_renderer.renderer);
#else
        memcpy_s(global_renderer.pixels, global_renderer.screen_num_pixels, RenderState.colour_buffer, IMAGE_W * IMAGE_H * IMAGE_BPP);
        SDL_UpdateWindowSurface(global_renderer.window);
#endif

        Timer_Update(&rasterizer_timer);

        // TODO: Average this over X loops
        char buff[16] = {0};
        sprintf_s(buff, 16, "%fms", Timer_Get_Elapsed_MS(&rasterizer_timer));
        Renderer_Set_Title(buff);
    }

    free(index_data);
    free(vertex_data);

    Mesh_Destroy(&obj);
    Renderer_Destroy();
    jobs_shutdown();

    printf("EXIT_SUCCESS\n");
    return EXIT_SUCCESS;
}
