#ifndef SHADER_H
#define SHADER_H
#include <stdbool.h>

#include <glad/glad.h>

// Shader code is specific to each GPU, so we have to store the GLSL source
// code as a string and compile it at runtime. Loading the shaders from a file
// breaks if the files get moved/deleted, so it's better to store them as a
// hardcoded string in the program. We use CMake to automatically generate
// header files in the src/gui/shader folder with the contents of each shader
// enclosed in quotes. The generator code is at ext/txt2h.c.

// Make sure the shader program linked correctly, printing error messages if
// needed.
bool shader_link_check(gl_obj shader);

// Compile a shader from GLSL source in-memory
// shader_type is GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
gl_obj shader_compile_src(const char* src, GLenum shader_type);

// Load a GLSL source file from disk, then compile it with shader_compile_src()
// shader_type is GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
gl_obj shader_compile(const char* path, GLenum shader_type);

// Corresponding wrappers to compile a whole shader program. You should call
// shader_link_check() afterwards in case of failure.
gl_obj program_compile_src(const char* vert_src, const char* frag_src);
gl_obj program_compile(const char* vert_path, const char* frag_path);


#endif // #ifndef SHADER_H
