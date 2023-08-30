#include "renderer.h"
#include "utils/utils.h"
#include "graphics.h"

#include "job_system/js.h"

typedef struct TriangleRasterData
{
    size_t stride;
    size_t number_of_triangles;
} TriangleRasterData_t;

static inline void Inpterpolate_Attribute(VaryingAttributes_t *varying, InterpolatedPixel_t *res, const __m128 W_vals[3], const __m128 w0, const __m128 w1, const __m128 w2, const __m128 interFactor)
{
    for (size_t i = 0; i < NUMBER_OF_VARYING_VE4_ATTRIBUTES; i++)
    {
        // TODO : Better naming here plz
        __m128 X[3];
        X[0] = _mm_set1_ps(varying[0].vec4_attribute[i].vec.m128_f32[0]);
        X[1] = _mm_set1_ps(varying[1].vec4_attribute[i].vec.m128_f32[0]);
        X[2] = _mm_set1_ps(varying[2].vec4_attribute[i].vec.m128_f32[0]);

        __m128 Y[3];
        Y[0] = _mm_set1_ps(varying[0].vec4_attribute[i].vec.m128_f32[1]);
        Y[1] = _mm_set1_ps(varying[1].vec4_attribute[i].vec.m128_f32[1]);
        Y[2] = _mm_set1_ps(varying[2].vec4_attribute[i].vec.m128_f32[1]);

        __m128 Z[3];
        Z[0] = _mm_set1_ps(varying[0].vec4_attribute[i].vec.m128_f32[2]);
        Z[1] = _mm_set1_ps(varying[1].vec4_attribute[i].vec.m128_f32[2]);
        Z[2] = _mm_set1_ps(varying[2].vec4_attribute[i].vec.m128_f32[2]);

        __m128 W[3];
        W[0] = _mm_set1_ps(varying[0].vec4_attribute[i].vec.m128_f32[3]);
        W[1] = _mm_set1_ps(varying[1].vec4_attribute[i].vec.m128_f32[3]);
        W[2] = _mm_set1_ps(varying[2].vec4_attribute[i].vec.m128_f32[3]);

        res->vec4_attribute[i].mX = _mm_add_ps(_mm_add_ps(_mm_mul_ps(X[0], w0), _mm_mul_ps(X[1], w1)), _mm_mul_ps(X[2], w2));
        res->vec4_attribute[i].mY = _mm_add_ps(_mm_add_ps(_mm_mul_ps(Y[0], w0), _mm_mul_ps(Y[1], w1)), _mm_mul_ps(Y[2], w2));
        res->vec4_attribute[i].mZ = _mm_add_ps(_mm_add_ps(_mm_mul_ps(Z[0], w0), _mm_mul_ps(Z[1], w1)), _mm_mul_ps(Z[2], w2));
        res->vec4_attribute[i].mY = _mm_add_ps(_mm_add_ps(_mm_mul_ps(Y[0], w0), _mm_mul_ps(Y[1], w1)), _mm_mul_ps(Y[2], w2));

        res->vec4_attribute[i].mX = _mm_mul_ps(interFactor, res->vec4_attribute[i].mX);
        res->vec4_attribute[i].mY = _mm_mul_ps(interFactor, res->vec4_attribute[i].mY);
        res->vec4_attribute[i].mZ = _mm_mul_ps(interFactor, res->vec4_attribute[i].mZ);
        res->vec4_attribute[i].mY = _mm_mul_ps(interFactor, res->vec4_attribute[i].mY);
    }

    for (size_t i = 0; i < NUMBER_OF_VARYING_VE3_ATTRIBUTES; i++)
    {
        // NOTE: Could we transpose this?
        __m128 X[3];
        X[0] = _mm_set1_ps(varying[0].vec3_attribute[i].vec.m128_f32[0]);
        X[1] = _mm_set1_ps(varying[1].vec3_attribute[i].vec.m128_f32[0]);
        X[2] = _mm_set1_ps(varying[2].vec3_attribute[i].vec.m128_f32[0]);

        __m128 Y[3];
        Y[0] = _mm_set1_ps(varying[0].vec3_attribute[i].vec.m128_f32[1]);
        Y[1] = _mm_set1_ps(varying[1].vec3_attribute[i].vec.m128_f32[1]);
        Y[2] = _mm_set1_ps(varying[2].vec3_attribute[i].vec.m128_f32[1]);

        __m128 Z[3];
        Z[0] = _mm_set1_ps(varying[0].vec3_attribute[i].vec.m128_f32[2]);
        Z[1] = _mm_set1_ps(varying[1].vec3_attribute[i].vec.m128_f32[2]);
        Z[2] = _mm_set1_ps(varying[2].vec3_attribute[i].vec.m128_f32[2]);

        res->vec3_attribute[i].mX = _mm_add_ps(_mm_add_ps(_mm_mul_ps(X[0], w0), _mm_mul_ps(X[1], w1)), _mm_mul_ps(X[2], w2));
        res->vec3_attribute[i].mY = _mm_add_ps(_mm_add_ps(_mm_mul_ps(Y[0], w0), _mm_mul_ps(Y[1], w1)), _mm_mul_ps(Y[2], w2));
        res->vec3_attribute[i].mZ = _mm_add_ps(_mm_add_ps(_mm_mul_ps(Z[0], w0), _mm_mul_ps(Z[1], w1)), _mm_mul_ps(Z[2], w2));

        res->vec3_attribute[i].mX = _mm_mul_ps(interFactor, res->vec3_attribute[i].mX);
        res->vec3_attribute[i].mY = _mm_mul_ps(interFactor, res->vec3_attribute[i].mY);
        res->vec3_attribute[i].mZ = _mm_mul_ps(interFactor, res->vec3_attribute[i].mZ);
    }

    for (size_t i = 0; i < NUMBER_OF_VARYING_VE2_ATTRIBUTES; i++)
    {
        __m128 U[3];
        U[0] = _mm_set1_ps(varying[0].vec2_attribute[i].vec.m128_f32[0]);
        U[1] = _mm_set1_ps(varying[1].vec2_attribute[i].vec.m128_f32[0]);
        U[2] = _mm_set1_ps(varying[2].vec2_attribute[i].vec.m128_f32[0]);

        __m128 V[3];
        V[0] = _mm_set1_ps(varying[0].vec2_attribute[i].vec.m128_f32[1]);
        V[1] = _mm_set1_ps(varying[1].vec2_attribute[i].vec.m128_f32[1]);
        V[2] = _mm_set1_ps(varying[2].vec2_attribute[i].vec.m128_f32[1]);

        U[0] = _mm_mul_ps(U[0], W_vals[0]);
        U[1] = _mm_mul_ps(U[1], W_vals[1]);
        U[2] = _mm_mul_ps(U[2], W_vals[2]);

        V[0] = _mm_mul_ps(V[0], W_vals[0]);
        V[1] = _mm_mul_ps(V[1], W_vals[1]);
        V[2] = _mm_mul_ps(V[2], W_vals[2]);

        res->vec2_attribute[i].mX = _mm_add_ps(_mm_add_ps(_mm_mul_ps(U[0], w0), _mm_mul_ps(U[1], w1)), _mm_mul_ps(U[2], w2));
        res->vec2_attribute[i].mY = _mm_add_ps(_mm_add_ps(_mm_mul_ps(V[0], w0), _mm_mul_ps(V[1], w1)), _mm_mul_ps(V[2], w2));

        res->vec2_attribute[i].mX = _mm_mul_ps(interFactor, res->vec2_attribute[i].mX);
        res->vec2_attribute[i].mY = _mm_mul_ps(interFactor, res->vec2_attribute[i].mY);
    }
}

