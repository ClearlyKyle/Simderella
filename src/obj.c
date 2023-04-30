#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "obj.h"

static void loadFile(void *ctx, const char *filename, const int is_mtl, const char *obj_filename, char **buffer, size_t *len)
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

static const char *tinyobj_parse_error_str[] =
    {
        "TINYOBJ_SUCCESS",
        "TINYOBJ_ERROR_EMPTY",
        "TINYOBJ_ERROR_INVALID_PARAMETER",
        "TINYOBJ_ERROR_FILE_OPERATION"};

static inline const char *tinyobj_parse_error(const int return_value_from_tinyobj)
{
    return tinyobj_parse_error_str[return_value_from_tinyobj * -1];

    // switch (return_value_from_tinyobj)
    //{
    // case 0:
    //     return tinyobj_parse_error_str[0];
    // case -1:
    //     return tinyobj_parse_error_str[1];
    // case -2:
    //     return tinyobj_parse_error_str[2];
    // case -3:
    //     return tinyobj_parse_error_str[3];
    // default:
    //     return "Error type not found";
    // }
}

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
        fprintf(stderr, "Failed to parse OBJ file : %s : %s\n", tinyobj_parse_error(ret), file_name);
        exit(1);
    }

    assert(shapes);

    if (materials == NULL)
    {
        printf("Object has no materials : %s\n", file_name);
    }
    else
    {
        printf("Material : %s\n", materials->name);
        // float ambient[3];
        // float diffuse[3];
        // float specular[3];
        // float transmittance[3];
        // float emission[3];
        // float shininess;
        // float ior;
        // float dissolve;
        // int   illum;

        printf("\t map_Ka   ambient_texname            :%s\n", materials->ambient_texname);
        printf("\t map_Kd   diffuse_texname            :%s\n", materials->diffuse_texname);
        printf("\t map_Ks   specular_texname           :%s\n", materials->specular_texname);
        printf("\t map_Ns   specular_highlight_texname :%s\n", materials->specular_highlight_texname);
        printf("\t map_bump bump_texname               :%s\n", materials->bump_texname);
        printf("\t disp     displacement_texname       :%s\n", materials->displacement_texname);
        printf("\t map_d    alpha_texname              :%s\n", materials->alpha_texname);
    }

    /*
    What are "faces"?
    - In the .obj file there are lines starting with f these are the faces. The format is:
        vertex_index/texture_index/normal_index
    for which each index starts at 1 and increases corresponding to the order in which the referenced element was defined
    the index is then used to get the values from "attributes.vertices[vertex_index]" for example
    each "face" is a group of "vertex_index/texture_index/normal_index", 3 faces make a triangle

    face_num_verts      - is an array of integers each element corresponds to the number of vertices in a face.
                            For example, if face_num_verts[i] is equal to 3, then the i-th face is a triangle.
    num_face_num_verts  - is an integer that specifies the number of faces in the mesh.
                            This value can be used to determine the size of the face_num_verts array.

    */
    { // Print Shape data
        printf("Shapes...\n");
        for (size_t i = 0; i < number_of_shapes; i++)
        {
            printf("[%zd]  Shape name         : %s\n", i, shapes[i].name);     // o crate_Cube.004
            printf("       Shape face_offset  : %d\n", shapes[i].face_offset); // offset into tinyobj_attrib_t->faces (starting f value, index)
            printf("       Shape length       : %d\n", shapes[i].length);      // number of faces for the given shape (f values in .obj)
        }
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

triang *Make_Triangles(struct Mesh *m)
{
    tinyobj_attrib_t attrib        = m->attribute;
    const size_t     num_triangles = attrib.num_face_num_verts;

    /* Allocate array for triangles */

    triang *triangles = malloc(sizeof(triang) * num_triangles);

#ifdef OBJ_CENTER
    vec3 bmin, bmax;
    bmin[0] = bmin[1] = bmin[2] = FLT_MAX;
    bmax[0] = bmax[1] = bmax[2] = -FLT_MAX;
#endif

    // Lets group verts into triangles
    for (size_t i = 0; i < num_triangles; i++)
    {
        assert(attrib.face_num_verts[i] % 3 == 0); /* assume all triangle faces. */

        tinyobj_vertex_index_t idx0 = attrib.faces[3 * i + 0];
        tinyobj_vertex_index_t idx1 = attrib.faces[3 * i + 1];
        tinyobj_vertex_index_t idx2 = attrib.faces[3 * i + 2];

        /* Get vertex data */
        triangles[i].v0[0] = attrib.vertices[idx0.v_idx * 3 + 0]; // X
        triangles[i].v0[1] = attrib.vertices[idx0.v_idx * 3 + 1]; // Y
        triangles[i].v0[2] = attrib.vertices[idx0.v_idx * 3 + 2]; // Z

        triangles[i].v1[0] = attrib.vertices[idx1.v_idx * 3 + 0]; // X
        triangles[i].v1[1] = attrib.vertices[idx1.v_idx * 3 + 1]; // Y
        triangles[i].v1[2] = attrib.vertices[idx1.v_idx * 3 + 2]; // Z

        triangles[i].v2[0] = attrib.vertices[idx2.v_idx * 3 + 0]; // X
        triangles[i].v2[1] = attrib.vertices[idx2.v_idx * 3 + 1]; // Y
        triangles[i].v2[2] = attrib.vertices[idx2.v_idx * 3 + 2]; // Z

        /* Get normal data */
        triangles[i].n0[0] = attrib.normals[idx0.vn_idx * 3 + 0]; // X
        triangles[i].n0[1] = attrib.normals[idx0.vn_idx * 3 + 1]; // Y
        triangles[i].n0[2] = attrib.normals[idx0.vn_idx * 3 + 2]; // Z

        triangles[i].n1[0] = attrib.normals[idx1.vn_idx * 3 + 0]; // X
        triangles[i].n1[1] = attrib.normals[idx1.vn_idx * 3 + 1]; // Y
        triangles[i].n1[2] = attrib.normals[idx1.vn_idx * 3 + 2]; // Z

        triangles[i].n2[0] = attrib.normals[idx2.vn_idx * 3 + 0]; // X
        triangles[i].n2[1] = attrib.normals[idx2.vn_idx * 3 + 1]; // Y
        triangles[i].n2[2] = attrib.normals[idx2.vn_idx * 3 + 2]; // Z

        /* Get texture data */
        triangles[i].u[0] = attrib.texcoords[idx0.vt_idx * 2 + 0];
        triangles[i].u[1] = attrib.texcoords[idx1.vt_idx * 2 + 0];
        triangles[i].u[2] = attrib.texcoords[idx2.vt_idx * 2 + 0];

        triangles[i].v[0] = attrib.texcoords[idx0.vt_idx * 2 + 1];
        triangles[i].v[1] = attrib.texcoords[idx1.vt_idx * 2 + 1];
        triangles[i].v[2] = attrib.texcoords[idx2.vt_idx * 2 + 1];

#if 0 /* Fund the bounding volume*/
        for (size_t coor = 0; coor < 3 coor++)
        {
            /* Find the min of x, y, z */
            bmin[coor] = (triangles[tri_index].v0[coor] < bmin[coor]) ? triangles[tri_index].v0[coor] : bmin[coor];
            bmin[coor] = (triangles[tri_index].v1[coor] < bmin[coor]) ? triangles[tri_index].v1[coor] : bmin[coor];
            bmin[coor] = (triangles[tri_index].v2[coor] < bmin[coor]) ? triangles[tri_index].v2[coor] : bmin[coor];

            /* Find the max of x, y, z */
            bmax[coor] = (triangles[tri_index].v0[coor] > bmax[coor]) ? triangles[tri_index].v0[coor] : bmax[coor];
            bmax[coor] = (triangles[tri_index].v1[coor] > bmax[coor]) ? triangles[tri_index].v1[coor] : bmax[coor];
            bmax[coor] = (triangles[tri_index].v2[coor] > bmax[coor]) ? triangles[tri_index].v2[coor] : bmax[coor];
        }
    }
    printf("bmin (%f, %f, %f)\n", bmin[0], bmin[1], bmin[2]);
    printf("bmax (%f, %f, %f)\n", bmax[0], bmax[1], bmax[2]);

    float obj_height = bmax[1] - bmin[1]; // calculate the height of the bounding box
    float obj_depth  = bmax[2] - bmin[2]; // calculate the depth of the bounding box

    printf("Object height: %f\n", obj_height);
    printf("Object depth : %f\n", obj_depth);

    /* Fit to 0, 1 with respect to object height*/
    float scale_to_height = obj_height;
    vec3  scale           = {1.0f / scale_to_height, 1.0f / scale_to_height, 1.0f / scale_to_height};

    // Calculate distance to bounding box
    // screen_height_amount is the percent of the screen the height of the object will fill
    float screen_height_amount = 1.0f;
    float FOVv                 = FOVh * (float)M_PI / 180.0f; // vertical FOV in radians
    float push_amount          = obj_height / (screen_height_amount * (2.0f * tanf(FOVv / 2.0f)));
    push_amount += (obj_depth / 2.0f);

    printf("Push amount: %f\n", push_amount);

    /* Centerize object. */
    vec3 position = {-0.5f * (bmax[0] + bmin[0]), // Center on the x axis
                     -0.5f * (bmax[1] + bmin[1]), // Center on the y axis
                     -push_amount};

    mat4 model;
    glm_scale_make(model, scale);
    //  glm_translated(model, position);
    glm_translate(model, position);
#endif
    }

    return triangles;
}

void free_object(struct Mesh *m)
{
    tinyobj_attrib_free(&m->attribute);

    if (m->shapes)
    {
        tinyobj_shapes_free(m->shapes, m->number_of_shapes);
        m->shapes           = NULL;
        m->number_of_shapes = 0;
    }

    if (m->materials)
    {
        tinyobj_materials_free(m->materials, m->number_of_materials);
        m->materials           = NULL;
        m->number_of_materials = 0;
    }
}