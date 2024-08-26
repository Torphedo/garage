#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "common/int.h"
#include "logging.h"
#include "list.h"

u8 list_element_size(list l) {
    return sizeof(*l.data);
}

u32 list_maxidx(list l) {
    return (l.alloc_size / list_element_size(l)) - 1;
}

bool list_full(list l) {
    return l.end_idx >= list_maxidx(l);
}
list list_create(u32 init_size) {
    return (list) {
        .data = calloc(1, init_size),
        .alloc_size = init_size,
    };
}

void list_add(list* l, list_element val) {
    // If there's no room, we need to realloc
    if (list_full(*l)) {
        // The buffer is completely full & needs a new allocation.
        // Grow by 50%, rounded up to the next multiple of our element size.
        const u32 newsize = ALIGN_UP((u32)(l->alloc_size * 1.5), list_element_size(*l));
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
    if (idx > l->end_idx || l->end_idx == 0) {
        // Caller wants to remove an element that isn't used...
        return;
    }

    // Overwrite the element to be deleted with the last one in the list. This
    // simultaneously removes the target data and shrinks the list.
    l->data[idx] = l->data[l->end_idx - 1];

    // Zero out the back element just in case
    l->data[l->end_idx] = 0;
    l->end_idx--; // Shrink the list
}

void list_remove_val(list* l, list_element val) {
    const s64 idx = list_find(*l, val);
    if (idx == -1) {
        return;
    }
    list_remove(l, idx);
}

s64 list_find(list l, list_element val) {
    for (u32 i = 0; i < l.end_idx; i++) {
        if (l.data[i] == val) {
            // Found it!
            return i;
        }
    }

    // Nothin...
    return -1;
}

bool list_contains(list l, list_element val) {
    return list_find(l, val) != -1;
}

void list_clear(list* l) {
    memset(l->data, 0x00, l->alloc_size);
    l->end_idx = 0;
}

bool list_empty(list l) {
    return (l.end_idx == 0);
}

