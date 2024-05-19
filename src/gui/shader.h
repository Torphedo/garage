#pragma once
#include <stdbool.h>

#include <glad/glad.h>

#include "gl_types.h"

// Make sure the shader program linked correctly, printing error messages if
// needed.
bool shader_link_check(gl_obj shader);

// Compile a shader from GLSL source in-memory
// shader_type is GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
gl_obj shader_compile_src(const char* src, GLenum shader_type);

// Load a GLSL source file from disk, then compile it with shader_compile_src()
// shader_type is GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, or GL_GEOMETRY_SHADER
gl_obj shader_compile(const char* path, GLenum shader_type);
