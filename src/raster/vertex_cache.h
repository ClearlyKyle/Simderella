#ifndef __VERTEX_CACHE_H__
#define __VERTEX_CACHE_H__

#include "cglm/cglm.h"

#define VERTEX_CACHE_SIZE 16

typedef struct
{
    vec4 clip_values[VERTEX_CACHE_SIZE];
    int  index_values[VERTEX_CACHE_SIZE];
    int  front;
} VertCache_t;

#define VertCACHE_CREATE \
    (VertCache) { 0 }

static inline void VertCache_Add(VertCache_t *cache, int index, vec4 vert)
{
    glm_vec4_copy(vert, cache->clip_values[cache->front]);
    cache->index_values[cache->front] = index;
    cache->front                      = (cache->front + 1) % VERTEX_CACHE_SIZE;
}

static inline int VertCache_Lookup(VertCache_t *cache, int index, vec4 dest)
{
    for (int i = 0; i < VERTEX_CACHE_SIZE; i++)
    {
        if (cache->index_values[i] == index)
        {
            glm_vec4_copy(cache->clip_values[i], dest);
            return 1; // Return true
        }
    }
    return -1; // Return false
}

static inline void FIFOCache_Reset_Index_Values(VertCache_t *cache)
{
    *cache = (VertCache_t){0};
    memset(cache->index_values, -1, sizeof(int) * VERTEX_CACHE_SIZE);
}

#endif // __VERTEX_CACHE_H__