#ifndef PHYSFS_BUNDLING_H
#define PHYSFS_BUNDLING_H
#include "common/int.h"

// Initialize PhysicsFS, then mount the folder where the executable is located
// and the bundled asset file into the virtual filesystem
bool setup_physfs(const char* argv0);

// Dump the bundled asset zip file to CWD so the user can edit the files
void dump_assets();

/// Read an entire file into a buffer using PhysicsFS.
/// Caller must free the resource.
/// \param path Filepath
/// \return Pointer to buffer, or NULL on failure.
u8* physfs_load_file(const char* path);

#endif // #ifndef PHYSFS_BUNDLING_H

