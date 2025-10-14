#include <cstring>
#include <cstdio>

#include <glad/glad.h>

#include <common/int.h>
#include <common/logging.h>
#include <gui_bootstrap.hxx>

#include "editor/editor.hxx"
#include "editor/render_text.hxx"
#include "editor/render_garage.hxx"
#include "editor/render_user.hxx"
#include "editor/vehicle_edit.hxx"
#include "editor/render_debug.hxx"

extern "C" {
#include <common/gl/gl_setup.h>
#include <common/gl/input.h>

#include "editor/camera.hxx"
#include "vehicle.h"
#include "parts.h"
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
    app.layers.reserve(5);
    app.layers.emplace_back(std::make_unique<editor_state>(vehicle_path));
    editor_state* editor = dynamic_cast<editor_state*>(app.layers[0].get());

    app.layers.emplace_back(std::make_unique<garage_state>(editor));
    app.layers.emplace_back(std::make_unique<layer_debug>(editor));
    app.layers.emplace_back(std::make_unique<editor_ui>(editor));

    app.run("Garage Opener");

    /*
    // Print vehicle details (mostly a leftover from old versions of this program)
    LOG_MSG(info, "\"");
    print_c16s(editor.v.name); // We need a special function to portably print UTF-16
    printf("\" has %d parts & weighs %f\n", editor.v.part_count, editor.v.weight);

    part_iterator iter = part_iterator_setup(editor, SEARCH_ALL);
    u32 i = 0;
    while (!iter.done) {
        i++;
        part_entry* p = part_iterator_next(&iter);
        LOG_MSG(info, "Part %d: 0x%x [%s] ", i, p->id, part_get_info((part_id)p->id).name);
        printf("@ (%d, %d, %d) ", p->pos.x, p->pos.y, p->pos.z);
        printf("painted #%x%x%x", p->color.r, p->color.g, p->color.b);
        printf(", modifier 0x%02hx\n", p->modifier);
    }
     */

    return 0;
}
