#include "tex.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

texture_t Texture_Load(const char *file_path, int bbp)
{
    texture_t t = {0};

    // textures oriented tha same as you view them in paint
    stbi_set_flip_vertically_on_load(1);

    // char buff[64]; /* add file path to res folder */
    // sprintf_s(buff, 64, "../../res/%s", file_path);

    unsigned char *data = stbi_load(file_path, &t.w, &t.h, &t.bpp, bbp);
    if (!data)
    {
        fprintf(stderr, "Loading image : %s\n", stbi_failure_reason());
        assert(data);
        return t;
    }

    t.data = data;

    return t;
}