#include "common/endian.h"

#include "vehicle.h"

// TODO: Make this a no-op when compiling for a big endian system
void part_byteswap(part_entry* part) {
    ENDIAN_FLIP(u32, part->unknown);
    ENDIAN_FLIP(u32, part->id);
    part->rot = vec3f_byteswap(part->rot);
    ENDIAN_FLIP(u32, part->pad3);
}

void vehicle_header_byteswap(vehicle_header* v) {
    ENDIAN_FLIP(u64, v->magic);
    ENDIAN_FLIP(u16, v->part_count);
}

part_entry part_read(FILE* f) {
    part_entry part = {0};
    fread(&part, sizeof(part), 1, f);
    part_byteswap(&part);
    return part;
}
