#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <stddef.h> // For size_t (used for heap size)
#include <stdbool.h> // For the boolean return type

// Define the function pointer type for the user-provided comparison.
// This function determines the priority of the items in the heap.
typedef int (*min_heap_cmp_func)(const void *a, const void *b);
//
// Comparison Rule for a **Min-Heap**:
// - Returns: < 0 if **a** has higher priority (a "smaller" value) than **b**.
// - Returns: > 0 if **a** has lower priority (a "larger" value) than **b**.
// - Returns: 0 if **a** and **b** are considered equal in priority.

// The opaque structure for the Generic Min-Heap Library (GML).
// The actual members of this structure are hidden from the user and defined in `min_heap.c`.
typedef struct GML_Heap GML_Heap;

// --- GML PUBLIC API ---

// Initializes a new, empty generic min-heap instance.
// The required parameter is the user's item comparison function.
// Returns: A pointer to the initialized GML_Heap structure, or NULL on failure.
GML_Heap *gml_init(min_heap_cmp_func compare_func);

// Cleans up and frees all internal memory and structures used by the heap.
// NOTE: This does *not* free the application-specific data pointers stored in the heap.
void gml_cleanup(GML_Heap *h);

// Inserts an item (a generic void pointer) into the heap.
// The heap automatically places the pointer based on its priority (O(log N)).
// Returns: 0 on success, or -1 on memory allocation failure.
int gml_push(GML_Heap *h, void *item);

// Extracts (removes) and returns the item with the highest priority (the minimum value).
// The item is removed from the heap array and the index map (O(log N)).
// Returns: The highest-priority item pointer, or NULL if the heap is empty.
void *gml_pop(GML_Heap *h);

// Removes an **arbitrary** item from the heap using its pointer value.
// This operation uses the internal hash map for an efficient index lookup (O(1) average)
// followed by a heap fix-up (O(log N)).
// Returns: true if the item was found and successfully deleted, false otherwise.
bool gml_delete(GML_Heap *h, void *item);

// Returns the current number of items (pointers) stored in the heap.
size_t gml_size(const GML_Heap *h);

// Returns the highest priority item (the root) **without** removing it from the heap.
// Returns: The highest-priority item pointer, or NULL if the heap is empty.
void *gml_peek(const GML_Heap *h);

#endif // MIN_HEAP_H