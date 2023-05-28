#ifndef __SOFTRAWR_H__
#define __SOFTRAWR_H__

#include <stdbool.h>
#include <float.h>

#include "SDL2/SDL.h"
#include "utils/timer.h"

typedef struct Renderer_s
{
    bool running;
    int  width;
    int  height;
    int  screen_num_pixels;

    SDL_Window  *window;
    SDL_Surface *surface;

    SDL_Renderer *renderer;
    SDL_Texture  *texture;

    SDL_PixelFormat *fmt;
    uint8_t         *pixels;

    float  max_depth_value;
    float *depth_buffer;
} Renderer;

extern Renderer global_renderer;

static bool Reneder_Startup(const char *title, const int width, const int height)
{
    memset((void *)&global_renderer, 0, sizeof(Renderer));

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Could not SDL_Init(SDL_INIT_VIDEO): %s\n", SDL_GetError());
        return false;
    }

    global_renderer.window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        SDL_WINDOW_SHOWN); // show upon creation

    if (global_renderer.window == NULL)
    {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        return false;
    }

    SDL_Surface *window_surface = SDL_GetWindowSurface(global_renderer.window);

    // Allocate z buffer
    // float *z_buff = (float *)_aligned_malloc(sizeof(float) * width * height, 16);
    float *z_buff = (float *)malloc(sizeof(float) * width * height);
    if (!z_buff)
    {
        fprintf(stderr, "Error with : (float *)_aligned_malloc(sizeof(float) * global_renderer.screen_num_pixels, 16);\n");
        return false;
    }

    global_renderer.renderer = SDL_CreateRenderer(global_renderer.window, -1, SDL_RENDERER_ACCELERATED);
    global_renderer.texture  = SDL_CreateTexture(global_renderer.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1024, 512);

    // Get window data
    global_renderer.surface = window_surface;
    global_renderer.fmt     = window_surface->format;

    printf("Pixel format : %s\n", SDL_GetPixelFormatName(global_renderer.surface->format->format));
    printf("BytesPP      : %d\n", global_renderer.fmt->BytesPerPixel);
    printf("BPP          : %d\n", global_renderer.fmt->BitsPerPixel);

    global_renderer.pixels            = (uint8_t *)window_surface->pixels;
    global_renderer.height            = window_surface->h;
    global_renderer.width             = window_surface->w;
    global_renderer.screen_num_pixels = window_surface->h * window_surface->w;

    global_renderer.depth_buffer    = z_buff;
    global_renderer.max_depth_value = FLT_MAX;

    global_renderer.running = true;

    return true;
}

static void Renderer_Destroy(void)
{
    if (global_renderer.window)
    {
        SDL_DestroyWindow(global_renderer.window);
        global_renderer.window = NULL;
    }

    if (global_renderer.surface)
    {
        SDL_FreeSurface(global_renderer.surface);
        global_renderer.surface = NULL;
        global_renderer.pixels  = NULL;
    }

    if (global_renderer.pixels)
    {
        free(global_renderer.pixels);
        global_renderer.pixels = NULL;
    }

    if (global_renderer.depth_buffer)
    {
        //_aligned_free(global_renderer.depth_buffer);
        free(global_renderer.depth_buffer);
        global_renderer.depth_buffer = NULL;
    }

    SDL_DestroyRenderer(global_renderer.renderer);
    SDL_DestroyTexture(global_renderer.texture);

    SDL_Quit();

    fprintf(stderr, "Renderer has been destroyed\n");
}

static inline void Renderer_Clear_Screen_Pixels(void)
{
    memset(global_renderer.pixels, 0, global_renderer.screen_num_pixels * 4);
}

static inline void Renderer_Clear_Depth_Buffer(void)
{
    float *end = &global_renderer.depth_buffer[global_renderer.screen_num_pixels];
    for (float *p = global_renderer.depth_buffer; p != end; p++)
        *p = global_renderer.max_depth_value;

    // for (size_t i = 0; i < global_renderer.screen_num_pixels; i++)
    //{
    //     global_renderer.depth_buffer[i] = global_renderer.max_depth_value;
    // }

    // const float *END = &global_renderer.depth_buffer[global_renderer.screen_num_pixels];
    // for (float *i = global_renderer.depth_buffer;
    //      i < END;
    //      i += 1)
    //{
    //     *i = global_renderer.max_depth_value;
    // }
}

static inline void Renderer_Present(void)
{
    SDL_UpdateWindowSurface(global_renderer.window);
}

static inline void Draw_Pixel_RGBA(const int x, const int y, const uint8_t red, const uint8_t green, const uint8_t blue, const uint8_t alpha)
{
    const int index               = y * global_renderer.width + x;
    global_renderer.pixels[index] = (Uint32)((alpha << 24) + (red << 16) + (green << 8) + (blue << 0));
}

static inline void Renderer_Set_Title(char *buff)
{
    SDL_SetWindowTitle(global_renderer.window, buff);
}

#endif // __SOFTRAWR_H__