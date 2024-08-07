// Initialize PhysicsFS, then mount the folder where the executable is located
// and the bundled asset file into the virtual filesystem
bool setup_physfs(const char* argv0);

// Dump the bundled asset zip file to CWD so the user can edit the files
void dump_assets();

