#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <glad/glad.h>

#include <common/file.h>
#include <common/int.h>
#include <common/logging.h>
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
    s32 link_success = 0;
    glGetProgramiv(shader, GL_LINK_STATUS, &link_success);
    if (link_success) {
        return true;
    }
    LOG_MSG(error, "Failed to link shader program:\n");
    program_print_log(shader);

    return false;
}

gl_obj shader_compile_src(const char* src, GLenum shader_type) {
    // Create & compile shader code
    gl_obj shader = glCreateShader(shader_type);
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

gl_obj shader_compile(const char* path, GLenum shader_type) {
    // Load shader source code
    u8* shader_source = file_load(path);
    if (shader_source == NULL) {
        LOG_MSG(error, "Failed to open GLSL source file %s\n", path);
        return 0;
    }
    gl_obj shader = shader_compile_src((char*)shader_source, shader_type);
    free(shader_source);
    return shader;
}

