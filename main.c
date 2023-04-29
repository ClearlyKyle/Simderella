#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "obj.h"
#include "pgm.h"
#include "cglm/cglm.h"
#include "timer.h"
#include "tex.h"

#define NUMBER_OF_OBJECTS 1

#define WINDOW_H 512
#define WINDOW_W 1024

typedef uint8_t RGB[3];

RGB   frame_buffer[WINDOW_H * WINDOW_W] = {0};
float depth_buffer[WINDOW_H * WINDOW_W] = {0};

// Setup vertices & indices to draw an indexed cube
vec3 vertices[] =
    {
        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, 1.0f},
        {-1.0f, -1.0f, 1.0f},
        {-1.0f, -1.0f, -1.0f},
        {1.0f, 1.0f, -1.0f},
        {1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, 1.0f},
        {-1.0f, 1.0f, -1.0f},
};

const uint32_t indices[] =
    {
        1, 3, 0, 7, 5, 4, 4, 1, 0, 5, 2, 1, 2, 7, 3, 0, 7, 4, 1, 2, 3, 7, 6, 5, 4, 5, 1, 5, 6, 2, 2, 6, 7, 0, 3, 7};

#define TO_RASTER(v)                                                                     \
    {                                                                                    \
        (WINDOW_W * (v[0] + v[3]) / 2.0f), (WINDOW_H * (v[3] - v[1]) / 2.0f), v[2], v[3] \
    }

// move objects from object-space to clip-space
static inline void Object_Space_To_Clip_Space(vec3 vert, mat4 projection, mat4 view, mat4 model, vec4 clip_space, vec4 model_space)
{
    vec4 pos4 = {vert[0], vert[1], vert[2], 1.0f};

    glm_mat4_mulv(model, pos4, model_space);

    vec4 view_space;
    glm_mat4_mulv(view, model_space, view_space);
    glm_mat4_mulv(projection, view_space, clip_space);
}

static inline bool EvaluateEdgeFunction(const vec3 E, const vec3 sample)
{
    // Interpolate edge function at given sample
    float result = (E[0] * sample[0]) + (E[1] * sample[1]) + E[2];

    // Apply tie-breaking rules on shared vertices in order to avoid double-shading fragments
    if (result > 0.0f)
        return true;
    else if (result < 0.0f)
        return false;

    if (E[0] > 0.f)
        return true;
    else if (E[0] < 0.0f)
        return false;

    if ((E[0] == 0.0f) && (E[1] < 0.0f))
        return false;
    else
        return true;
}

static struct Mesh global_m = {0};

