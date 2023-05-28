#ifndef __FS_H__
#define __FS_H__

#include "cglm/cglm.h"

static inline void FRAGMENT_SHADER(void *interpolated_data,
                                   void *input_from_vertex_shader,
                                   vec4  out_frag_colour)
{
    interpolated_data        = NULL;
    input_from_vertex_shader = NULL;

    out_frag_colour[0] = 1.0f;
    out_frag_colour[1] = 0.0f;
    out_frag_colour[2] = 0.0f;
    out_frag_colour[3] = 1.0f;
}

#endif // __FS_H__