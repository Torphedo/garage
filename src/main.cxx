#include <cstring>

#include <common/int.h>
#include <common/logging.h>
#include <gui_bootstrap.hxx>

#include "editor/layer_editor.hxx"
#include "editor/render_text.hxx"
#include "editor/layer_garage.hxx"
#include "editor/layer_hud.hxx"
#include "editor/layer_debug.hxx"

extern "C" {
#include <common/gl/gl_setup.h>
#include <common/gl/input.h>

#include "physfs_bundling.h"
}

// The file IO works a little strangely. We pack all assets in a zip file and
// statically link it into the executable as part of the build. When we use a
// filepath, it checks the directory and then the embedded zip file for that
// path.
int main(int argc, char** argv) {
    enable_win_ansi(); // Enable color & extra terminal features on Windows
    if (argc != 2) {
        LOG_MSG(error, "No input files.\n");
        LOG_MSG(info, "Usage: garage [vehicle file]\n");
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "--dump-assets") == 0) {
        dump_assets();
        return EXIT_SUCCESS;
    }

    // Setup PhysicsFS
    if (!setup_physfs(argv[0])) {
        return EXIT_FAILURE;
    }
    const char* vehicle_path = argv[1]; // Give our first argument a convenient name

    gui_app app;
    app.layers.emplace_back(std::make_unique<editor_state>(vehicle_path));
    editor_state& editor = *dynamic_cast<editor_state*>(app.layers[0].get());

    app.layers.emplace_back(std::make_unique<layer_garage>(editor));
    app.layers.emplace_back(std::make_unique<layer_debug>(editor));
    app.layers.emplace_back(std::make_unique<layer_hud>(editor));

    app.run("Garage Opener");

    return 0;
}
