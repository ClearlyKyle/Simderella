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

    SDL_PixelFormat *fmt;
    uint8_t         *pixels;
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

    // Get window data
    global_renderer.surface = window_surface;
    global_renderer.fmt     = window_surface->format;

    printf("Pixel format : %s\n", SDL_GetPixelFormatName(global_renderer.surface->format->format));
    printf("BytesPP      : %d\n", global_renderer.fmt->BytesPerPixel);
    printf("BPP          : %d\n", global_renderer.fmt->BitsPerPixel);

    global_renderer.pixels            = (uint8_t *)window_surface->pixels;
    global_renderer.height            = window_surface->h;
    global_renderer.width             = window_surface->w;
    global_renderer.screen_num_pixels = window_surface->h * window_surface->w * window_surface->format->BytesPerPixel;

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

    SDL_Quit();

    fprintf(stderr, "Renderer has been destroyed\n");
}

static inline void Renderer_Present(void)
{
    SDL_UpdateWindowSurface(global_renderer.window);
}

static inline void Renderer_Set_Title(char *buff)
{
    SDL_SetWindowTitle(global_renderer.window, buff);
}

#endif // __SOFTRAWR_H__