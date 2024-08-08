#include <malloc.h>
#include <stdio.h>

#define INCNBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <incbin.h>

#include <physfs.h>
#include <common/path.h>
#include <common/logging.h>
#include <common/int.h>

INCBIN(asset_archive, "data.7z");

bool setup_physfs(const char* argv0) {
    char* self_path = get_self_path(argv0);
    PHYSFS_init(argv0);
    if (PHYSFS_mount(self_path, "/", 0) == 0) {
        LOG_MSG(error, "Failed to mount %s\n", self_path);
        free(self_path);
        return false;
    }
    else {
        LOG_MSG(debug, "Mounted %s as virtual filesystem root\n", self_path);
    }

    // We give it a suggested filenames with a slash because it's invalid on all
    // filesystems, so there'll never be a name conflict
    if (PHYSFS_mountMemory(gasset_archive_data, gasset_archive_size, NULL, "/garage_data.7z", "/", 1) == 0) {
        LOG_MSG(error, "Somehow failed to mount bundled assets!\n");
        return  false;
    }

    free(self_path);

    return true;
}

void dump_assets() {
    FILE* dump = fopen("data.7z", "wb");
    if (dump == NULL) {
        return;
    }

    fwrite(gasset_archive_data, gasset_archive_size, 1, dump);
    fclose(dump);
}

u8* physfs_load_file(const char* path) {
    PHYSFS_File* resource = PHYSFS_openRead(path);
    if (resource == NULL) {
        LOG_MSG(error, "Failed to open \"%s\" via PHYSFS!\n", path);
        return NULL;
    }
    s64 filesize = PHYSFS_fileLength(resource);
    if (filesize == -1) {
        LOG_MSG(debug, "Couldn't get size of file!\n");
        return NULL;
    }
    // We add an extra byte here for the null terminator, in case of text data
    u8* data = calloc(1, filesize + 1);
    if (data == NULL) {
        LOG_MSG(error, "Failed to load \"%s\" via PHYSFS!\n", path);
        return NULL;
    }
    PHYSFS_readBytes(resource, data, filesize);
    PHYSFS_close(resource);

    return data;
}

