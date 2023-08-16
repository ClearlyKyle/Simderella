#ifndef __SOFTRAWR_H__
#define __SOFTRAWR_H__

// #define GRAPHICS_USE_SDL_RENDERER

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

    SDL_Window *window;

#ifdef GRAPHICS_USE_SDL_RENDERER
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
#else
    SDL_Surface     *surface;
    SDL_PixelFormat *fmt;
    uint8_t         *pixels;
#endif

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

#ifdef GRAPHICS_USE_SDL_RENDERER
    SDL_Renderer *renderer   = SDL_CreateRenderer(global_renderer.window, -1, 0);
    global_renderer.renderer = renderer;

    SDL_Texture *texture    = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, width, height);
    global_renderer.texture = texture;
#else
    SDL_Surface     *window_surface = SDL_GetWindowSurface(global_renderer.window);
    global_renderer.surface         = window_surface;

    global_renderer.fmt = window_surface->format;

    printf("Window Surface\n\tPixel format : %s\n", SDL_GetPixelFormatName(global_renderer.surface->format->format));
    printf("\tBytesPP      : %d\n", global_renderer.fmt->BytesPerPixel);
    printf("\tBPP          : %d\n", global_renderer.fmt->BitsPerPixel);
    printf("\tPitch : %d\n", window_surface->pitch);

    // global_renderer.pixels            = (uint8_t *)window_surface->pixels;
    global_renderer.pixels = (uint8_t *)window_surface->pixels;
    global_renderer.height = window_surface->h;
    global_renderer.width  = window_surface->w;

    // https://stackoverflow.com/questions/20070155/how-to-set-a-pixel-in-a-sdl-surface
    global_renderer.screen_num_pixels = window_surface->h * window_surface->w * window_surface->format->BytesPerPixel;
#endif
    global_renderer.running = true;

    return true;
}

static void Renderer_Destroy(void)
{
    if (global_renderer.window)
    {
        // global_renderer.surface is freed when calling this function
        SDL_DestroyWindow(global_renderer.window);
        global_renderer.window  = NULL;
        global_renderer.surface = NULL;
        global_renderer.pixels  = NULL;
    }

#ifdef GRAPHICS_USE_SDL_RENDERER
    if (global_renderer.renderer)
    {
        SDL_DestroyRenderer(global_renderer.renderer);
        global_renderer.renderer = NULL;
    }
    if (global_renderer.texture)
    {
        SDL_DestroyTexture(global_renderer.texture);
        global_renderer.texture = NULL;
    }
#endif
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