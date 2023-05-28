#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "raster/graphics.h"
#include "raster/renderer.h"

#include "raster/light.h"
#include "utils/mat4x4.h"
#include "utils/utils.h"

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

    /* Load a object */
    struct Mesh obj = Mesh_Load("../../res/Wooden Box/wooden crate.obj");
    // struct Mesh obj = Mesh_Load("../../res/Teapot/teapot.obj");
    // struct Mesh obj = Mesh_Load("../../res/sponza/sponza.obj"); // Big boy
    // struct Mesh obj = Mesh_Load("../../res/sponza_small/sponza.obj"); // Big boy
    // struct Mesh obj = Mesh_Load("../../res/dragon.obj"); // Big boy

    vec3 cam_position = {-1.5f, -3.0f, 1.0f};

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
    int *index_data = malloc(sizeof(int) * obj.attribute.num_faces);
    for (size_t i = 0; i < obj.attribute.num_faces; i++)
    {
        index_data[i] = obj.attribute.faces[i].v_idx;
    }

    UniformData_t uniform_data;

    RenderState.vertex_shader_uniforms = (void *)&uniform_data;

    RenderState.indices           = index_data;
    RenderState.number_of_indices = obj.attribute.num_faces;

    RenderState.number_of_vertices = obj.attribute.num_vertices;
    RenderState.vertices           = obj.attribute.vertices;
    RenderState.vertex_stride      = 3;

    Render_Set_Viewport(IMAGE_W, IMAGE_H);

    float fTheta = 0.0f;

    while (global_renderer.running)
    {
        // Clear z_buffer
        Renderer_Clear_Depth_Buffer();

        // Clear Screen
        Renderer_Clear_Screen_Pixels();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if ((SDL_QUIT == event.type) ||
                (SDL_KEYDOWN == event.type && SDL_SCANCODE_ESCAPE == event.key.keysym.scancode))
            {
                global_renderer.running = false;
                break;
            }
        }

        fTheta += (float)Timer_Get_Elapsed_MS(&rasterizer_timer) / 32.0f;

        { // Update the MVP matrix for the Vertex Shader
            mat4x4 model;
            dash_translate_make(model, 0.0f, 0.0f, 0.0f);
            dash_rotate(model, glm_rad(fTheta), (vec3){0.0f, 1.0f, 0.0f});

            // mat4 model;
            // glm_translate_make(model, (vec3){0.0f, 0.0f, 0.0f});
             glm_rotate(model, glm_rad(fTheta), (vec3){0.0f, 1.0f, 0.0f});
            // mat4x4 model2;
            // mat4_to_mat4x4(model, model2);

            mat4x4 MVP;
            dash_mat_mul_mat(view, model, MVP);
            dash_mat_mul_mat(proj, MVP, MVP);

            dash_mat_copy(MVP, uniform_data.MVP);
        }

        /* Update Scene here */
        Setup_Triangles();
        Raster_Triangles();

        // SDL_UpdateTexture(global_renderer.texture, NULL, RenderState.colour_buffer, IMAGE_W * IMAGE_BPP);
        // SDL_RenderClear(global_renderer.renderer);
        // SDL_RenderCopy(global_renderer.renderer, global_renderer.texture, NULL, NULL);
        // SDL_RenderPresent(global_renderer.renderer);

        // Update the pixels of the surface with the color buffer data
        assert(global_renderer.screen_num_pixels != IMAGE_W * IMAGE_H * IMAGE_BPP);
        memcpy_s(global_renderer.pixels, global_renderer.screen_num_pixels * global_renderer.fmt->BitsPerPixel, RenderState.colour_buffer, IMAGE_W * IMAGE_H * IMAGE_BPP);
        SDL_UpdateWindowSurface(global_renderer.window);

        Timer_Update(&rasterizer_timer);

        char buff[16] = {0};
        sprintf_s(buff, 16, "%fms", Timer_Get_Elapsed_MS(&rasterizer_timer));
        Renderer_Set_Title(buff);
    }

    free(index_data);

    Mesh_Destroy(&obj);
    Renderer_Destroy();

    return EXIT_SUCCESS;
}
