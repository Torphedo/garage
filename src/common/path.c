#include <stdbool.h>
#include <string.h>
#include <malloc.h>

#include "int.h"
#include "file.h"
#include "logging.h"


#include "platform.h"
#if defined(PLATFORM_APPLE)
    #include <mach-o/dyld.h>
#endif
#if defined(PLATFORM_POSIX)
    #include <fcntl.h>
#elif defined(PLATFORM_WINDOWS)
    #include <windows.h>
#endif

bool path_has_extension(const char* path, const char* extension) {
    const u32 pos = strlen(path);
    const u16 ext_length = strlen(extension);

    // File extension is longer than input string.
    if (ext_length > pos) {
        return false;
    }
    return (strncmp(&path[pos - ext_length], extension, ext_length) == 0);
}

void path_fix_backslashes(char* path) {
    u16 pos = strlen(path) - 1; // Subtract 1 so that we don't need to check null terminator
    while (pos > 0) {
        if (path[pos] == '\\') {
            path[pos] = '/';
        }
        pos--;
    }
}

void path_fix_forward_slashes(char* path) {
    u16 pos = strlen(path) - 1; // Subtract 1 so that we don't need to check null terminator
    while (pos > 0) {
        if (path[pos] == '/') {
            path[pos] = '\\';
        }
        pos--;
    }
}

bool path_has_slashes(const char* path) {
    s64 pos = strlen(path) - 1; // Subtract 1 so that we don't need to check null terminator
    while (pos >= 0) {
        if (path[pos] == '\\' || path[pos] == '/') {
            return true;
        }
        pos--;
    }

    // Didn't find anything
    return false;
}

void path_truncate(char* path, u16 pos) {
    path[--pos] = 0; // Removes last character to take care of trailing "\\" or "/".
    while(path[pos] != '\\' && path[pos] != '/' && pos >= 0) {
        path[pos--] = 0;
    }
}

void path_get_filename(const char* path, char* output) {
    u16 pos = strlen(path);
    while(path[pos] != '\\' && path[pos] != '/') {
        pos--;
    }
    strcpy(output, &path[pos] + 1);

}

char* get_self_path(const char* argv_0) {
    // Allocate space for output string
    const u32 size = 4096;
    char* out = calloc(1, size);
    if (out == NULL) {
        LOG_MSG(error, "Failed to alloc %d bytes for executable path\n", size);
        return NULL;
    }

    // Apple & Windows have a simple function we can call to grab our location
    #if defined(PLATFORM_APPLE) || defined(PLATFORM_WINDOWS)
        // On Windows we use GetModuleFileName()
        #if defined(PLATFORM_WINDOWS)
            DWORD nchar = GetModuleFileName(NULL, out, size - 1);
            if (nchar == 0 || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                LOG_MSG(error, "Nothing was written, or the path was too big to fit in %d bytes\n", size);
            }

        // On Apple we use _NSGetExecutablePath()
        #else
            u32 size_out = 0; // Temp variable
            if (_NSGetExecutablePath(out, &size_out) != 0) {
                LOG_MSG(error, "Nothing was written, or the path was too big to fit in %d bytes\n", size);
            }
        #endif

        // Cut off the filename, leaving the parent dir
        path_truncate(out, strlen(out));
        out[strlen(out) - 1] = 0; // Remove the final trailing slash
        return out;
    #endif

    // On most Unixes we can use the /proc/self/exe symlink. On OpenBSD, we'd
    // have to search the PATH and hope for the best.
    #if defined(PLATFORM_POSIX)
        // Symlink pointing to the location of our own executable
        char* selflink = "/proc/self/exe"; // Default to Linux 
        if (!file_exists(selflink)) {
            // DragonFlyBSD, FreeBSD if /proc is enabled
            selflink = "/proc/curproc/file";
        }
        if (!file_exists(selflink)) {
            // NetBSD
            selflink = "/proc/curproc/exe";
        }
        if (!file_exists(selflink)) {
            // Solaris
            // We could also use getexecname() on Solaris, but this should work.
            selflink = "/proc/self/path/a.out";
        }

        if (file_exists(selflink)) {
            // Find out where the symlink points
            const ssize_t len = readlink(selflink, out, size - 1);
            if (len == -1) {
                LOG_MSG(error, "Failed to read symlink to get executable location (probably was over %d bytes)\n", size);
                return out;
            }

            // We found it :)
            // Chop off executable filename
            path_truncate(out, len);
            out[strlen(out) - 1] = 0; // Remove the final trailing slash
            return out;
        }
    #endif

    
    // This will only be hit on BSD, where argv[0] is the only hope to maybe
    // find out our location
    if (argv_0 == NULL) {
        LOG_MSG(error, "argv[0] was a nullptr (and our only hope)!\n", argv_0);
        free(out);
        return NULL;
    }

    // If the file exists, it's a relative path like "build/program" or "./program".
    // If there's no slashes, it must be a program that has a matching filename
    // in the current folder, but was actually invoked from a copy on the PATH.
    if (file_exists(argv_0) && path_has_slashes(argv_0)) {
        const u64 len = strlen(argv_0);
        if (len > size) {
            // Realloc if the argv[0] is absolutely massive for some reason
            free(out);
            out = calloc(1, len);
            if (out == NULL) {
                LOG_MSG(error, "Couldn't allocate %d bytes to clone argv[0] string \"%s\"\n", argv_0);
                return NULL;
            }
        }
        memcpy(out, argv_0, len);

        // If argv[0] exists, we're running from an absolute or relative path. 
        // So, we can just chop off the filename and we have our parent dir.
        path_truncate(out, len);
        out[strlen(out) - 1] = 0; // Remove the final trailing slash
        return out;
    }
    #if defined(PLATFORM_POSIX)
    else {
        if (!file_exists(selflink)) {
            LOG_MSG(error, "Unimplemented: We've been run from the PATH, but no usable /proc symlinks for finding our location exist (we're probably on OpenBSD).\n");
            LOG_MSG(info, "Run this program with an absolute or relative path to bypass this issue.\n");
            return out;
        }

    }
    #endif

    // Oh well.
    return out;
}