static void Raster_Triangles(void *data)
{
    TriangleRasterData_t *rd = (TriangleRasterData_t *)data;

    // const size_t max_index              = rd->number_of_triangles;
    size_t current_triangle_index = (Platform_InterlockedIncrement((int32_t *)&rd->stride) - 1) * 4;

    const __m128i x_pixel_offset = _mm_setr_epi32(0, 1, 2, 3); // X value offsets
    const __m128i y_pixel_offset = _mm_setr_epi32(0, 0, 0, 0); // Y value offsets

    /* 4 Triangles, with 3 vertices */
    __m128       collected_vertices[4][3] = {0};
    RasterData_t collected_raster_data[4] = {0};

    ASSERT(Trianges_To_Be_Rastered_Counter != 0);

    /* Gather 4 triangles */
    size_t number_of_collected_triangles = 0;
    for (size_t i = 0; i < 4; i++)
    {
        if (current_triangle_index > Trianges_To_Be_Rastered_Counter)
            break;

        CHECK_ARRAY_BOUNDS(current_triangle_index, MAX_NUMBER_OF_TRIANGLES_TO_RASTER);
        collected_raster_data[i] = Trianges_To_Be_Rastered[current_triangle_index++];

        collected_vertices[i][0] = collected_raster_data[i].ss_v0;
        collected_vertices[i][1] = collected_raster_data[i].ss_v1;
        collected_vertices[i][2] = collected_raster_data[i].ss_v2;

        ++number_of_collected_triangles;
    }

    /* 4 triangles, 3 vertices, 4 * 3 = 12 'x' values, we can store all this in  X_values[3]*/
    __m128 Xf_values[3], Yf_values[3];
    __m128 Z_values[3], W_values[3];
    for (uint8_t i = 0; i < 3; i++)
    {
        /* Get 4 verticies at once */
        __m128 tri0_vert_i = collected_vertices[0][i]; // Get vertex i from triangle 0
        __m128 tri1_vert_i = collected_vertices[1][i]; // Get vertex i from triangle 1
        __m128 tri2_vert_i = collected_vertices[2][i]; // Get vertex i from triangle 2
        __m128 tri3_vert_i = collected_vertices[3][i]; // Get vertex i from triangle 3

        // transpose into SoA layout
        // X, X, X, X and Y, Y, Y, Y
        _MM_TRANSPOSE4_PS(tri0_vert_i, tri1_vert_i, tri2_vert_i, tri3_vert_i);
        Xf_values[i] = _mm_add_ps(_mm_set1_ps(0.5f), tri0_vert_i);
        Yf_values[i] = _mm_add_ps(_mm_set1_ps(0.5f), tri1_vert_i);
        Z_values[i]  = tri2_vert_i;
        W_values[i]  = tri3_vert_i;
    }

    // Use bounding box traversal strategy to determine which pixels to rasterize
    const __m128i startX = _mm_cvtps_epi32(_mm_max_ps(_mm_min_ps(_mm_min_ps(Xf_values[0], Xf_values[1]), Xf_values[2]), _mm_set1_ps(0.0f)));
    const __m128i endX   = _mm_cvtps_epi32(_mm_min_ps(_mm_max_ps(_mm_max_ps(Xf_values[0], Xf_values[1]), Xf_values[2]), _mm_set1_ps(IMAGE_W)));

    const __m128i startY = _mm_cvtps_epi32(_mm_max_ps(_mm_min_ps(_mm_min_ps(Yf_values[0], Yf_values[1]), Yf_values[2]), _mm_set1_ps(0.0f)));
    const __m128i endY   = _mm_cvtps_epi32(_mm_min_ps(_mm_max_ps(_mm_max_ps(Yf_values[0], Yf_values[1]), Yf_values[2]), _mm_set1_ps(IMAGE_H)));

    // We switch to using ints for rasterizing
    __m128i X_values[3], Y_values[3];
    for (size_t i = 0; i < 3; i++)
    {
        X_values[i] = _mm_cvtps_epi32(Xf_values[i]);
        Y_values[i] = _mm_cvtps_epi32(Yf_values[i]);
    }

    // Counter clockwise triangles
    const __m128i A0 = _mm_sub_epi32(Y_values[2], Y_values[1]); // 0 - 1
    const __m128i A1 = _mm_sub_epi32(Y_values[0], Y_values[2]); // 1 - 2
    const __m128i A2 = _mm_sub_epi32(Y_values[1], Y_values[0]); // 2 - 0

    const __m128i B0 = _mm_sub_epi32(X_values[1], X_values[2]); // 1 - 0
    const __m128i B1 = _mm_sub_epi32(X_values[2], X_values[0]); // 2 - 1
    const __m128i B2 = _mm_sub_epi32(X_values[0], X_values[1]); // 0 - 2

    // Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
    const __m128i C0 = _mm_sub_epi32(_mm_mullo_epi32(X_values[2], Y_values[1]), _mm_mullo_epi32(X_values[1], Y_values[2]));
    const __m128i C1 = _mm_sub_epi32(_mm_mullo_epi32(X_values[0], Y_values[2]), _mm_mullo_epi32(X_values[2], Y_values[0]));
    const __m128i C2 = _mm_sub_epi32(_mm_mullo_epi32(X_values[1], Y_values[0]), _mm_mullo_epi32(X_values[0], Y_values[1]));

    // Compute inverse triangle area
    const __m128i triArea = _mm_sub_epi32(
        _mm_mullo_epi32(B1, A2),
        _mm_mullo_epi32(B2, A1));
    // const __m128 oneOverTriArea = _mm_rcp_ps(_mm_cvtepi32_ps(triArea));
    const __m128 oneOverTriArea = _mm_div_ps(_mm_set1_ps(1.0f), _mm_cvtepi32_ps(triArea));

    // const __m128 oneOverTriArea = _mm_setr_ps(collected_raster_data[0].area, collected_raster_data[1].area, collected_raster_data[2].area, collected_raster_data[3].area);

    Z_values[1] = _mm_mul_ps(_mm_sub_ps(Z_values[1], Z_values[0]), oneOverTriArea);
    Z_values[2] = _mm_mul_ps(_mm_sub_ps(Z_values[2], Z_values[0]), oneOverTriArea);

    /* lane is the counter for how many triangles were loaded, if only 3 were loaded, it
        should only be 3, etc...
    */
    for (int lane = 0; lane < number_of_collected_triangles; lane++) // Now we have 4 triangles set up.  Rasterize them each individually.
    {
        const float area_value = oneOverTriArea.m128_f32[lane];
        if (area_value < 0.0f)
            continue;

        const __m128 inv_area = _mm_set1_ps(area_value);

        const int startXx = startX.m128i_i32[lane];
        const int endXx   = endX.m128i_i32[lane];
        const int startYy = startY.m128i_i32[lane];
        const int endYy   = endY.m128i_i32[lane];

        ASSERT(startXx >= 0 && startXx < IMAGE_W);
        ASSERT(endXx >= 0 && endXx < IMAGE_W);

        ASSERT(startYy >= 0 && startYy < IMAGE_H);
        ASSERT(endYy >= 0 && endYy < IMAGE_H);

        __m128 Z[3];
        Z[0] = _mm_set1_ps(Z_values[0].m128_f32[lane]);
        Z[1] = _mm_set1_ps(Z_values[1].m128_f32[lane]);
        Z[2] = _mm_set1_ps(Z_values[2].m128_f32[lane]);

        __m128 W[3];
        W[0] = _mm_set1_ps(W_values[0].m128_f32[lane]);
        W[1] = _mm_set1_ps(W_values[1].m128_f32[lane]);
        W[2] = _mm_set1_ps(W_values[2].m128_f32[lane]);

        const __m128i a0 = _mm_set1_epi32(A0.m128i_i32[lane]);
        const __m128i a1 = _mm_set1_epi32(A1.m128i_i32[lane]);
        const __m128i a2 = _mm_set1_epi32(A2.m128i_i32[lane]);

        const __m128i b0 = _mm_set1_epi32(B0.m128i_i32[lane]);
        const __m128i b1 = _mm_set1_epi32(B1.m128i_i32[lane]);
        const __m128i b2 = _mm_set1_epi32(B2.m128i_i32[lane]);

        // Add our SIMD pixel offset to our starting pixel location, so we are doing 4 pixels in the x axis
        // so we add 0, 1, 2, 3, to the starting x value, y isnt changing
        const __m128i col = _mm_add_epi32(x_pixel_offset, _mm_set1_epi32(startXx));
        const __m128i row = _mm_add_epi32(y_pixel_offset, _mm_set1_epi32(startYy));

        const __m128i A0_start = _mm_mullo_epi32(a0, col);
        const __m128i A1_start = _mm_mullo_epi32(a1, col);
        const __m128i A2_start = _mm_mullo_epi32(a2, col);

        /* Step in the y direction */
        // First we must compute E at out starting pixel, this will be the minX and minY of
        // our boudning box of the traingle
        const __m128i B0_start = _mm_mullo_epi32(b0, row);
        const __m128i B1_start = _mm_mullo_epi32(b1, row);
        const __m128i B2_start = _mm_mullo_epi32(b2, row);

        // Barycentric Setip
        // Order of triangle sides *IMPORTANT*
        // E(x, y) = a*x + b*y + c;
        // v1, v2 :  w0_row = (A12 * p.x) + (B12 * p.y) + C12;
        __m128i E0 = _mm_add_epi32(_mm_add_epi32(A0_start, B0_start), _mm_set1_epi32(C0.m128i_i32[lane]));
        __m128i E1 = _mm_add_epi32(_mm_add_epi32(A1_start, B1_start), _mm_set1_epi32(C1.m128i_i32[lane]));
        __m128i E2 = _mm_add_epi32(_mm_add_epi32(A2_start, B2_start), _mm_set1_epi32(C2.m128i_i32[lane]));

        // Since we are doing SIMD, we need to calcaulte our step amount
        // E(x+L, y) = E(x) + L dy (where dy is out a0 values)
        // B0_inc controls the step amount in the Y axis, since we are only moving 1px at a time in the y axis
        // we dont need to change the step amount
        const __m128i B0_inc = b0;
        const __m128i B1_inc = b1;
        const __m128i B2_inc = b2;

        // A0_inc controls the step amount in the X axis, we are doing 4px at a time so multiply our dY by 4
        const __m128i A0_inc = _mm_slli_epi32(a0, 2); // a0 * 4
        const __m128i A1_inc = _mm_slli_epi32(a1, 2);
        const __m128i A2_inc = _mm_slli_epi32(a2, 2);

        // Generate masks used for tie-breaking rules (not to double-shade along shared edges)
        // there is no _mm_cmpge_epi32, so use lt and swap operands
        // _mm_cmplt_epi32(bb0Inc, _mm_setzero_si128()) - becomes - _mm_cmplt_epi32(_mm_setzero_si128(), bb0Inc)
        const __m128i Edge0TieBreak = _mm_or_epi32(_mm_cmpgt_epi32(A0_inc, _mm_setzero_si128()),
                                                   _mm_and_epi32(_mm_cmplt_epi32(_mm_setzero_si128(), B0_inc), _mm_cmpeq_epi32(A0_inc, _mm_setzero_si128())));

        const __m128i Edge1TieBreak = _mm_or_epi32(_mm_cmpgt_epi32(A1_inc, _mm_setzero_si128()),
                                                   _mm_and_epi32(_mm_cmplt_epi32(_mm_setzero_si128(), B1_inc), _mm_cmpeq_epi32(A1_inc, _mm_setzero_si128())));

        const __m128i Edge2TieBreak = _mm_or_epi32(_mm_cmpgt_epi32(A2_inc, _mm_setzero_si128()),
                                                   _mm_and_epi32(_mm_cmplt_epi32(_mm_setzero_si128(), B2_inc), _mm_cmpeq_epi32(A2_inc, _mm_setzero_si128())));

        __m128 Zstep = _mm_mul_ps(_mm_cvtepi32_ps(A1_inc), Z[1]);
        Zstep        = _mm_add_ps(Zstep, _mm_mul_ps(_mm_cvtepi32_ps(A2_inc), Z[2]));

        // Incrementally compute Fab(x, y) for all the pixels inside the bounding box formed by (startX, endX) and (startY, endY)
        for (size_t pix_y = startYy; pix_y <= endYy; ++pix_y,
                    E0    = _mm_add_epi32(E0, B0_inc),
                    E1    = _mm_add_epi32(E1, B1_inc),
                    E2    = _mm_add_epi32(E2, B2_inc))
        {
            // Compute barycentric coordinates
            __m128i alpha = E0;
            __m128i beta  = E1;
            __m128i gama  = E2;

            __m128 depth = Z[0];
            depth        = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(beta), Z[1]));
            depth        = _mm_add_ps(depth, _mm_mul_ps(_mm_cvtepi32_ps(gama), Z[2]));

            for (size_t pix_x = startXx; pix_x <= endXx; pix_x += 4,
                        alpha = _mm_add_epi32(alpha, A0_inc),
                        beta  = _mm_add_epi32(beta, A1_inc),
                        gama  = _mm_add_epi32(gama, A2_inc),
                        depth = _mm_add_ps(depth, Zstep))
            {
                // Test Pixel inside triangle
                const __m128i Edge0Positive = _mm_cmpgt_epi32(alpha, _mm_setzero_si128());
                const __m128i Edge0Negative = _mm_cmplt_epi32(alpha, _mm_setzero_si128());
                const __m128i Edge0FuncMask = _mm_or_epi32(Edge0Positive,
                                                           _mm_andnot_epi32(Edge0Negative, Edge0TieBreak));

                // Edge 1 test
                const __m128i Edge1Positive = _mm_cmpgt_epi32(beta, _mm_setzero_si128());
                const __m128i Edge1Negative = _mm_cmplt_epi32(beta, _mm_setzero_si128());
                const __m128i Edge1FuncMask = _mm_or_epi32(Edge1Positive,
                                                           _mm_andnot_epi32(Edge1Negative, Edge1TieBreak));

                // Edge 2 test
                const __m128i Edge2Positive = _mm_cmpgt_epi32(gama, _mm_setzero_si128());
                const __m128i Edge2Negative = _mm_cmplt_epi32(gama, _mm_setzero_si128());
                const __m128i Edge2FuncMask = _mm_or_epi32(Edge2Positive,
                                                           _mm_andnot_epi32(Edge2Negative, Edge2TieBreak));

                // Combine resulting masks of all three edges
                __m128i mask = _mm_and_epi32(Edge0FuncMask, _mm_and_epi32(Edge1FuncMask, Edge2FuncMask));

                /* Check if pixel is inside the triangle */
                // const __m128i or_mask = _mm_or_si128(_mm_or_si128(alpha, beta), gama);
                //__m128i       mask    = _mm_cmpgt_epi32(or_mask, _mm_setzero_si128());

#if 0
                const uint16_t maskInt = (uint16_t)_mm_movemask_ps(_mm_cvtepi32_ps(mask));
                if (maskInt == 0x0)
                    continue;
#else
                if (_mm_test_all_zeros(mask, mask))
                    continue;
#endif

                const size_t index        = pix_y * IMAGE_W + pix_x;
                float *const pDepthBuffer = &RenderState.depth_buffer[index];

                const __m128 previousDepthValue = _mm_loadu_ps(pDepthBuffer);
                const __m128 sseDepthRes        = _mm_cmplt_ps(depth, previousDepthValue);

                if ((uint16_t)_mm_movemask_ps(sseDepthRes) == 0x0)
                    continue;

                const __m128 sseWriteMask = _mm_and_ps(sseDepthRes, _mm_castsi128_ps(mask));

                const __m128 finaldepth = _mm_blendv_ps(previousDepthValue, depth, sseWriteMask);
                _mm_store_ps(pDepthBuffer, finaldepth);

                mask = _mm_abs_epi32(mask);

                /* Barycentric Weights */
                const __m128 w0 = _mm_mul_ps(_mm_cvtepi32_ps(alpha), inv_area);
                const __m128 w1 = _mm_mul_ps(_mm_cvtepi32_ps(beta), inv_area);
                const __m128 w2 = _mm_mul_ps(_mm_cvtepi32_ps(gama), inv_area);

                __m128 intrFactor = _mm_add_ps(_mm_add_ps(_mm_mul_ps(W[0], w0), _mm_mul_ps(W[1], w1)), _mm_mul_ps(W[2], w2));
                intrFactor        = _mm_rcp_ps(intrFactor);
                intrFactor        = _mm_mul_ps(intrFactor, _mm_cvtepi32_ps(mask));

                InterpolatedPixel_t res;
                Inpterpolate_Attribute(collected_raster_data[lane].varying, &res, W, w0, w1, w2, intrFactor);

                uint8_t frag_colour0[4] = {0};
                uint8_t frag_colour1[4] = {0};
                uint8_t frag_colour2[4] = {0};
                uint8_t frag_colour3[4] = {0};

                if (mask.m128i_i32[0])
                    FRAGMENT_SHADER(&res, 0, &RenderState.data_from_vertex_shader, frag_colour0);
                if (mask.m128i_i32[1])
                    FRAGMENT_SHADER(&res, 1, &RenderState.data_from_vertex_shader, frag_colour1);
                if (mask.m128i_i32[2])
                    FRAGMENT_SHADER(&res, 2, &RenderState.data_from_vertex_shader, frag_colour2);
                if (mask.m128i_i32[3])
                    FRAGMENT_SHADER(&res, 3, &RenderState.data_from_vertex_shader, frag_colour3);

                // Dont even ask about this pixel format
                const __m128i combined_colours = _mm_set_epi8(frag_colour3[3], frag_colour3[0], frag_colour3[1], frag_colour3[2],
                                                              frag_colour2[3], frag_colour2[0], frag_colour2[1], frag_colour2[2],
                                                              frag_colour1[3], frag_colour1[0], frag_colour1[1], frag_colour1[2],
                                                              frag_colour0[3], frag_colour0[0], frag_colour0[1], frag_colour0[2]);

                uint8_t *const pixel_location = &RenderState.colour_buffer[index * IMAGE_BPP];

#if 1 /* Fabian method */
                const __m128i original_pixel_data = _mm_loadu_si128((__m128i *)pixel_location);

                const __m128i write_mask    = _mm_castps_si128(sseWriteMask);
                const __m128i masked_output = _mm_or_si128(_mm_and_si128(write_mask, combined_colours),
                                                           _mm_andnot_si128(write_mask, original_pixel_data));

                _mm_storeu_si128((__m128i *)pixel_location, masked_output);
#else
                // Mask-store 4-sample fragment values
                _mm_maskstore_epi32(
                    (int *)pixel_location,
                    _mm_castps_si128(sseWriteMask),
                    combined_colours);
#endif
            }
        }
    }
}

void Raster_Triangles_MT(void)
{
    static TriangleRasterData_t rd = {0};
    rd.stride                      = 0;
    // rd.number_of_triangles         = Trianges_To_Be_Rastered_Counter;

    const size_t tmp = Trianges_To_Be_Rastered_Counter / 4;
    job_t        job = {Raster_Triangles, (void *)&rd};

    for (size_t i = 0; i < tmp; i++)
        job_submit(job);

    jobs_complete_all_work();
}