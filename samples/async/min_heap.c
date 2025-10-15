#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "min_heap.h"
// Include the lightweight hash table library (khashl) for pointer-to-index mapping.
#include "khashl.h"


// Define KHASHL_INIT macros here to create the map
// Key: void* (item pointer), Value: khint_t (array index)
// Macro to hash the pointer address as a 64-bit integer.
#define gmli_hash_ptr(p) kh_hash_uint64((khint64_t)p)
// Macro for pointer equality comparison.
#define gmli_eq_ptr(a, b) ((a) == (b))
#define gmli_hash_ptr(p) kh_hash_uint64((khint64_t)p)
#define gmli_eq_ptr(a, b) ((a) == (b))

// Declare a type-safe wrapper for the hash map to track item indices
// KHASHL_MAP_INIT(SCOPE, HType, prefix, khkey_t, kh_val_t, __hash_fn, __hash_eq)
// Creates static map functions prefixed with 'gmli' to map a 'void*' item pointer (key)
// to its current array index (khint_t) within the heap data structure (value).
KHASHL_MAP_INIT(static, kh_gml, gmli, void*, khint_t, gmli_hash_ptr, gmli_eq_ptr)

// The internal structure for the Generic Min-Heap (GML stands for Generic Min-Heap List).
struct GML_Heap {
	// Dynamic array (vector) to store the void* item pointers (the heap data structure).
	void **data;
	size_t size; // Current number of items in the heap.
	size_t capacity; // Current allocated size of the 'data' array.
	// Function pointer provided by the user to compare two heap items.
	min_heap_cmp_func cmp;
	// Auxiliary hash map for O(1) average time lookup of an item's array index,
	// which is critical for O(log N) arbitrary deletion.
	kh_gml *idx_map;
};

// Internal utility function to swap two items in the heap array at indices i and j.
static void gmli_swap(GML_Heap *h, khint_t i, khint_t j) {
	// Standard array element swap.
	void *temp = h->data[i];
	h->data[i] = h->data[j];
	h->data[j] = temp;

	// CRITICAL: Update the index map for both items after they move.
	// This maintains the consistency between the array and the map.
	khint_t *v;
	khint_t k;

	// Item at array index 'j' moved to 'i'. Update its index in the map.
	k = gmli_get(h->idx_map, h->data[i]);
	if (k != kh_end(h->idx_map)) {
		v = &kh_val(h->idx_map, k);
		*v = i; // Store the new array index 'i'.
	}

	// Item at array index 'i' moved to 'j'. Update its index in the map.
	k = gmli_get(h->idx_map, h->data[j]);
	if (k != kh_end(h->idx_map)) {
		v = &kh_val(h->idx_map, k);
		*v = j; // Store the new array index 'j'.
	}
}

// Internal function to maintain the heap property by moving the item at 'i' up the tree.
// Used after insertion or when an item's priority increases (not used in this demo).
static void gmli_sift_up(GML_Heap *h, khint_t i) {
	khint_t p; // Index of the parent.
	while (i > 0) {
		p = (i - 1) / 2; // Calculate parent index.
		// Check if parent (p) has higher priority (smaller value from comparison) than child (i).
		if (h->cmp(h->data[p], h->data[i]) < 0) {
			break; // Heap property satisfied: parent is smaller than child.
		}
		// If not, swap child and parent.
		gmli_swap(h, i, p);
		i = p; // Move up to the parent's old position.
	}
}

// Internal function to maintain the heap property by moving the item at 'i' down the tree.
// Used after removal (pop or delete) or when an item's priority decreases (not used in this demo).
static void gmli_sift_down(GML_Heap *h, khint_t i) {
	khint_t l, r, smallest;
	while (1) {
		l = 2 * i + 1; // Left child index.
		r = l + 1; // Right child index.
		smallest = i; // Assume current node is the smallest.

		// Check left child: if it exists and has higher priority than 'smallest'.
		if (l < h->size && h->cmp(h->data[l], h->data[smallest]) < 0) {
			smallest = l;
		}

		// Check right child: if it exists and has higher priority than 'smallest' (which might be 'l').
		if (r < h->size && h->cmp(h->data[r], h->data[smallest]) < 0) {
			smallest = r;
		}

		if (smallest == i) {
			break; // Heap property satisfied: current node is the smallest of itself and its children.
		}
		// Swap the current node with the smallest child.
		gmli_swap(h, i, smallest);
		i = smallest; // Move down to the smallest child's old position.
	}
}

// Internal function to increase the heap array capacity if the new_size exceeds it.
static int gmli_reserve(GML_Heap *h, size_t new_size) {
	if (new_size <= h->capacity) return 0; // Enough capacity, return success.

	// Calculate new capacity (doubling strategy or to at least new_size).
	size_t new_cap = h->capacity == 0 ? 16 : h->capacity * 2;
	if (new_cap < new_size) new_cap = new_size;

	// Reallocate the underlying array.
	void **new_data = realloc(h->data, new_cap * sizeof(void*));
	if (!new_data) return -1; // Reallocation failed.

	h->data = new_data;
	h->capacity = new_cap;
	return 0;
}

