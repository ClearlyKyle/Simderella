#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "cglm/cglm.h"

struct ShadingData
{
    vec3 *diffuse_colour; // Normaly from a texture file
    vec3 *position;       // Can be vertex or fragment
    vec3 *normal;

    vec3 *cam_position;
    vec3 *light_position;
    float ambient_amount;
    float specular_amount;
    float shininess; // specular "pow"
};

static inline void glm_vec3_reflect(vec3 I, vec3 N, vec3 dest)
{
    const float dot = glm_vec3_dot(N, I) * 2.0f;

    glm_vec3_scale(N, dot, dest);
    glm_vec3_sub(I, dest, dest);
}

static inline float Calculate_Diffuse_Amount(vec3 L, vec3 N)
{
    const float dot_product = glm_vec3_dot(L, N);
    return fmaxf(dot_product, 0.0f);
}

static inline float Calculate_Specular_Amount(vec3 L, vec3 E, vec3 N, const float shininess)
{
#if 0
    {
        // BLINN PHONG
        vec3 halfway_direction;
        glm_vec3_add(L, E, halfway_direction);
        glm_normalize(halfway_direction);

        const float specular_intensity = (const float)pow(fmax(glm_vec3_dot(N, halfway_direction), 0.0), shininess);
        return specular_intensity;
    }
#else
    {
        // Regular PHONG
        vec3 R, neg_L;
        glm_vec3_negate_to(L, neg_L);
        glm_vec3_reflect(neg_L, N, R);

        const float specular_intensity = powf(fmaxf(glm_vec3_dot(E, R), 0.0f), shininess);
        return specular_intensity;
    }
#endif
}

static void Calculate_Shading(struct ShadingData data, vec3 out_shading)
{
    //  Normalise the Noraml - N
    vec3 N = {0};
    glm_vec3_normalize_to(*data.normal, N);

    // Calculate L - direction to the light source
    vec3 L = {0};
    glm_vec3_sub(*data.light_position, *data.position, L);
    glm_normalize(L);

    // Calculate E - view direction
    vec3 E = {0};
    glm_vec3_sub(*data.cam_position, *data.position, E);
    glm_normalize(E);

    // Calculate Ambient Term:
    vec3 Iamb = {0};
    glm_vec3_scale(*data.diffuse_colour, data.ambient_amount, Iamb);

    // Calculate Diffuse Term:
    vec3        Idiff          = {0};
    const float diffuse_amount = Calculate_Diffuse_Amount(L, N);
    glm_vec3_scale(*data.diffuse_colour, diffuse_amount, Idiff);

    // Calculate Specular Term:
    vec3        Ispec    = {0};
    const float specular = Calculate_Specular_Amount(L, E, N, data.shininess) * data.specular_amount;
    glm_vec3_broadcast(specular, Ispec);

#if 0
    vec3 red = {1.0f, 0.0f, 0.0f};
    vec3 gre = {0.0f, 1.0f, 0.0f};
    vec3 blu = {0.0f, 0.0f, 1.0f};

    glm_vec3_scale(red, data.ambient_amount, Iamb);
    glm_vec3_scale(gre, diffuse_amount / 2.0f, Idiff);
    glm_vec3_scale(blu, specular, Ispec); // Scale with red vector instead of blu

    glm_vec3_add(Iamb, Idiff, out_shading);
    glm_vec3_add(out_shading, Ispec, out_shading); // Add Ispec to out_shading instead of Ispec to Ispec

    glm_vec3_clamp(out_shading, 0.0f, 1.0f);
#else
    glm_vec3_add(Iamb, Idiff, out_shading);
    glm_vec3_add(out_shading, Ispec, out_shading);
    glm_vec3_clamp(out_shading, 0.0f, 1.0f);
#endif
}

#endif // __LIGHT_H__