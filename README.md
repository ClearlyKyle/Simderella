SIMDERELLA 3D Software Rasterizer
=================================

SIMDERELLA is a 3D software rasterizer written in C utilizing SIMD and multi-threading to accelerate the rendering process. This rasterizer is designed to provide efficient and fast rendering capabilities, with the ability to use vertex and fragment-like shaders similar to OpenGL.

![cube](https://raw.githubusercontent.com/ClearlyKyle/Simderella/master/images/cube.PNG)

Features
--------
- SIMD instructions for enhanced processing speed
- Multi-threading support through a job system
- Vertex and fragment-like shader system

Dependencies
------------

- tinyobj: A small and easy-to-use Wavefront OBJ loader written in C++
- cglm: A C99-based library for vector and matrix mathematics
- stb_image: A single-file library for loading various image file formats
