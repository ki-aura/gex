#include <stdio.h>
#include <stdlib.h>
#include "khash.h"

/*
Function / Macro	Description
KHASH_MAP_INIT_INT(name, valtype)	Defines a complete integer-keyed hash table type 
					name with values of type valtype and all related macros/functions.
khash_t(name) *h	Declares a pointer h to a hash table of the type given name.
h = kh_init(name)	Allocate and initialize a new hash table; returns a pointer to the table
kh_destroy(name, h)	Free memory used by hash table h.

khiter_t k 		integer index to a slot within a table. Returned by functions that 
			modify the table or iterate / check on key existence Remember
			** SLOTS ARE PRE-ALLOCATED INTERNALLY AND ARE ALL INITIALLY EMPTY **
			
kh_put(name, h, key, &ret)	Insert new key (or get existing) and return its slot; 
				ret indicates if new (1), existing (0), or failure (-1).
				**NOTE nothing is overwritten at this point, you just have a 
				slot index and a return code that says if it's new or existing
kh_val(h, k)		Access the value at slot k. used to set or retrieve the value
kh_key(h, k)		read only lookup of a key; returns the slot index k. can NOT be used to update the key
kh_get(name, h, key)	read only Look up key; returns slot index or kh_end(h) if not found but 
			doesn't create one if it does exist (compare this to kh_put)
kh_del(name, h, k)	Delete the key/value at slot k.
kh_begin(h)		Returns the index of the first slot in the table (all slots may be empty).
kh_end(h)		Returns the index just past the last slot. (ie used as iter loop terminator)
kh_exist(h, k)		Returns true if slot k contains a valid key/value.
kh_size(h)		Returns the number of valid elements currently in the table.
kh_clear(name, h)	Remove all elements but keep table allocated (reset all slots to empty).

** NOTE FOR UNSIGNED LONGS **
KHASH_MAP_INIT_INT64(name, valtype) 	will create a type long enough to take unsigned longs, but
					INT64 is signed. this isn't a problem as KH doesn't care if 
					your key is negative, so casting from UL to INT64 isn't a
					problem. other funcs will need to look like this:
kh_get(name, h, (int64_t)UL_key)	casting the ul key to an int64
*/

// Initialize khash: integer key  → slots holding a single char
KHASH_MAP_INIT_INT(charmap, char)

// Comparison for qsort: by char value
int cmp_val(const void *a, const void *b) {
    const char *pa = (const char*)a;
    const char *pb = (const char*)b;
    return (*pa) - (*pb);
}

// Comparison for qsort: by int key
typedef struct {
	int key; 
	char val; 
} kv_t;

int cmp_key(const void *a, const void *b) {
    const kv_t *pa = (const kv_t*)a;
    const kv_t *pb = (const kv_t*)b;
    return pa->key - pb->key;
}

khash_t(charmap) *myhash;

void sortval(){
    // --- Extract entries for sorting by value ---
    // we don't bother even storing the key for each slot as not needed for sort
    char *v_array = malloc(kh_size(myhash) * sizeof(char));
    int idx = 0;
    khiter_t slot;
    for (slot = kh_begin(myhash); slot != kh_end(myhash); ++slot) {
        if (kh_exist(myhash, slot)) 
        	v_array[idx] = kh_val(myhash, slot);
        	idx++;
    }

    qsort(v_array, kh_size(myhash), sizeof(char), cmp_val);

    printf("Sorted by char value: ");
    for (int i = 0; i < kh_size(myhash); i++) printf("%c", v_array[i]);
    printf("\n");
    free(v_array);
}

void sortkey(){
    // --- Extract entries for sorting by key ---
    // this time we need the slot key and associated char value, so we use a struct kv_t
    kv_t *kv_array = malloc(kh_size(myhash) * sizeof(kv_t));
    int idx = 0;
    khiter_t slot;
    for (slot = kh_begin(myhash); slot != kh_end(myhash); ++slot) {
        if (kh_exist(myhash, slot)) {
            kv_array[idx].key = kh_key(myhash, slot);
            kv_array[idx].val = kh_val(myhash, slot);
            idx++;
        }
    }

    qsort(kv_array, kh_size(myhash), sizeof(kv_t), cmp_key);

    printf("Sorted by key: ");
    for (int i = 0; i < kh_size(myhash); i++) printf("%d:%c ", kv_array[i].key, kv_array[i].val);
    printf("\n");
    free(kv_array);
}

int main() 
{
	int keys[] = {3, 1, 5, 2, 4};        // random-ish order
	char vals[] = {'n', 'b', 'o', 'i', 'g'};
	int ret; // return code from kh functions

	// initialise myhash as the hash, giving it the data type held by each key
	myhash = kh_init(charmap);
	
	// Insert into hash
	for (int i = 0; i < 5; i++) {
		khiter_t slot = kh_put(charmap, myhash, keys[i], &ret);
		kh_val(myhash, slot) = vals[i];
	}

	// show original 5 keys sorted by value & then by key 
	sortval(); sortkey();
	
	// add some more keys
	khiter_t slot;
	slot = kh_put(charmap, myhash, 6, &ret);
	kh_val(myhash, slot) = 'a';
	sortval(); sortkey();
	
	slot = kh_put(charmap, myhash, 7, &ret);
	kh_val(myhash, slot) = 'z';
	sortval(); sortkey();
	
	// delete a key
	slot = kh_get(charmap, myhash, 4);
	if (slot != kh_end(myhash)) 
		kh_del(charmap, myhash, slot);
	sortval(); sortkey();

	// Cleanup
	kh_destroy(charmap, myhash);
	return 0;
}
