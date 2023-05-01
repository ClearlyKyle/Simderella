#ifndef __RASTER_H__
#define __RASTER_H__

#include "obj.h"

#include "cglm/cglm.h"
#include "graphics.h"
#include "timer.h"
#include "light.h"

static inline void Raster_View_Matrix(mat4 view, vec3 eye)
{
    vec3 lookat = {0.0f, 0.0f, 0.0f};
    vec3 up     = {0.0f, 1.0f, 0.0f};

    glm_lookat(eye, lookat, up, view);
}

static inline void Raster_Projection_Matrix(mat4 proj)
{
    float nearPlane = 0.1f;
    float farPlane  = 100.f;
    float aspect    = (float)global_renderer.width / (float)global_renderer.height;
    float FOVh      = 60.0f; // Horizontal FOV

    glm_perspective(glm_rad(FOVh), aspect, nearPlane, farPlane, proj);
}

void Raster_Model(struct Mesh *obj, struct ShadingData sd, mat4 model, mat4 view, mat4 proj);

#endif // __RASTER_H__