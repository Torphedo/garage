#include <stdio.h>

#include "common/int.h"
#include "common/logging.h"

#include "vehicle.h"
#include "part_ids.h"

int main(int argc, char** argv) {
    enable_win_ansi();
    check_part_entry(); // Sanity check for padding bugs
    if (argc != 2) {
        LOG_MSG(error, "No input files.\n");
        LOG_MSG(info, "Usage: vtweak [vehicle file]");
        return 1;
    }
    char* path = argv[1];
    vehicle* v = vehicle_load(path);
    if (v == NULL) {
        // Loader function will print the error message for us
        return 1;
    }

    }
    }

    LOG_MSG(info, "\"%ls\" has %d parts\n", v->head.name, v->head.part_count);
    for (u16 i = 0; i < v->head.part_count; i++) {
        LOG_MSG(info, "Part %d: 0x%x [%s] ", i, v->parts[i].id, part_get_name(v->parts[i].id));
        printf("@ (%d, %d, %d) ", v->parts[i].pos.x, v->parts[i].pos.y, v->parts[i].pos.z);
        printf("painted #%x%x%x", v->parts[i].color.r, v->parts[i].color.g, v->parts[i].color.b);
        printf(", modifier 0x%02hx\n", v->parts[i].modifier);
    }

    return 0;
}
