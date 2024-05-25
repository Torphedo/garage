#ifndef FILE_H
#define FILE_H

#include <stdbool.h>

#include "int.h"

bool file_exists(const char* path);
bool path_is_file(const char* path);
bool path_is_dir(const char* path);

// Check if a path has a file extension (like ".zip"):
// path_has_extension("data/file.zip", ".zip") == true
bool path_has_extension(const char* path, const char* extension);

u32 file_size(const char* path);

/// Read an entire file into a buffer. Caller must free the resource.
/// \param path Filepath
/// \return Pointer to buffer, or NULL on failure.
u8* file_load(const char* path);

/// Read an entire file into an existing buffer.
/// \param path Filepath
/// \param buf Buffer to read file into
/// \param size Size of buffer
bool file_load_existing(const char* path, u8* buf, u32 size);

static inline u32 magic(char a, char b, char c, char d) {
    return a | (b << 8) | (c << 16) | (d << 24);
}
#define MAGIC(a, b, c, d) ((u32)a | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24))

#endif // #ifndef FILE_H

