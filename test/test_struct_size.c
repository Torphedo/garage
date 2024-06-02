#include <vehicle.h>
#include <stfs.h>
#include <common/logging.h>

#include "testing.h"

bool size_check(char* str, u32 size, u32 target_size) {
    if (size != target_size) {
        LOG_MSG(error, "sizeof(%s) == 0x%X, expected 0x%X\n", str, size, target_size);
        return false;
    }
    return true;
}

bool test_struct_size() {
    // Vehicle structures
    bool result = size_check("vehicle_header", sizeof(vehicle_header), 0x80);
    result |= size_check("part_entry", sizeof(part_entry), 0x24);
    result |= size_check("vehicle_bitmask", sizeof(vehicle_bitmask), 0x200000);

    // STFS structures
    result |= size_check("stfs_header", sizeof(stfs_header), 0x971A);
    result |= size_check("stfs_vol_desc", sizeof(stfs_vol_desc), 0x23);
    result |= size_check("stfs_license", sizeof(stfs_license), 0x10);
    result |= size_check("stfs_hash_table", sizeof(stfs_hash_table), 0x18);
    result |= size_check("stfs_filetable", sizeof(stfs_filetable), 0x40);

    REPORT_RESULT(result);
    return result;
}