void Draw_mesh2()
{
    triang *triangles = Make_Triangles(&global_m);

    texture_t texture = Texture_Load("../../res/Wooden Box/crate_BaseColor.png", false);
    Texture_Print_Info(texture);

    timer_t t;
    Timer_Start(&t);

    /* Assume triangulated face. */
    tinyobj_attrib_t attrib        = global_m.attribute;
    const size_t     num_triangles = attrib.num_face_num_verts;

    mat4 transform = {0};
    // vec3 position  = {0.0f, 0.0f, 2.0f};
    vec3 axis = {0.0f, 1.0f, 0.0f};
    // glm_translate_make(transform, position);
    // glm_rotate(transform, glm_rad(45.0f), axis);

    mat4  view      = {0};
    mat4  proj      = {0};
    float nearPlane = 0.1f;
    float farPlane  = 100.f;
    vec3  eye       = {0.0f, 0.0f, 0.0f};
    vec3  lookat    = {0.0f, 0.0f, -1.0f};
    vec3  up        = {0.0f, 1.0f, 0.0f};

    glm_lookat_rh(eye, lookat, up, view);
    glm_perspective(glm_rad(60.f), (float)WINDOW_W / (float)WINDOW_H, nearPlane, farPlane, proj);

    mat4 MVP = {0};

    vec3 bmin, bmax;
    bmin[0] = bmin[1] = bmin[2] = FLT_MAX;
    bmax[0] = bmax[1] = bmax[2] = -FLT_MAX;

    for (size_t tri_index = 0; tri_index < num_triangles; tri_index++)
    {
        for (size_t i = 0; i < 3; i++) // Looping over x, y, z
        {
            /* Find the min of x, y, z */
            bmin[i] = (triangles[tri_index].v0[i] < bmin[i]) ? triangles[tri_index].v0[i] : bmin[i];
            bmin[i] = (triangles[tri_index].v1[i] < bmin[i]) ? triangles[tri_index].v1[i] : bmin[i];
            bmin[i] = (triangles[tri_index].v2[i] < bmin[i]) ? triangles[tri_index].v2[i] : bmin[i];

            /* Find the max of x, y, z */
            bmax[i] = (triangles[tri_index].v0[i] > bmax[i]) ? triangles[tri_index].v0[i] : bmax[i];
            bmax[i] = (triangles[tri_index].v1[i] > bmax[i]) ? triangles[tri_index].v1[i] : bmax[i];
            bmax[i] = (triangles[tri_index].v2[i] > bmax[i]) ? triangles[tri_index].v2[i] : bmax[i];
        }
    }

    printf("bmin (%f, %f, %f)\n", bmin[0], bmin[1], bmin[2]);
    printf("bmax (%f, %f, %f)\n", bmax[0], bmax[1], bmax[2]);

    float maxExtent = 0.5f * (bmax[0] - bmin[0]);
    if (maxExtent > 0.5f * (bmax[1] - bmin[1]))
    {
        maxExtent = 0.5f * (bmax[1] - bmin[1]);
    }
    if (maxExtent > 0.5f * (bmax[2] - bmin[2]))
    {
        maxExtent = 0.5f * (bmax[2] - bmin[2]);
    }
    /* Fit to -1, 1 */
    vec3 scale = {1.0f / maxExtent, 1.0f / maxExtent, 1.0f / maxExtent};

    /* object_size_factor - is a value between 0 and 1 that represents how much of the screen
    height you want your object to fill
        float desired_height = WINDOW_H *object_size_factor
    */
    // calculate the height of the bounding box
    float obj_height = bmax[1] - bmin[1];
    float obj_depth  = bmax[2] - bmin[2];
    printf("Object height: %f\n", obj_height);
    printf("Object depth : %f\n", obj_depth);

    // calculate the center of the bounding box
    vec3 center;
    glm_vec3_add(bmin, bmax, center);
    glm_vec3_scale(center, 0.5f, center);

    float FOVy = 60.0f * (float)M_PI / 180.0f; // example vertical FOV in radians

    // calculate distance to bounding box
    float push_amount = obj_height / (2.0f * tanf(FOVy / 2.0f));
    push_amount += (obj_depth / 2.0f);

    printf("Push amount: %f\n", push_amount);

    /* Centerize object. */
    vec3 position = {-0.5f * (bmax[0] + bmin[0]),
                     -0.5f * (bmax[1] + bmin[1]),
                     -(push_amount)};

    printf("position (%f, %f, %f)\n", position[0], position[1], position[2]);
    printf("scale (%f, %f, %f)\n", scale[0], scale[1], scale[2]);

    mat4 model;
    glm_scale_make(model, scale);
    glm_translate(model, position);

    for (size_t i = 0; i < num_triangles; i++)
    {
        vec4 v0Clip = {0};
        vec4 v1Clip = {0};
        vec4 v2Clip = {0};
        // vec4 clip_v0, clip_v1, clip_v2;
        vec4 ws_v0, ws_v1, ws_v2;
        Object_Space_To_Clip_Space(triangles[i].v0, proj, view, model, v0Clip, ws_v0);
        Object_Space_To_Clip_Space(triangles[i].v1, proj, view, model, v1Clip, ws_v1);
        Object_Space_To_Clip_Space(triangles[i].v2, proj, view, model, v2Clip, ws_v2);

        // Apply viewport transformation
        const vec4 v0Homogen = TO_RASTER(v0Clip);
        const vec4 v1Homogen = TO_RASTER(v1Clip);
        const vec4 v2Homogen = TO_RASTER(v2Clip);

        // Base vertex matrix
        mat3 M =
            {
                // Notice that glm is itself column-major)
                v0Homogen[0], v1Homogen[0], v2Homogen[0], //
                v0Homogen[1], v1Homogen[1], v2Homogen[1], //
                v0Homogen[3], v1Homogen[3], v2Homogen[3], //
            };

        // If det(M) == 0.0f, we'd perform division by 0 when calculating the invert matrix,
        // whereas (det(M) > 0) implies a back-facing triangle
        // const float det = glm_mat3_det(M);
        if (glm_mat3_det(M) >= 0.0f)
            continue;

        // Compute the inverse of vertex matrix to use it for setting up edge & constant functions
        glm_mat3_inv(M, M);

        // Set up edge functions based on the vertex matrix
        const float abs0 = 1.0f / (fabsf(M[0][0]) + fabsf(M[0][1]));
        const float abs1 = 1.0f / (fabsf(M[1][0]) + fabsf(M[1][1]));
        const float abs2 = 1.0f / (fabsf(M[2][0]) + fabsf(M[2][1]));
        vec3        E0   = {abs0 * M[0][0], abs0 * M[0][1], abs0 * M[0][2]};
        vec3        E1   = {abs1 * M[1][0], abs1 * M[1][1], abs1 * M[1][2]};
        vec3        E2   = {abs2 * M[2][0], abs2 * M[2][1], abs2 * M[2][2]};

        // Calculate constant function to interpolate 1/w
        vec3 C = {0};
        glm_mat3_mulv(M, (vec3){1.0f, 1.0f, 1.0f}, C);

        // Calculate z interpolation vector
        vec3 Z = {0};
        glm_mat3_mulv(M, (vec3){v0Clip[2], v1Clip[2], v2Clip[2]}, Z);

        // Calculate normal interpolation vector
        vec3 int_n0, int_n1, int_n2;
        glm_mat3_mulv(M, triangles[i].n0, int_n0);
        glm_mat3_mulv(M, triangles[i].n1, int_n1);
        glm_mat3_mulv(M, triangles[i].n2, int_n2);

        // Calculate UV interpolation vector
        vec3 int_t0, int_t1;
        glm_mat3_mulv(M, triangles[i].u, int_t0);
        glm_mat3_mulv(M, triangles[i].v, int_t1);

        // Start rasterizing by looping over pixels to output a per-pixel color
        for (auto y = 0; y < WINDOW_H; y++)
        {
            for (auto x = 0; x < WINDOW_W; x++)
            {
                // Sample location at the center of each pixel
                const vec3 sample = {x + 0.5f, y + 0.5f, 1.0f};

                // Evaluate edge functions at current fragment
                const bool inside0 = EvaluateEdgeFunction(E0, sample);
                const bool inside1 = EvaluateEdgeFunction(E1, sample);
                const bool inside2 = EvaluateEdgeFunction(E2, sample);

                // If sample is "inside" of all three half-spaces bounded by the three edges of the triangle, it's 'on' the triangle
                if (inside0 && inside1 && inside2)
                {
                    // Interpolate 1/w at current fragment
                    const float oneOverW = (C[0] * sample[0]) + (C[1] * sample[1]) + C[2];

                    // w = 1/(1/w)
                    const float w = 1.0f / oneOverW;

                    // Interpolate z that will be used for depth test
                    const float zOverW = (Z[0] * sample[0]) + (Z[1] * sample[1]) + Z[2];
                    const float z      = zOverW * w;

                    const int index = x + y * WINDOW_W;
                    if (z <= depth_buffer[index])
                    {
                        // Depth test passed; update depth buffer value
                        depth_buffer[index] = z;

                        // Final vertex attributes to be passed to FS
                        // {nx/w, ny/w, nz/w} * w -> {nx, ny, nz}
                        // {u/w, v/w} * w -> {u, v}
                        // Interpolate normal
                        const float nxOverW = ((int_n0[0] * sample[0]) + (int_n0[1] * sample[1]) + int_n0[2]) * w;
                        const float nyOverW = ((int_n1[0] * sample[0]) + (int_n1[1] * sample[1]) + int_n1[2]) * w;
                        const float nzOverW = ((int_n2[0] * sample[0]) + (int_n2[1] * sample[1]) + int_n2[2]) * w;

                        // Interpolate texture coordinates
                        const float uOverW = ((int_t0[0] * sample[0]) + (int_t0[1] * sample[1]) + int_t0[2]) * w;
                        const float vOverW = ((int_t1[0] * sample[0]) + (int_t1[1] * sample[1]) + int_t1[2]) * w;

                        // Pass interpolated normal & texture coordinates to FS
                        // FS here...
                        const uint32_t idxS = (uint32_t)((uOverW - (int64_t)(uOverW)) * texture.w - 0.5f);
                        const uint32_t idxT = (uint32_t)((vOverW - (int64_t)(vOverW)) * texture.h - 0.5f);

                        const uint32_t tex_index = (idxT * texture.w + idxS) * texture.bpp;

                        unsigned char *tex_data = texture.data + tex_index;

                        // Write new color at this fragment
                        frame_buffer[index][0] = tex_data[0]; // r
                        frame_buffer[index][1] = tex_data[1]; // g
                        frame_buffer[index][2] = tex_data[2]; // b
                    }
                }
            }
        }
    }

    PGM_Write("basic_raster.pgm", WINDOW_W, WINDOW_H, (void *)frame_buffer, sizeof(RGB), WINDOW_W * WINDOW_H);

    Timer_Stop(&t);
    printf("Time was : %f\n", Timer_Get_Elapsed_MS(&t));
}

int main(int argc, char const *argv[])
{
    printf("STARTING\n");

    // global_m = Initialise_Object("../../res/Wooden Box/wooden crate.obj");
    global_m = Initialise_Object("../../res/Teapot/teapot.obj");

    for (size_t i = 0; i < (WINDOW_H * WINDOW_W); i++)
    {
        depth_buffer[i] = FLT_MAX;
    }

    // Triangle_Setup();
    timer_t t;
    Timer_Start(&t);
    {
        // Draw_mesh();
        Draw_mesh2();
    }
    Timer_Stop(&t);
    printf("Time was : %f\n", Timer_Get_Elapsed_MS(&t));

    free_object(&global_m);
    printf("EXIT SUCCESS\n");

    return 0;
}
