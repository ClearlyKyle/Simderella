#ifndef __OBJ_H__
#define __OBJ_H__

#include <assert.h>

#include "cglm/cglm.h"
#include "tex.h"

#include "tinyobj_loader_c.h"

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

/*
Here we are converting the data from tinyObj into glm vec3
instead of
    verticies = float float float float...
it will be
    verticies = (vec3){} (vec3){} (vec3){}
*/
typedef struct
{
    /* vertices */
    vec3 *vertex;

    /* normals */
    vec3 *normals;

    /* texture coordinates */
    vec3 *u; /* u0, u1, u2*/
    vec3 *v; /* v0, v1, v2*/
} index_triang;

struct Mesh
{
    tinyobj_attrib_t    attribute;
    tinyobj_shape_t    *shapes;
    size_t              number_of_shapes;
    tinyobj_material_t *materials;
    size_t              number_of_materials;

    unsigned int number_of_triangles;
    triang      *triangle;

    texture_t *ambient_tex;            // map_Ka   ambient_tex
    texture_t *diffuse_tex;            // map_Kd   diffuse_tex
    texture_t *specular_tex;           // map_Ks   specular_tex
    texture_t *specular_highlight_tex; // map_Ns   specular_highlight_tex
    texture_t *bump_tex;               // map_bump bump_tex
    texture_t *displacement_tex;       // disp     displacement_tex
    texture_t *alpha_tex;              // map_d    alpha_tex
};

struct Mesh Mesh_Load(const char *file_name);
void        Mesh_Destroy(struct Mesh *m);

#endif // __OBJ_H__