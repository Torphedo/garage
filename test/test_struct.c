#include <vehicle.h>
#include <stfs.h>
#include <common/logging.h>

#include "testing.h"

bool size_check(char* str, u32 size, u32 target_size) {
    if (size != target_size) {
        LOG_MSG(error, "sizeof(%s) == 0x%X, expected 0x%X\n", str, size, target_size);
        return true;
    }
    return false;
}

int test_struct() {
    bool fail = size_check("vehicle_header", sizeof(vehicle_header), 0x80);
    fail |= size_check("part_entry", sizeof(part_entry), 0x24);
    fail |= size_check("vehicle_bitmask", sizeof(vehicle_bitmask), 0x200000);
    fail |= size_check("stfs_header", sizeof(stfs_header), 0x971A);

    return fail ? TEST_FAIL : TEST_PASS;
}

