#ifndef LIST_H
#define LIST_H
#include <stddef.h>
#include <stdbool.h>
#include "int.h"

typedef struct {
    // Base allocation & size
    // We use a uintptr_t to avoid dereferencing void* which is an error on some platforms
    uintptr_t data;
    u32 alloc_size;
    // Index of the next open slot in the array (not the last element!)
    u32 end_idx;
    u32 element_size; // Size of each array element
}list;

// Create a list.
// init_size is allocation size in bytes, please try to align to [element_size]
// The buffer is treated like an array, using [element_size] to index into it.
// list of u16 has element_size 2, list of vec3f has element_size 12, etc.
list list_create(u32 init_size, u32 element_size);

// Append an element to the list.
void list_add(list* l, const void* data);

// We're forced to have this getter function to keep the list structure generic
// Cast to your desired type and derefence.
void* list_get_element(list l, u32 idx);

// Reset to initial state and fill buffer with 0. Does not free buffer.
void list_clear(list* l);

// Remove an element from the list. Every element after it is moved backwards.
void list_remove(list* l, u32 idx);

// Find and remove the first occurance of a value from the list.
void list_remove_val(list* l, const void* data);

// Find the index of a value in the list. Returns -1 on failure.
s64 list_find(list l, const void* data);

// Whether the list contains a certain value
bool list_contains(list l, const void* data);

// Whether the list is empty
bool list_empty(list l);

#endif // #ifndef LIST_H

