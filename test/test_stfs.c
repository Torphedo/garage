#include <common/logging.h>

#include <stfs.h>

#include "testing.h"

const char* stfs_path = "../bin/explore_2.bin";

#define TEST_EXIT(file, result) fclose(f); REPORT_RESULT(result); return result

bool test_stfs() {
    bool result = false;
    stfs_header head = {0};
    FILE* f = fopen(stfs_path, "rb");
    if (f == NULL) {
        LOG_MSG(error, "Couldn't open STFS test file \"%s\"\n", stfs_path);
        REPORT_RESULT(result);
        return result;
    }

    fread(&head, sizeof(head), 1, f);
    stfs_header_byteswap(&head);
    u32 first_off = stfs_first_block_off(&head);
    if (first_off > 0xA000) {
        LOG_MSG(warning, "Header is too large (0x%X), first block offset is 0x%X\n", head.meta.header_size, first_off);
        TEST_EXIT(f, result);
    }
    if (first_off % STFS_BLOCK_SIZE != 0) {
        LOG_MSG(error, "Block offset 0x%X isn't a multiple of block size!\n", first_off);
        TEST_EXIT(f, result);
    }

    // TODO: Test extracting the file data and make sure endian is flipped correctly for all the right fields.
    result = true;
    TEST_EXIT(f, result);
}
