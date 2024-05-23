#include <malloc.h>
#include "common/endian.h"
#include "common/file.h"

#include "vehicle.h"
#include "stfs.h"

// TODO: Make this a no-op when compiling for a big endian system
void part_byteswap(part_entry* part) {
    ENDIAN_FLIP(u32, part->unknown);
    ENDIAN_FLIP(u32, part->id);
    vec3_byteswap(&part->rot);
    ENDIAN_FLIP(u32, part->pad3);
}

void vehicle_header_byteswap(vehicle_header* v) {
    ENDIAN_FLIP(u64, v->magic);
    ENDIAN_FLIP(u16, v->part_count);
    for (u32 i = 0; i < 0x20; i++) {
        ENDIAN_FLIP(c16, v->name[i]);
    }
}

vec3s vehicle_find_center(vehicle* v) {
    vec3u8 max_horizontal = {0};
    u8 top = 0;
    for (u16 i = 0; i < v->head.part_count; i++) {
        vec3u8 pos = v->parts[i].pos;
        if (pos.x > max_horizontal.x || pos.z > max_horizontal.z) {
            max_horizontal = pos;
        }
        if (pos.y > top) {
            top = pos.y;
        }
    }

    // Return the centerpoint
    return (vec3s) {
        .x = (float)max_horizontal.x / 2,
        .y = (float)top / 2,
        .z = (float)max_horizontal.z / 2,
    };
}

part_entry part_read(FILE* f) {
    part_entry part = {0};
    fread(&part, sizeof(part), 1, f);
    part_byteswap(&part);
    return part;
}

vehicle* vehicle_load(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        LOG_MSG(error, "Failed to open %s\n", path);
        return NULL;
    }

    vehicle* v = NULL;
    vehicle_header head = {0};
    fread(&head, sizeof(head), 1, f);
    vehicle_header_byteswap(&head);
    if (head.magic == VEHICLE_MAGIC) {
        u32 size = file_size(path);
        v = calloc(1, size);
        if (v == NULL) {
            LOG_MSG(error, "Couldn't alloc %d bytes for raw vehicle file\n", size);
            return NULL;
        }
        fseek(f, 0, SEEK_SET);
        fread(v, size, 1, f);
        LOG_MSG(debug, "Vehicle loaded from raw save file\n");
        fclose(f);
    }
    else {
        fclose(f);
        v = (vehicle*)stfs_get_vehicle(path);
        if (v == NULL) {
            LOG_MSG(error, "Failed to extract vehicle from STFS save.\n");
            return NULL;
        }
        LOG_MSG(debug, "Vehicle loaded from STFS save entry\n");
    }

    vehicle_header_byteswap(&v->head);
    for (u16 i = 0; i < v->head.part_count; i++) {
        part_byteswap(&v->parts[i]);
    }

    return v;
}
