SIMDERELLA 3D Software Rasterizer
=================================

SIMDERELLA is a 3D software rasterizer written in C using SIMD and multi-threading to accelerate the rendering process. This rasterizer is designed to provide efficient and fast rendering capabilities, with the ability to use vertex and fragment-like shaders similar to OpenGl.

![cube](images\cube.PNG)

Features
--------
- SIMD instructions for enhanced processing speed
- Multi-threading support through a job system
- Vertex and fragment-like shader system

Overview of Rasterization Algorithm
-----------------------------------

The task of rendering polygons falls two main steps: ethe dge function computation and scanline processing. This approach allows for efficient rasterizing of polygons.

   - For each edge of a polygon, the algorithm computes the edge function at the start of the scanline and increments it as it steps horizontally across the scanline.
   - The algorithm processes each scanline of the polygon to determine which pixels to fill.

Dependencies
------------

SIMDERELLA depends on the following libraries:

- tinyobj: A small and easy-to-use Wavefront OBJ loader written in C++
- cglm: A C99-based library for vector and matrix mathematics
- stb_image: A single-file library for loading various image file formats
