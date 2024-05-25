#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <sys/stat.h>

// MSVC doesn't define S_ISREG() or S_ISDIR() in stat.h, so we have to do it.
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
    #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
    #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#include "logging.h"
#include "file.h"

bool file_exists(const char* path) {
    struct stat st = {0};
    return (stat(path, &st) == 0);
}

bool path_is_file(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

bool path_is_dir(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool path_has_extension(const char* path, const char* extension) {
    uint32_t pos = strlen(path);
    uint16_t ext_length = strlen(extension);

    // File extension is longer than input string.
    if (ext_length > pos) {
        return false;
    }
    return (strncmp(&path[pos - ext_length], extension, ext_length) == 0);
}

u32 file_size(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) {
        return 0;
    }
    return st.st_size;
}

u8* file_load(const char* path) {
    if (!file_exists(path)) {
        LOG_MSG(error, "Requested file %s does not exist.\n", path);
        return NULL;
    }
    u32 size = file_size(path);
    u8* buffer = calloc(1, size);
    if (buffer == NULL) {
        return NULL;
    }

    file_load_existing(path, buffer, size);
    return buffer;
}

bool file_load_existing(const char* path, u8* buf, u32 size) {
    if (buf == NULL) {
        LOG_MSG(error, "Caller provided NULL buffer. Check your allocations!\n");
        return false;
    }
    if (!file_exists(path)) {
        LOG_MSG(error, "Requested file %s does not exist.\n", path);
        return false;
    }

    u32 filesize = file_size(path);
    if (filesize > size) {
        LOG_MSG(error, "Not enough space for file %s\n", path);
        return false;
    }

    FILE* resource = fopen(path, "rb");
    if (resource == NULL) {
        return false;
    }

    fread(buf, size, 1, resource);
    fclose(resource);
    return true;
}

