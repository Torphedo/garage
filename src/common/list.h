#ifndef LIST_H
#define LIST_H
#include <stddef.h>
#include <stdbool.h>
#include "int.h"

typedef struct {
    // Base allocation & size
    // TODO: Consider making this structure more generic
    u16* data; // Change pointer type to change type/size of the array elements
    u32 alloc_size;
    // Index of the next open slot in the array
    u32 end_idx;
}list;

// Create a list.
// init_size is the initial allocation, please try to align it to sizeof(*data)
list list_create(u32 init_size);

// Append an element to the list.
void list_add(list* l, u16 val);

// Remove an element from the list. Every element after it is moved backwards.
void list_remove(list* l, u32 idx);

// Reset to initial state and fill buffer with 0. Does not free buffer.
void list_clear(list* l);

// Whether the list contains a certain value
bool list_contains(const list* l, u16 val);

// Whether the list is empty
bool list_empty(const list* l);

#endif // #ifndef LIST_H

