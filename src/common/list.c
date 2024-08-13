#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "logging.h"
#include "list.h"

u32 list_maxidx(list* l) {
    return (l->alloc_size / sizeof(*l->data)) - 1;
}

bool list_full(list* l) {
    return l->end_idx >= list_maxidx(l);
}

list list_create(u32 init_size) {
    return (list) {
        .data = calloc(1, init_size),
        .alloc_size = init_size,
    };
}

void list_add(list* l, u16 val) {
    // If there's no room, we need to realloc
    if (list_full(l)) {
        // The buffer is completely full & needs a new allocation.
        // Grow by 50%, rounded up to the next multiple of our data size.
        u32 newsize = (l->alloc_size * 1.5);
        newsize += sizeof(*l->data) - (newsize % sizeof(*l->data));
        void* newbuf = calloc(1, newsize);
        if (newbuf == NULL) {
            LOG_MSG(error, "Couldn't expand list 0x%X -> 0x%X [alloc failure]\n", l->alloc_size, newsize);
            return;
        }

        // Copy data & update state
        memcpy(newbuf, l->data, l->alloc_size);
        free(l->data);
        l->data = newbuf;
        l->alloc_size = newsize;
    }

    // Put value in the next slot
    l->data[l->end_idx++] = val;
}

void list_remove(list* l, u32 idx) {
    if (idx >= l->end_idx) {
        // Caller wants to remove an element that isn't used...
        return;
    }

    l->end_idx--;
    if (idx == l->end_idx) {
        // We're removing the last element, no memmove() needed.
        l->data[idx] = 0;
        return;
    }

    // Shift the rest of the list back to fill the empty space
    u32 size = (l->end_idx - idx) * sizeof(*l->data);
    memmove(&l->data[idx], &l->data[idx + 1], size);
}

bool list_contains(list* l, u16 val) {
    for (u32 i = 0; i < l->end_idx; i++) {
        if (l->data[i] == val) {
            return true;
        }
    }

    return false;
}

void list_clear(list* l) {
    memset(l->data, 0x00, l->alloc_size);
    l->end_idx = 0;
}

bool list_empty(list* l) {
    return (l->end_idx == 0);
}

