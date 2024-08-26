#ifndef LIST_H
#define LIST_H
#include <stddef.h>
#include <stdbool.h>
#include "int.h"

typedef u16 list_element; // Change this to change type/size of array elements
typedef struct {
    // Base allocation & size
    // TODO: Consider making this structure more generic
    list_element* data;
    u32 alloc_size;
    // Index of the next open slot in the array
    u32 end_idx;
}list;

// Create a list.
// init_size is the initial allocation, please try to align it to sizeof(*data)
list list_create(u32 init_size);

// Append an element to the list.
void list_add(list* l, list_element val);

// Remove an element from the list. Every element after it is moved backwards.
void list_remove(list* l, u32 idx);

// Find and remove the first occurance of a value from the list.
void list_remove_val(list* l, list_element val);

// Find the index of a value in the list. Returns -1 on failure.
s64 list_find(list l, list_element val);

// Reset to initial state and fill buffer with 0. Does not free buffer.
void list_clear(list* l);

// Whether the list contains a certain value
bool list_contains(list l, list_element val);

// Whether the list is empty
bool list_empty(list l);

#endif // #ifndef LIST_H