// --- GML PUBLIC API IMPLEMENTATION ---

// Allocates and initializes a new GML_Heap structure.
GML_Heap *gml_init(min_heap_cmp_func compare_func) {
	if (!compare_func) return NULL; // Comparator is required.

	// Allocate and zero-initialize the main heap structure.
	GML_Heap *h = calloc(1, sizeof(GML_Heap));
	if (!h) return NULL;

	h->cmp = compare_func;
	// Initialize the khash map for index tracking using the 'gmli' functions.
	h->idx_map = gmli_init();
	if (!h->idx_map) {
		free(h);
		return NULL;
	}

	return h;
}

// Frees all memory associated with the GML_Heap structure.
void gml_cleanup(GML_Heap *h) {
	if (!h) return;
	// Free the data array (the array of item pointers).
	free(h->data);
	// Destroy and free the khash map structure.
	gmli_destroy(h->idx_map);
	// Free the main heap structure.
	free(h);
}

// Inserts a new item into the heap (O(log N)).
int gml_push(GML_Heap *h, void *item) {
	if (!h || !item) return -1;
	// Ensure capacity is sufficient.
	if (gmli_reserve(h, h->size + 1) != 0) return -1;

	// 1. Insert the new item at the next available position (the end of the array).
	h->data[h->size] = item;

	// 2. Add the item pointer and its current array index to the index map.
	int absent;
	// Find or insert the item into the map.
	khint_t k = gmli_put(h->idx_map, item, &absent);
	if (absent) {
		// If it's a new item, store its array index 'h->size'.
		kh_val(h->idx_map, k) = h->size;
	} else {
		// If the pointer was already in the map (a push error), overwrite the old index.
		fprintf(stderr, "Warning: Pushing duplicate item pointer to heap. Overwriting index.\n");
		kh_val(h->idx_map, k) = h->size;
	}

	// 3. Restore the heap property by moving the new item up from the bottom.
	gmli_sift_up(h, h->size);
	h->size++; // Increment the item count.

	return 0;
}

// Removes and returns the highest priority item (the root) (O(log N)).
void *gml_pop(GML_Heap *h) {
	if (!h || h->size == 0) return NULL;

	void *root_item = h->data[0];

	// 1. Remove the root item from the index map.
	khint_t k = gmli_get(h->idx_map, root_item);
	if (k != kh_end(h->idx_map)) {
		gmli_del(h->idx_map, k);
	}

	h->size--;
	if (h->size > 0) {
		// 2. Move the last item in the array to the root position.
		h->data[0] = h->data[h->size];

		// CRITICAL: Update the index map entry for the item that just moved to the root (index 0).
		khint_t k_new_root = gmli_get(h->idx_map, h->data[0]);
		if (k_new_root != kh_end(h->idx_map)) {
			kh_val(h->idx_map, k_new_root) = 0;
		}

		// 3. Restore the heap property by moving the new root item down.
		gmli_sift_down(h, 0);
	}

	return root_item;
}

// Deletes an arbitrary item from the heap using its pointer (O(log N) average).
bool gml_delete(GML_Heap *h, void *item) {
	if (!h || h->size == 0) return false;

	// 1. Find the item's current index 'i' using the hash map (O(1) average lookup).
	khint_t k = gmli_get(h->idx_map, item);
	if (k == kh_end(h->idx_map)) {
		return false; // Item not found.
	}
	khint_t i = kh_val(h->idx_map, k);

	// 2. Remove the item from the index map.
	gmli_del(h->idx_map, k);

	h->size--;
	if (i == h->size) {
		// Case 1: The item was the last one in the array. Deletion is complete.
		return true;
	}

	// Case 2: Item is in the middle. Replace it with the very last item in the array.
	h->data[i] = h->data[h->size];

	// CRITICAL: Update the index map for the item that was moved into position 'i'.
	khint_t k_moved = gmli_get(h->idx_map, h->data[i]);
	if (k_moved != kh_end(h->idx_map)) {
		kh_val(h->idx_map, k_moved) = i;
	}

	// 3. Restore the heap property (O(log N)).
	// The moved item at 'i' must be sifted up *or* down.
	// Check for a violation up (i.e., the item is smaller than its parent).
	if (i > 0) {
		khint_t p = (i - 1) / 2;
		if (h->cmp(h->data[p], h->data[i]) > 0) {
			gmli_sift_up(h, i); // If violation, move up and exit.
			return true;
		}
	}
	// If no violation upwards (or if it's the root), check for a violation down.
	gmli_sift_down(h, i);

	return true;
}

// Returns the current number of items in the heap.
size_t gml_size(const GML_Heap *h) {
	return h ? h->size : 0;
}

// Returns a pointer to the highest priority item (the root) without removing it.
void *gmli_peek(const GML_Heap *h) {
	if (!h || h->size == 0) return NULL;
	return h->data[0];
}