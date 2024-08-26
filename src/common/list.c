#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "common/int.h"
#include "logging.h"
#include "list.h"

u32 list_maxidx(list l) {
    return (l.alloc_size / l.element_size) - 1;
}

bool list_full(list l) {
    return l.end_idx >= list_maxidx(l);
}

void* list_get_element(list l, u32 idx) {
    return (void*)(l.data + (idx * l.element_size));
}

list list_create(u32 init_size, u32 element_size) {
    return (list) {
        .element_size = element_size,
        .data = (uintptr_t)calloc(1, init_size),
        .alloc_size = init_size,
    };
}

void list_add(list* l, void* data) {
    // If there's no room, we need to realloc
    if (list_full(*l)) {
        // The buffer is completely full & needs a new allocation.
        // Grow by 50%, rounded up to the next multiple of our element size.
        const u32 newsize = ALIGN_UP((u32)(l->alloc_size * 1.5), l->element_size);
        void* newbuf = calloc(1, newsize);
        if (newbuf == NULL) {
            LOG_MSG(error, "Couldn't expand list 0x%X -> 0x%X [alloc failure]\n", l->alloc_size, newsize);
            return;
        }

        // Copy data & update state
        memcpy(newbuf, (void*)l->data, l->alloc_size);
        free((void*)l->data);
        l->data = (uintptr_t)newbuf;
        l->alloc_size = newsize;
    }

    // Put value in the next slot. Sorry it's kinda verbose
    void* next_slot = list_get_element(*l, l->end_idx++);
    memcpy(next_slot, data, l->element_size);
}

void list_remove(list* l, u32 idx) {
    if (idx > l->end_idx || l->end_idx == 0) {
        // Caller wants to remove an element that isn't used...
        return;
    }

    // Overwrite the element to be deleted with the last one in the list. This
    // simultaneously removes the target data and shrinks the list.
    void* back_element = list_get_element(*l, l->end_idx - 1);
    void* target_element = list_get_element(*l, idx);
    memcpy(target_element, back_element, l->element_size);

    // Zero out the back element just in case
    memset(back_element, 0x00, l->element_size);
    l->end_idx--; // Shrink the list
}

void list_remove_val(list* l, void* data) {
    const s64 idx = list_find(*l, data);
    if (idx == -1) {
        return;
    }
    list_remove(l, idx);
}

s64 list_find(list l, void* data) {
    for (u32 i = 0; i < l.end_idx; i++) {
        void* element = list_get_element(l, i);
        if (memcmp(data, element, l.element_size) == 0) {
            // Found it!
            return i;
        }
    }

    // Nothin...
    return -1;
}

bool list_contains(list l, void* data) {
    return list_find(l, data) != -1;
}

void list_clear(list* l) {
    memset((void*)l->data, 0x00, l->alloc_size);
    l->end_idx = 0;
}

bool list_empty(list l) {
    return (l.end_idx == 0);
}

