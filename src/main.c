#include <stdio.h>
#include <malloc.h>

#include "common/int.h"
#include "common/endian.h"
#include "common/logging.h"

#include "vehicle.h"
#include "part_ids.h"
#include "stfs.h"

int main(int argc, char** argv) {
    enable_win_ansi();
    check_part_entry(); // Sanity check for padding bugs
    if (argc != 2) {
        LOG_MSG(error, "No input files.\n");
        LOG_MSG(info, "Usage: vtweak [vehicle file]");
        return 1;
    }
    char* path = argv[1];
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        LOG_MSG(error, "Failed to open %s\n", path);
        return 1;
    }

    u8* vehicle = NULL;
    vehicle_header head = {0};
    fread(&head, sizeof(head), 1, f);
    vehicle_header_byteswap(&head);
    if (head.magic == VEHICLE_MAGIC) {
        u32 size = file_size(path);
        vehicle = calloc(1, size);
        if (vehicle == NULL) {
            LOG_MSG(error, "Couldn't alloc %d bytes for raw vehicle file\n", size);
            return 1;
        }
        fseek(f, 0, SEEK_SET);
        fread(vehicle, size, 1, f);
        LOG_MSG(debug, "Vehicle loaded from raw save file\n");
    }
    else {
        fclose(f);
        vehicle = stfs_get_vehicle(path);
        if (vehicle == NULL) {
            LOG_MSG(error, "Failed to extract vehicle from STFS save.\n");
            return 1;
        }
        LOG_MSG(debug, "Vehicle loaded from STFS save entry\n");
    }
    head = *(vehicle_header*)vehicle;
    ENDIAN_FLIP(u16, head.part_count);
    part_entry* parts = (part_entry *)(((u8*)vehicle) + PART_ENTRIES_OFFSET);

    LOG_MSG(info, "Vehicle has %d parts\n", head.part_count);
    for (u16 i = 0; i < head.part_count; i++) {
        part_byteswap(&parts[i]);
        LOG_MSG(info, "id 0x%x [%s] ", parts[i].id, part_get_name(parts[i].id));
        printf("@ (%d, %d, %d) ", parts[i].pos.x, parts[i].pos.y, parts[i].pos.z);
        printf("painted #%x%x%x", parts[i].color.r, parts[i].color.g, parts[i].color.b);
        printf(", modifier 0x%02hx\n", parts[i].modifier);
    }

    fclose(f);
    return 0;
}
