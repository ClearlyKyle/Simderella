#ifndef __SHADER_ATTRIBUTES_H__
#define __SHADER_ATTRIBUTES_H__

#include <math.h>

#include "tex.h"
#include "cglm/cglm.h"
#include "utils/utils.h"

typedef struct
{
    union
    {
        vec4   raw;
        __m128 vec;
    } vec4_attribute[NUMBER_OF_VARYING_VE4_ATTRIBUTES + 1];

    union
    {
        vec3   raw;
        float  pad[4];
        __m128 vec;
    } vec3_attribute[NUMBER_OF_VARYING_VE3_ATTRIBUTES + 1];

    union
    {
        vec2   raw;
        float  pad[4];
        __m128 vec;
    } vec2_attribute[NUMBER_OF_VARYING_VE2_ATTRIBUTES + 1];
} VaryingAttributes_t;

#pragma warning(push)
#pragma warning(disable : 4201)

// clang-format off
typedef struct InterpolatedPixel {
    struct Vec4Attributes
    {
        union { vec4 X; __m128 mX; }; 
        union { vec4 Y; __m128 mY; };
        union { vec4 Z; __m128 mZ; };
        union { vec4 W; __m128 mW; };
    } vec4_attribute[NUMBER_OF_VARYING_VE4_ATTRIBUTES + 1];

    struct Vec3Attributes
    {
        union { vec4 X; __m128 mX; };
        union { vec4 Y; __m128 mY; };
        union { vec4 Z; __m128 mZ; };
    } vec3_attribute[NUMBER_OF_VARYING_VE3_ATTRIBUTES + 1];

    struct Vec2Attributes
    {
        union { vec4 X; __m128 mX; };
        union { vec4 Y; __m128 mY; };
    } vec2_attribute[NUMBER_OF_VARYING_VE2_ATTRIBUTES + 1];
} InterpolatedPixel_t;

// clang-format on
#pragma warning(pop)

#endif // __SHADER_ATTRIBUTES_H__