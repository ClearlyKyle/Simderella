#ifndef __OBJ_H__
#define __OBJ_H__

#include <assert.h>

#include "cglm/cglm.h"
#include "tex.h"

#include "tinyobj_loader_c.h"

struct Mesh
{
    tinyobj_attrib_t    attribute;
    tinyobj_shape_t    *shapes;
    size_t              number_of_shapes;
    tinyobj_material_t *materials;
    size_t              number_of_materials;

    texture_t diffuse;
};

typedef struct
{
    /* vertices */
    vec3 v0;
    vec3 v1;
    vec3 v2;

    /* normals */
    vec3 n0;
    vec3 n1;
    vec3 n2;

    /* texture coordinates */
    vec3 u; /* u0, u1, u2*/
    vec3 v; /* v0, v1, v2*/
} triang;

struct Mesh Initialise_Object(const char *file_name);
triang     *Make_Triangles(struct Mesh *m);
void        free_object(struct Mesh *m);

#endif // __OBJ_H__