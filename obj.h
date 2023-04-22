#ifndef __OBJ_H__
#define __OBJ_H__

#include <assert.h>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

#include "cglm/cglm.h"

struct Mesh
{
    tinyobj_attrib_t    attribute;
    tinyobj_shape_t    *shapes;
    size_t              number_of_shapes;
    tinyobj_material_t *materials;
    size_t              number_of_materials;
};

static void
loadFile(void *ctx, const char *filename, const int is_mtl, const char *obj_filename, char **buffer, size_t *len)
{
    *buffer            = NULL;
    long   string_size = 0;
    size_t read_size   = 0;
    FILE  *handler     = NULL;
    fopen_s(&handler, filename, "r");

    if (handler)
    {
        fseek(handler, 0, SEEK_END);
        string_size = ftell(handler);
        rewind(handler);
        *buffer                = (char *)malloc(sizeof(char) * (string_size + 1));
        read_size              = fread(*buffer, sizeof(char), (size_t)string_size, handler);
        (*buffer)[string_size] = '\0';
        if (string_size != read_size)
        {
            free(buffer);
            *buffer = NULL;
        }
        fclose(handler);
    }

    *len = read_size;
}

static const char *tinyobj_error[] =
    {
        "TINYOBJ_SUCCESS",
        "TINYOBJ_ERROR_EMPTY",
        "TINYOBJ_ERROR_INVALID_PARAMETER",
        "TINYOBJ_ERROR_FILE_OPERATION"};

static inline const char *parse_error(const int return_values_from_tinyobj)
{
    return tinyobj_error[return_values_from_tinyobj * -1];

    // switch (return_values_from_tinyobj)
    //{
    // case -1:
    //     return tinyobj_error[1];
    // case -2:
    //     return tinyobj_error[1];
    // case -3:
    //     return tinyobj_error[1];
    // default:
    //     return "Error type not found";
    // }
}

// void InitializeSceneObjects(const char *fileName, std::vector<Mesh> &meshBuffer, std::vector<VertexInput> &vertexBuffer, std::vector<uint32_t> &indexBuffer, std::map<std::string, Texture *> &textures)
struct Mesh Initialise_Object(const char *file_name)
{
    assert(file_name);

    tinyobj_attrib_t attribute;

    tinyobj_shape_t *shapes;
    size_t           number_of_shapes;

    tinyobj_material_t *materials;
    size_t              number_of_materials;

    const int ret = tinyobj_parse_obj(&attribute, &shapes, &number_of_shapes, &materials, &number_of_materials, file_name, loadFile, NULL, TINYOBJ_FLAG_TRIANGULATE);
    if (ret != TINYOBJ_SUCCESS)
    {
        fprintf(stderr, "Failed to parse OBJ file : %s : %s\n", parse_error(ret), file_name);
        exit(1);
    }

    assert(shapes);

    if (materials == NULL)
    {
        printf("Object has no materials : %s\n", file_name);
    }

    {                                                             // Print Shape data
        printf("Shape name         : %s\n", shapes->name);        // o crate_Cube.004
        printf("Shape face_offset  : %d\n", shapes->face_offset); // offset into tinyobj_attrib_t->faces (starting f value, index)
        printf("Shape length       : %d\n", shapes->length);      // number of faces for the given shape (f values in .obj)
    }

    { // Print Attribute data
        printf("Attribute data...\n");
        printf("\tnum_vertices       : %d\n", attribute.num_vertices);       // Number of vertices in 'vertices' (the actual array length is num_vertices*3)
        printf("\tnum_normals        : %d\n", attribute.num_normals);        // Number of vertices in 'normals' (the actual array length is num_normals*3)
        printf("\tnum_texcoords      : %d\n", attribute.num_texcoords);      // Number of vertices in 'texcoords' (the actual array length is num_normals*2)
        printf("\tnum_faces          : %d\n", attribute.num_faces);          // Array of faces (containing tinyobj_vertex_index_t information)
        printf("\tnum_face_num_verts : %d\n", attribute.num_face_num_verts); // Total number of triangles in this object (length of face_num_verts)
    }

    struct Mesh mesh =
        {
            .attribute = attribute,

            .shapes           = shapes,
            .number_of_shapes = number_of_shapes,

            .materials           = materials,
            .number_of_materials = number_of_materials,

        };

    return mesh;

}

}

#endif // __OBJ_H__