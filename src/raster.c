#include "raster.h"

// Test for a particular (x, y) position being on the "inside"
// of a particular halfspace equation, with tie-breaking tests.
static inline bool Edge_Tie_Breaker(const vec3 E, const float edge_value)
{
    // Apply tie-breaking rules on shared vertices in order to avoid double-shading fragments
    // if (edge_value > 0.0f ||
    //    (edge_value == 0.0f && E[0] > 0.0f) ||
    //    (edge_value == 0.0f && E[0] == 0.0f && E[1] >= 0.0f))
    //    return true;
    // else
    //    return false;
    if (edge_value > 0.0f)
        return true;
    if (edge_value < 0.0f)
        return false;
    if (E[0] > 0.0f)
        return true;
    if (E[0] < 0.0f)
        return false;
    if (E[0] == 0.0f && E[1] < 0.0f)
        return false;
    return true;
}

static inline void Object_Space_To_Clip_Space(vec3 vert, mat4 projection, mat4 view, mat4 model, vec4 clip_space, vec4 model_space)
{
    vec4 pos4 = {vert[0], vert[1], vert[2], 1.0f};

    glm_mat4_mulv(model, pos4, model_space);

    vec4 view_space;
    glm_mat4_mulv(view, model_space, view_space);
    glm_mat4_mulv(projection, view_space, clip_space);
}

static inline void Clip_Space_To_Raster(vec4 clip, vec4 raster)
{
    raster[0] = (float)global_renderer.width * ((clip[0] + clip[3]) * 0.5f);
    raster[1] = (float)global_renderer.height * ((clip[1] + clip[3]) * 0.5f);
    raster[2] = clip[2];
    raster[3] = clip[3];
}

// Convert clip space coordinates to NDC [-1, 1]
static inline void Clip_Space_To_NDC(vec4 clip, vec4 ndc)
{
    const float inv_clip_w = 1.0f / clip[3];
    glm_vec4_scale(clip, inv_clip_w, ndc);
    // ndc[3] = clip[3]; // Do I need to save this?
}

static inline void Compute_Bounding_Box(vec4 clip_v0, vec4 clip_v1, vec4 clip_v2, ivec4 AABB)
{
    vec4 ndc_v0, ndc_v1, ndc_v2;
    Clip_Space_To_NDC(clip_v0, ndc_v0);
    Clip_Space_To_NDC(clip_v1, ndc_v1);
    Clip_Space_To_NDC(clip_v2, ndc_v2);

    // Transform NDC [-1, 1] -> RASTER [0, {width|height}]
    vec2 v0Raster = {(float)global_renderer.width * (ndc_v0[0] + 1.0f) * 0.5f,
                     (float)global_renderer.height * (ndc_v0[1] + 1.0f) * 0.5f};

    vec2 v1Raster = {(float)global_renderer.width * (ndc_v1[0] + 1.0f) * 0.5f,
                     (float)global_renderer.height * (ndc_v1[1] + 1.0f) * 0.5f};

    vec2 v2Raster = {(float)global_renderer.width * (ndc_v2[0] + 1.0f) * 0.5f,
                     (float)global_renderer.height * (ndc_v2[1] + 1.0f) * 0.5f};

    const int maxX = (const int)ceilf(fmaxf(v0Raster[0], fmaxf(v1Raster[0], v2Raster[0])));
    const int minX = (const int)floorf(fminf(v0Raster[0], fminf(v1Raster[0], v2Raster[0])));
    const int maxY = (const int)ceilf(fmaxf(v0Raster[1], fmaxf(v1Raster[1], v2Raster[1])));
    const int minY = (const int)floorf(fminf(v0Raster[1], fminf(v1Raster[1], v2Raster[1])));

    AABB[0] = minX;
    AABB[1] = minY;
    AABB[2] = maxX;
    AABB[3] = maxY;
}

static inline float Interpolate_Attribute(vec4 littlef_values, vec4 attribute)
{
    return glm_vec4_dot(littlef_values, attribute);
    // return littlef_values[0] * attribute[0] + littlef_values[1] * attribute[1] + littlef_values[2] * attribute[2];
}

static inline void Interpolate_Vertex_Attribute(vec4 littlef_values, vec4 attribute0, vec4 attribute1, vec4 attribute2, vec4 res)
{
    res[0] = glm_vec4_dot(littlef_values, (vec4){attribute0[0], attribute1[0], attribute2[0], 0.0f});
    res[1] = glm_vec4_dot(littlef_values, (vec4){attribute0[1], attribute1[1], attribute2[1], 0.0f});
    res[2] = glm_vec4_dot(littlef_values, (vec4){attribute0[2], attribute1[2], attribute2[2], 0.0f});
}

