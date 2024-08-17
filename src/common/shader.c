#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <glad/glad.h>

#include "file.h"
#include "int.h"
#include "logging.h"
#include "shader.h"

// Only for the shader program (not the individual shader objects)
void program_print_log(gl_obj shader) {
    s32 log_size = 0;
    glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &log_size);
    
    char log_buf[0x1000] = {0};

    // If the log is really long (> 4KiB), it won't fit in our stack array.
    if (log_size > sizeof(log_buf)) {
        LOG_MSG(warning, "Abnormally large log (%d bytes) forced a heap allocation.\n", log_size);
        // Allocate enough space to store the log
        char* log_heap = calloc(1, log_size);
        if (log_heap == NULL) {
            LOG_MSG(error, "Couldn't allocate %d bytes needed to display the shader compiler's error log!\n", log_size);
            return;
        }

        glGetProgramInfoLog(shader, log_size, NULL, log_heap);
        printf("%s\n", log_heap);
        free(log_heap);
        return;
    }

    // Get the log and print it
    glGetProgramInfoLog(shader, sizeof(log_buf), NULL, log_buf);
    printf("%s\n", log_buf);
}

// Linker error checking
bool shader_link_check(gl_obj shader) {
    if (shader == 0) {
        return false;
    }
    s32 link_success = 0;
    glGetProgramiv(shader, GL_LINK_STATUS, &link_success);
    if (link_success) {
        return true;
    }
    LOG_MSG(error, "Failed to link shader program:\n");
    program_print_log(shader);

    return false;
}

gl_obj program_compile_src(const char* vert_src, const char* frag_src) {
    // Compile shaders
    const gl_obj vertex_shader = shader_compile_src(vert_src, GL_VERTEX_SHADER);
    const gl_obj fragment_shader = shader_compile_src(frag_src, GL_FRAGMENT_SHADER);
    if (vertex_shader == 0 || fragment_shader == 0) {
        LOG_MSG(error, "Failed to compile shaders\n");
        return 0;
    }
    const gl_obj program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    // Free the shader objects
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

gl_obj shader_compile_src(const char* src, GLenum shader_type) {
    // Create & compile shader code
    const gl_obj shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, (const GLchar* const *) &src, NULL);
    glCompileShader(shader);

    // Check for shader compile errors
    s32 success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[0x1000] = {0};
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        LOG_MSG(error, "Failed to compile shader:\n%s\n", log);
    }
    return shader;
}

