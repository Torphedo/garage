#ifndef SHADER_H
#define SHADER_H
#include <stdbool.h>

#include <glad/glad.h>

// Shader code is specific to each GPU, so we have to store the GLSL source
// code as a string and compile it at runtime. Loading the shaders from a file
// breaks if the files get moved/deleted, so it's better to store them inside
// the program itself. We use CMake and incbin (in the ext/ folder) to
// automatically compress and embed shader files into the executable.

// Ensure the shader program linked correctly, printing error messages if needed
bool shader_link_check(gl_obj shader);

// Compile a shader from GLSL source in-memory
// shader_type is GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
gl_obj shader_compile_src(const char* src, GLenum shader_type);

// A wrapper to compile a whole shader program. You should call
// shader_link_check() afterwards in case of failure.
gl_obj program_compile_src(const char* vert_src, const char* frag_src);

#endif // #ifndef SHADER_H
