#ifndef PATH_H
#define PATH_H

#include <stdbool.h>
#include <stdint.h>

bool path_has_extension(const char* path, const char* extension);

/// Replaces all backslashes in a string with forward slashes.
/// \param path String to edit. Must be null-terminated.
void path_fix_backslashes(char* path);

void path_fix_forward_slashes(char* path);

/// Truncates a filename or folder name from a path, leaving a trailing "\\" or "/".
/// \param path The path to truncate. This string will be edited. If the string does not contain any slashes or backslashes, it will be completely filled with null characters.
/// \param pos The position to start searching for directory separators. This should usually be the string's length + 1.
void path_truncate(char* path, uint16_t pos);

// Checks if a path has any forward or backslashes
bool path_has_slashes(const char* path);

void path_get_filename(const char* path, char* output);

// Find the parent directory of this executable. argv[0] is only needed on BSD,
// but you should provide it anyway.
// Caller is responsible for freeing the output string
char* get_self_path(const char* argv_0);

#endif // #ifndef PATH_H

