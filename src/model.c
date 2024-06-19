#include <stdio.h>
#include <string.h>

#include <glad/glad.h>

#include "common/logging.h"
#include "common/file.h"
#include "gui/gui_common.h"

void model_upload(model* m) {
    gl_obj buffers[2];
    glGenBuffers(2, buffers);
    glGenVertexArrays(1, &m->vao);
    m->vbuf = buffers[0];
    m->ibuf = buffers[1];

    // Setup vertex buffer
    glBindVertexArray(m->vao);
    glBindBuffer(GL_ARRAY_BUFFER, m->vbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * m->vert_count, m->vertices, GL_STATIC_DRAW);

    // Setup index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * m->idx_count, m->indices, GL_STATIC_DRAW);

    // Create vertex layout
    glVertexAttribPointer(0, sizeof(vec3) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, sizeof(vec4) / sizeof(float), GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, color));
    glEnableVertexAttribArray(1);

    // Unbind our buffers to avoid messing our state up
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

u32 model_size(model m) {
    return sizeof(m) - (2 * sizeof(void*)) + (m.vert_count * sizeof(vertex)) + (m.idx_count * sizeof(u16));
}

model obj_load(const char* path) {
    model out = {0};
    // Load the whole file into a text buffer so we don't need to read it twice
    char* txt = (char*)file_load(path);
    if (txt == NULL) {
        // file_load() already prints an error message. No need to spam log
        // LOG_MSG(error, "Failed to load \"%s\"\n", path);
        return out;
    }

    // We parse the file in 2 passes. The first pass tallies the vertex/face
    // counts to allocate the vertex/index buffers, and the second loads the
    // actual data.
    for (u8 i = 1; i < 3; i++) {
        // Used to track current position in buffer. We discard const-ness here
        vertex* vpos = (vertex*)out.vertices;
        u16* ipos = (u16*)out.indices;
        // Loop over the already-loaded OBJ file.
        // Final null terminator indicates end of file
        char* line = txt;
        while (*line != 0x00) {
            // Get pointer to end of line (NUL or newline)
            char* line_end = strchr(line, '\n');
            if (line_end == NULL) {
                break;
            }
            // Replace newline with NUL, so we only search the current line in
            // strstr(). Save original character so we can undo the change.
            char end = *line_end;
            *line_end = 0x00; 

            if (line[0] != '#') {
                // Faces
                if (line[0] == 'f') {
                    if (i == 1) {
                        // Each face requires 3 indices
                        out.idx_count += 3;
                    }
                    else {
                        // Read the index data
                        sscanf(line, "f %hd %hd %hd", &ipos[0], &ipos[1], &ipos[2]);
                        // OBJ indices are 1-based...
                        ipos[0]--;
                        ipos[1]--;
                        ipos[2]--;
                        ipos += 3; // Advance by 3 indices
                    }
                }
                // Vertex position
                if (strstr(line, "v ") != NULL) {
                    if (i == 1) {
                        out.vert_count++;
                    }
                    else {
                        // Read the vertex data
                        sscanf(line, "v %f %f %f %f %f %f", &vpos->position[0], &vpos->position[1],&vpos->position[2], &vpos->color[0], &vpos->color[1], &vpos->color[2]);
                        vpos->color[3] = 1.0f; // Set alpha channel
                        vpos++;
                    }
                }
            }

            // Undo our change & advance to next line
            *line_end = end;
            line = line_end + 1;
        }

        // Before the second pass, we need to alloc the vertex & index buffers.
        if (i == 1) {
            out.vertices = calloc(out.vert_count, sizeof(vertex));
            out.indices = calloc(out.idx_count, sizeof(u16));
            if (out.vertices == NULL || out.indices == NULL) {
                LOG_MSG(error, "Buffer alloc failure!\n");
                printf("\tVertex buffer %p (0x%lx bytes)\n", out.vertices, out.vert_count * sizeof(vertex));
                printf("\tIndex buffer %p (0x%lx bytes)\n", out.indices, out.idx_count * sizeof(u16));
                break;
            }
        }
    }

    free(txt);
    return out;
}
