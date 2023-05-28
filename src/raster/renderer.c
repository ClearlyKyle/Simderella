#include "renderer.h"

RendererState_t RenderState = {0};

RasterData_t Trianges_To_Be_Rastered[MAX_NUMBER_OF_TRIANGLES_TO_RASTER] = {0};
size_t       Trianges_To_Be_Rastered_Counter                            = 0;