void Raster_Model(struct Mesh *obj, struct ShadingData sd, mat4 model, mat4 view, mat4 proj)
{
    mat3 nrm_mat;
    glm_mat4_pick3(model, nrm_mat);
    glm_mat3_inv(nrm_mat, nrm_mat);
    glm_mat3_transpose(nrm_mat);

    // Loop over triangles in a given object and rasterize them one by one
    for (uint32_t idx = 0; idx < obj->number_of_triangles; idx++)
    {
        triang triangle = obj->triangle[idx];

        // TODO: Align these?
        vec4 cs_v0, cs_v1, cs_v2;
        vec4 ws_v0, ws_v1, ws_v2;
        Object_Space_To_Clip_Space(triangle.v0, proj, view, model, cs_v0, ws_v0);
        Object_Space_To_Clip_Space(triangle.v1, proj, view, model, cs_v1, ws_v1);
        Object_Space_To_Clip_Space(triangle.v2, proj, view, model, cs_v2, ws_v2);

        // "raster" space, what ever that means
        vec4 raster_v0, raster_v1, raster_v2;
        Clip_Space_To_Raster(cs_v0, raster_v0);
        Clip_Space_To_Raster(cs_v1, raster_v1);
        Clip_Space_To_Raster(cs_v2, raster_v2);

        /* From the paper, calculate out "A" matrix */
        const float a0 = (raster_v1[1] * raster_v2[3]) - (raster_v2[1] * raster_v1[3]);
        const float a1 = (raster_v2[1] * raster_v0[3]) - (raster_v0[1] * raster_v2[3]);
        const float a2 = (raster_v0[1] * raster_v1[3]) - (raster_v1[1] * raster_v0[3]);

        const float b0 = (raster_v2[0] * raster_v1[3]) - (raster_v1[0] * raster_v2[3]);
        const float b1 = (raster_v0[0] * raster_v2[3]) - (raster_v2[0] * raster_v0[3]);
        const float b2 = (raster_v1[0] * raster_v0[3]) - (raster_v0[0] * raster_v1[3]);

        const float c0 = (raster_v1[0] * raster_v2[1]) - (raster_v2[0] * raster_v1[1]);
        const float c1 = (raster_v2[0] * raster_v0[1]) - (raster_v0[0] * raster_v2[1]);
        const float c2 = (raster_v0[0] * raster_v1[1]) - (raster_v1[0] * raster_v0[1]);

        const float detM = (c0 * raster_v0[3]) + (c1 * raster_v1[3]) + (c2 * raster_v2[3]);

        /*
        The sign of the determinant gives
        the orientation of the triangle: a counterclockwise order gives a positive determinant
        */
        // det(M) == 0 -> degenerate/zero-area triangle
        // det(M) < 0  -> back-facing triangle
        if (detM <= 0.0f)
            continue;

        // Set up edge functions (this is A = adj(M))
        vec4 E0 = {a0, b0, c0, 0.0f};
        vec4 E1 = {a1, b1, c1, 0.0f};
        vec4 E2 = {a2, b2, c2, 0.0f};

        /* get the bounding box of the triangle */
        ivec4 AABB; /* minX, minY, maxX, maxY*/
        Compute_Bounding_Box(cs_v0, cs_v1, cs_v2, AABB);

        /* Transform our normals using the "normals matrix" this is needed due to
            the model matrix having the ability to scale and scew the model, and our
            normal do not like this */
        vec4 ws_n0, ws_n1, ws_n2;
        glm_mat3_mulv(nrm_mat, triangle.n0, ws_n0);
        glm_mat3_mulv(nrm_mat, triangle.n1, ws_n1);
        glm_mat3_mulv(nrm_mat, triangle.n2, ws_n2);

        /* Texture coordinates */
        vec4 tu, tv;
        glm_vec4(triangle.u, 0.0f, tu);
        glm_vec4(triangle.v, 0.0f, tv);

        // Evaluaate edge equation at first tile origin
        const float edgeFunc0 = (E0[0] * (float)AABB[0]) + (E0[1] * (float)AABB[1]) + E0[2];
        const float edgeFunc1 = (E1[0] * (float)AABB[0]) + (E1[1] * (float)AABB[1]) + E1[2];
        const float edgeFunc2 = (E2[0] * (float)AABB[0]) + (E2[1] * (float)AABB[1]) + E2[2];

        // Start rasterizing by looping over pixels to output a per-pixel color
        for (size_t y = AABB[1], step_y = 0; y < AABB[3]; y++, step_y++)
        {
            for (size_t x = AABB[0], step_x = 0; x < AABB[2]; x++, step_x++)
            {
                // Step from edge function by multiples of the edge values
                // edgevalue * step_multiple

                // Evaluate edge functions at current fragment
                // E(x + s, y + t) = E(x, y) + sa + tb,
                const float edgeFuncTR0 = edgeFunc0 + ((E0[0] * step_x) + (E0[1] * step_y));
                const float edgeFuncTR1 = edgeFunc1 + ((E1[0] * step_x) + (E1[1] * step_y));
                const float edgeFuncTR2 = edgeFunc2 + ((E2[0] * step_x) + (E2[1] * step_y));

                // Check if the current point is inside a traingle using Tie breaker rules
                const bool TRForEdge0 = Edge_Tie_Breaker(E0, edgeFuncTR0);
                const bool TRForEdge1 = Edge_Tie_Breaker(E1, edgeFuncTR1);
                const bool TRForEdge2 = Edge_Tie_Breaker(E2, edgeFuncTR2);

                // If sample is "inside" of all three half-spaces bounded by the three
                // edges of the triangle, it's 'on' the triangle
                if (!TRForEdge0 || !TRForEdge1 || !TRForEdge2)
                    continue;

                /* Calculate Interpolation Coefficients */
                const float F0 = edgeFuncTR0;
                const float F1 = edgeFuncTR1;
                const float F2 = edgeFuncTR2;

                const float little_r = 1.0f / (F0 + F1 + F2);

                vec4 littlef_values = {edgeFuncTR0 * little_r, edgeFuncTR1 * little_r, edgeFuncTR2 * little_r, 0.0f};

                /* Interpolate Depth value */
                const float z = Interpolate_Attribute(littlef_values, (vec4){raster_v0[2], raster_v1[2], raster_v2[2], 0.0f});
                // z       = 1.0f / z;

                const size_t index = y * global_renderer.width + x;

                // Perform depth test
                if (z < global_renderer.depth_buffer[index])
                {
                    // Depth test passed; update depth buffer value
                    global_renderer.depth_buffer[index] = z;

                    /* Interpolate attributes (tex, normals, colours...) */

                    /* Lets try some Phong shading */
                    vec3 frag_position; // World Space Frag position
                    Interpolate_Vertex_Attribute(littlef_values, ws_v0, ws_v1, ws_v2, frag_position);

                    vec3 frag_normal;
                    Interpolate_Vertex_Attribute(littlef_values, ws_n0, ws_n1, ws_n2, frag_normal);
                    glm_normalize(frag_normal);

                    unsigned char *texRGB;
                    if (obj->diffuse_tex != NULL)
                    {
                        // Interpolate texture coordinates
                        const float u = Interpolate_Attribute(littlef_values, tu);
                        const float v = Interpolate_Attribute(littlef_values, tv);

                        const int tex_u = (const int)((u) * (float)(obj->diffuse_tex->w - 1));
                        const int tex_v = (const int)((v) * (float)(obj->diffuse_tex->h - 1));

                        texRGB = Texture_Get_Pixel(*obj->diffuse_tex, tex_u, tex_v);

                        // vec3 diffuse_colour = {texRGB[0], texRGB[1], texRGB[2]};
                        // sd.diffuse_colour   = &diffuse_colour;
                    }

                    sd.normal   = &frag_normal;
                    sd.position = &frag_position;

                    // vec3 shading_amount;
                    // Calculate_Shading(sd, shading_amount);

                    // Combine to get final colour (convert back to 0-255 range)
                    // const uint8_t col_r = (uint8_t)((float)texRGB[0] * shading_amount[0]);
                    // const uint8_t col_g = (uint8_t)((float)texRGB[1] * shading_amount[1]);
                    // const uint8_t col_b = (uint8_t)((float)texRGB[2] * shading_amount[2]);
                    // const uint8_t col_r = (uint8_t)(255.0f * shading_amount[0]);
                    // const uint8_t col_g = (uint8_t)(255.0f * shading_amount[1]);
                    // const uint8_t col_b = (uint8_t)(255.0f * shading_amount[2]);
                    const uint8_t col_r = (uint8_t)(texRGB[0]);
                    const uint8_t col_g = (uint8_t)(texRGB[1]);
                    const uint8_t col_b = (uint8_t)(texRGB[2]);

                    // Draw the pixel finally
                    Draw_Pixel_RGBA((const int)x, (const int)y, col_r, col_g, col_b, 255);
                }
            }
        }
    }
}