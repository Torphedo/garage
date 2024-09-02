#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>

#include <common/endian.h>
#include <common/file.h>
#include <common/vector.h>
#include <common/logging.h>
#include <cglm/cglm.h>

#include "vehicle.h"
#include "parts.h"
#include "stfs.h"

void part_byteswap(part_entry* part) {
    if (is_little_endian()) {
        ENDIAN_FLIP(u32, part->unknown);
        ENDIAN_FLIP(u32, part->id);
        vec3_byteswap(&part->rot);

        // Byte-swap the padding just in case :P
        ENDIAN_FLIP(u16, part->pad);
        ENDIAN_FLIP(u32, part->pad3);
    }
}

void vehicle_header_byteswap(vehicle_header* v) {
    if (is_little_endian()) {
        ENDIAN_FLIP(u64, v->magic);
        ENDIAN_FLIP(u16, v->part_count);
        for (u32 i = 0; i < ARRAY_SIZE(v->name); i++) {
            // 16-bit characters are also subject to endian-ness
            ENDIAN_FLIP(c16, v->name[i]);
        }
        ENDIAN_FLIP_FLOAT(v->weight);
        ENDIAN_FLIP_FLOAT(v->power);
        ENDIAN_FLIP_FLOAT(v->integrity);
    }
}

vehicle* vehicle_load(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        LOG_MSG(error, "Failed to open %s\n", path);
        return NULL;
    }

    vehicle_header head = {0};
    fread(&head, sizeof(head), 1, f);
    fclose(f);
    vehicle_header_byteswap(&head);

    vehicle* v = NULL;
    if (head.magic == VEHICLE_MAGIC) {
        // Header claims it's a vehicle file, just load it directly
        LOG_MSG(debug, "Loading vehicle from raw save file %s\n", path);
        v = (vehicle*)file_load(path); // Reads file into a buffer for us
    }
    else {
        // Not a vehicle file, try to load it as an STFS save file.
        LOG_MSG(debug, "Loading vehicle from STFS save entry\n");
        v = (vehicle*)stfs_get_vehicle(path);
    }

    if (v == NULL) {
        LOG_MSG(error, "Failed to load vehicle\n");
        return NULL;
    }

    // Byte-swap everything & return result
    vehicle_header_byteswap(&v->head);
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_byteswap(&v->parts[i]);
    }
    return v;
}

