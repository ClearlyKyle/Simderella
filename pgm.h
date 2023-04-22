#ifndef __PGM_H__
#define __PGM_H__

#include <stdio.h>
#include <stdlib.h>

inline void PGM_Write(const char *file_name, const int w, const int h, const void *buffer, const size_t element_size, const size_t element_count)
{
    FILE *fp1 = NULL;

    errno_t err = fopen_s(&fp1, file_name, "wb");
    if (err != 0)
    {
        printf("Error opening file: %s\nError code: %d\n", file_name, err);
        return;
    }

    /* "P6\nW H\n255\n" */
    fprintf(fp1, "P6\n%d %d\n255\n", w, h);

    (void)fwrite(buffer, element_size, element_count, fp1);

    fclose(fp1);
    fprintf(stderr, "pgm written successfully\n");
}

#endif // __PGM_H__