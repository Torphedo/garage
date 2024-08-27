#include <common/logging.h>
#include <common/int.h>
#include <common/list.h>

#include "testing.h"

#define TEST_EXIT(file, result) fclose(f); REPORT_RESULT(result); return result

bool test_list() {
    bool result = true;

    // Test basic list creation
    const u32 alloc_size = 4;
    list l = list_create(alloc_size, sizeof(u16));
    if (l.end_idx != 0) {
        printf("CREATE: end idx init wrong!\n");
        result = false;
    }
    if (l.alloc_size != alloc_size) {
        printf("CREATE: alloc size init wrong!\n");
        result = false;
    }
    if (l.element_size != sizeof(u16)) {
        printf("CREATE: element size init wrong!\n");
        result = false;
    }

    // Test list add, make sure it increments list end idx and doesn't realloc
    const u16 val = 3;
    list_add(&l, &val);
    if (l.end_idx != 1) {
        printf("ADD: end_idx not incremented correctly!\n");
        result = false;
    }
    if (*(u16*)list_get_element(l, 0) != val) {
        printf("ADD: element not inserted correctly!\n");
        result = false;
    }
    if(l.alloc_size != 4) {
        printf("ADD: unnecessary realloc!\n");
        result = false;
    }

    for (u8 i = 0; i < 5; i++) {
        const u16 loopval = i * 3;
        list_add(&l, &loopval);
    }
    if (l.alloc_size <= 4) {
        printf("ADD: didn't realloc properly!\n");
        result = false;
    }
    if (l.end_idx != 6) {
        printf("ADD: end_idx not incremented correctly!\n");
        result = false;
    }

    if (list_find(l, &val) == -1) {
        printf("FIND: Couldn't find a value that's definitely there!\n");
        result = false;
    }

    const u16 removed_val = 15;
    list_remove_val(&l, &removed_val);
    if (list_contains(l, &removed_val)) {
        printf("REMOVE: Value that should've been removed is still there!\n");
        result = false;
    }

    REPORT_RESULT(result);
    return result;
}
