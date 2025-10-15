#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
// Include the interface for the generic minimum heap implementation.
#include "min_heap.h"
// Include the library for the lightweight hash table (khashl).
#include "khashl.h"

// --- APPLICATION-SPECIFIC TYPEDEFS ---

// Define the function signature for a scheduled task.
// It receives the *scheduled* time, the *actual* run time, and a *context* pointer.
typedef void (*task_func)(struct timespec *scheduled, struct timespec *actual, void *ctx);

// Custom context structure for the 'task_generic_wizard' function.
typedef struct {
	char *message;
	struct timespec last_run_time; // Stores the time of the previous execution.
} WizContext;

// The main structure defining a scheduled unit of work (a Task).
// This is the item stored within the minimum heap.
typedef struct {
	long task_id; // Unique ID used for external lookup (deletion/modification).
	// next_run is the key for priority: the earlier the time, the higher the priority.
	struct timespec next_run;
	task_func func; // The function to execute when the task runs.
	void *ctx; // Pointer to application-specific data (WizContext in this demo).
	long interval_ms; // The period for recurring tasks in milliseconds.
} Task;


// 1. Define the ID-to-Pointer map structure
// Key: long (Task ID), Value: Task* (Pointer to the heap item)
// Hashing and equality functions for 'long' keys.
#define id_to_ptr_hash(id) kh_hash_uint64((uint64_t)id)
#define id_to_ptr_eq(a, b) ((a) == (b))

// 2. Declare the map instance
// Creates type definitions and functions for a hash map (kh_id_ptr)
// that maps 'long' (ID) to 'Task*' (pointer to the task object in the heap).
KHASHL_MAP_INIT(static, kh_id_ptr, idp, long, Task*, id_to_ptr_hash, id_to_ptr_eq)
// A static pointer to the hash map object.
static kh_id_ptr *task_id_map;


// --- UTILITY FUNCTIONS ---

// POSIX-compliant timespec comparison.
// This function is passed to the generic min-heap to define task priority.
// Returns: < 0 if task A's next_run is earlier (higher priority) than task B's.
// Returns: > 0 if task B's next_run is earlier (higher priority) than task A's.
// Returns: 0 if the times are identical.
static int task_compare(const void *a, const void *b) {
	const Task *tA = (const Task *)a;
	const Task *tB = (const Task *)b;

	// Compare seconds first
	if (tA->next_run.tv_sec < tB->next_run.tv_sec) return -1;
	if (tA->next_run.tv_sec > tB->next_run.tv_sec) return 1;

	// Seconds are equal, compare nanoseconds
	if (tA->next_run.tv_nsec < tB->next_run.tv_nsec) return -1;
	if (tA->next_run.tv_nsec > tB->next_run.tv_nsec) return 1;

	return 0; // Times are exactly equal
}

// Adds a duration in milliseconds (ms) to a struct timespec *t*.
// Handles nanosecond overflow into seconds.
static void timespec_add_ms(struct timespec *t, long ms) {
	t->tv_sec += ms / 1000;
	t->tv_nsec += (ms % 1000) * 1000000;
	if (t->tv_nsec >= 1000000000) {
		t->tv_sec++;
		t->tv_nsec -= 1000000000;
	}
}

// Calculates the time difference between timespec *a* and *b* in seconds (double).
// Result is positive if *b* is later than *a*.
static double timespec_diff(struct timespec *a, struct timespec *b) {
	return (b->tv_sec - a->tv_sec) + (b->tv_nsec - a->tv_nsec) / 1e9;
}

// Uses POSIX nanosleep to sleep until the *target* time is reached.
static void sleep_until(const struct timespec *target) {
	struct timespec now;
	// Get the current monotonic time.
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) { perror("clock_gettime"); return; }
	struct timespec delta;
	// Calculate the difference (time to wait).
	delta.tv_sec = target->tv_sec - now.tv_sec;
	delta.tv_nsec = target->tv_nsec - now.tv_nsec;
	// Normalize nanoseconds if negative.
	if (delta.tv_nsec < 0) {
		delta.tv_sec--;
		delta.tv_nsec += 1000000000;
	}
	// If the target time is already past, return immediately.
	if (delta.tv_sec < 0) return;
	// Perform the sleep.
	nanosleep(&delta, NULL);
}

// --- TASK FUNCTIONS ---

// A simple task that prints the difference between scheduled and actual run time.
void task_timer(struct timespec *scheduled, struct timespec *actual, void *ctx) {
	double diff = timespec_diff(scheduled, actual);
	printf("Timer running: Scheduled=%ld.%09ld, Actual=%ld.%09ld, Diff=%+.6f s\n",
		scheduled->tv_sec, scheduled->tv_nsec,
		actual->tv_sec, actual->tv_nsec,
		diff);
}

// A demonstration task with a static counter and last run time.
void task_snape(struct timespec *scheduled, struct timespec *actual, void *ctx) {
	static int counter = 0;
	static struct timespec last_run = {0, 0};
	struct timespec now;
	// Get current time for calculating actual interval.
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) { perror("clock_gettime"); return; }
	double seconds_since_last_run;
	// Calculate interval since last run if 'last_run' is initialized.
	if (last_run.tv_sec != 0 || last_run.tv_nsec != 0)
		seconds_since_last_run = timespec_diff(&last_run, &now);
	else
		seconds_since_last_run = 0;

	// Print different messages based on the internal counter.
	switch(counter){
		case 0: printf("      Snape(%.1f)\n", seconds_since_last_run); break;
		case 1: printf("      Snape(%.1f)\n", seconds_since_last_run); break;
		case 2: printf("      Severus Snape(%.1f)\n", seconds_since_last_run); break;
		default: break;
	}
	counter++;
	// Reset counter after 3 prints.
	counter = counter == 4 ? 0 : counter;
	// Update the static 'last_run' time.
	last_run = now;
}

// A task that uses its custom context to display a message and last run time.
void task_generic_wizard(struct timespec *scheduled, struct timespec *actual, void *ctx) {
	// Cast the void* context to the specific WizContext structure.
	WizContext *self_ctx = (WizContext *)ctx;
	// The 'actual' run time is passed to the function.
	struct timespec now = *actual;
	double seconds_since_last_run;

	// Calculate and print the interval since the last time this specific task ran.
	if (self_ctx->last_run_time.tv_sec != 0 || self_ctx->last_run_time.tv_nsec != 0) {
		seconds_since_last_run = timespec_diff(&self_ctx->last_run_time, &now);
		printf("%s (%.2f)\n",
			self_ctx->message,
			seconds_since_last_run);
	} else {
		// First run.
		printf("%s (0.00)\n", self_ctx->message);
	}
	// Update the task's context with the current run time.
	self_ctx->last_run_time = now;
}

// --- TASK CREATION (Wrapper using GML) ---

// Creates, initializes, and adds a new Task to the scheduler.
// Returns a pointer to the Task object, or NULL on failure.
Task *add_task(long add_task_id, GML_Heap *heap, task_func func, long initial_delay_ms,
				long interval_ms, const char *msg) {
	struct timespec now;

	// Get the current time to calculate the first run time.
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) { perror("clock_gettime"); return NULL; }

	// Allocate memory for the main Task structure.
	Task *t = malloc(sizeof(Task));
	if (!t) { fprintf(stderr, "Failed to allocate Task\n"); return NULL; }

	// Allocate and initialize the custom context structure (WizContext) if a message is provided.
	WizContext *w_ctx = NULL;
	if (msg) {
		w_ctx = malloc(sizeof(WizContext));
		if (!w_ctx) {
			fprintf(stderr, "Failed to allocate WizContext\n");
			free(t);
			return NULL;
		}
		// Duplicate the message string for independent storage.
		w_ctx->message = strdup(msg);
		// Initialize the last run time to zero.
		w_ctx->last_run_time = (struct timespec){0, 0};
	}

	// Initialize the Task object fields.
	t->task_id = add_task_id;
	t->func = func;
	t->ctx = w_ctx;
	t->interval_ms = interval_ms;

	// Calculate the time for the first execution: current time + initial delay.
	t->next_run = now;
	timespec_add_ms(&t->next_run, initial_delay_ms);

	// Use the generic heap's push function to insert the task.
	if (gml_push(heap, t) != 0) {
		fprintf(stderr, "Failed to push task to heap.\n");
		// Clean up the allocated memory on failure.
		free(t->ctx);
		free(t);
		return NULL;
	}

	// Store the mapping from Task ID to the Task pointer in the external hash map.
	int absent;
	// Attempt to insert the task ID into the map.
	khint_t k = idp_put(task_id_map, t->task_id, &absent);
	if (absent) {
		// If the ID was successfully inserted (is unique), store the Task pointer as the value.
		kh_val(task_id_map, k) = t;
	} else {
		// This block handles a duplicate ID (shouldn't happen with a proper ID generator).
		fprintf(stderr, "Error: Duplicate task ID in ID map.\n");
	}

	// Return the newly created Task object pointer.
	return t;
}

// --- CLEANUP (Wrapper using GML) ---

// Frees all memory associated with a single Task object.
void destroy_task(Task *t) {
	// 1. Free the custom context data (e.g., the duplicated message string).
	if (t->ctx != NULL) {
		WizContext *w_ctx = (WizContext *)t->ctx;
		if (w_ctx->message) {
			free(w_ctx->message);
		}
		// Free the WizContext structure itself.
		free(w_ctx);
	}
	// 2. Free the main Task structure itself.
	free(t);
}

// Cleans up all tasks remaining in the heap and the heap structure itself.
void cleanup_tasks(GML_Heap *heap) {
	Task *t;
	// Repeatedly pop the highest priority task until the heap is empty.
	while ((t = gml_pop(heap)) != NULL) {
		// Call the application-specific cleanup for the task data.
		destroy_task(t);
	}
	// Finally, clean up the memory managed by the generic heap library.
	gml_cleanup(heap);
	// NOTE: The ID map (task_id_map) cleanup is handled elsewhere (main or an explicit function).
}

// Attempts to find, remove, and clean up a task using its unique ID.
// Returns true on successful deletion, false otherwise.
bool delete_task_by_id(GML_Heap *heap, long task_id) {
	// 1. Lookup the Task pointer using the external ID map.
	khint_t k = idp_get(task_id_map, task_id);
	// Check if the key was not found.
	if (k == kh_end(task_id_map)) {
		return false; // Task ID not found in the map.
	}
	// Retrieve the pointer to the Task object.
	Task *task_ptr = kh_val(task_id_map, k);

	// 2. Call the generic heap's pointer-based deletion function.
	if (gml_delete(heap, task_ptr)) {
		// Deletion from the heap was successful.
		// 3. Remove the entry from the external ID map.
		idp_del(task_id_map, k);

		// 4. Clean up the application-specific data.
		destroy_task(task_ptr);

		return true;
	}

	// Deletion from the heap failed (e.g., the pointer wasn't actually in the heap).
	return false;
}

// --- SCHEDULER (Main) ---

int main() {
	// Initialize the generic heap, passing the task-specific comparison function.
	GML_Heap *heap = gml_init(task_compare);
	if (!heap) {
		fprintf(stderr, "Failed to initialize generic heap.\n");
		return 1;
	}

	// Initialize the external hash map for ID-to-Pointer lookup.
	task_id_map = idp_init();
	if (!task_id_map) {
		fprintf(stderr, "Failed to initialize ID map.\n");
		// Need to clean up the heap before exit if the ID map fails.
		gml_cleanup(heap);
		return 1;
	}

	printf("Starting Single-Thread Scheduler (Scalable GML)...\n\n");

	// Add initial tasks to the scheduler with unique IDs, delays, and intervals.
	add_task(22, heap, task_generic_wizard, 0, 500, "tick");
	add_task(32, heap, task_snape, 0, 1000, "Snape Context");
	add_task(45, heap, task_generic_wizard, 3200, 4000, "              Dumbledore!");
	add_task(1 , heap, task_generic_wizard, 16000, 4000, "                      Ron");
	add_task(2 , heap, task_generic_wizard, 17000, 4000, "                      Ron...");
	add_task(6 , heap, task_generic_wizard, 18400, 4000, "                      Ron WEEEEEEEEEASLEY");
	add_task(77, heap, task_generic_wizard, 20500, 2000, "                              (Hermione)");
	add_task(9 , heap, task_generic_wizard, 28600, 250, "                                      Harry Potter...");

	int loop_count = 0;

	// The main scheduler loop continues as long as there are tasks in the heap.
	while (gml_size(heap) > 0) {
		// Demo: Delete the task with ID 45 after 30 loops.
		if (loop_count == 30) {
			printf("\n--- Deleting Dumbledore Task at runtime! ---\n");
			if (delete_task_by_id(heap, 45)) {
				printf("--- Dumbledore Task successfully removed from heap. ---\n\n");
			} else {
				printf("--- Failed to remove Dumbledore Task. ---\n\n");
			}
		}

		// Demo: Add a new task with the same ID 45 back after 60 loops.
		if (loop_count == 60) {
			printf("\n--- Adding Dumbledore Task back at runtime! ---\n");
			add_task(45, heap, task_generic_wizard, 0, 4000, "              New Dumbledore!");
		}


		// Retrieve the highest priority task (the one scheduled to run next) and remove it from the heap.
		Task *next = gml_pop(heap);
		if (!next) break; // Should not happen if gml_size > 0, but good check.

		// Store the time the task was *scheduled* to run for later use.
		struct timespec scheduled_for_this_run = next->next_run;

		// Block and wait until the scheduled time arrives.
		sleep_until(&scheduled_for_this_run);

		// Get the *actual* time the task started running.
		struct timespec actual;
		if (clock_gettime(CLOCK_MONOTONIC, &actual) == -1) { perror("clock_gettime"); return 1; }

		// Execute the task function, passing scheduled and actual times, and the context.
		next->func(&scheduled_for_this_run, &actual, next->ctx);

		// Calculate the next scheduled run time by adding the interval to the previous time.
		timespec_add_ms(&next->next_run, next->interval_ms);

		// Re-insert the recurring task into the heap for its next execution.
		gml_push(heap, next);

		// Exit condition for the demo.
		if (++loop_count >= 180) break;
	}

	// Clean up all remaining tasks and the heap structure itself.
	cleanup_tasks(heap);
	// Clean up the external ID map structure.
	idp_destroy(task_id_map);
	printf("\nScheduler finished and cleaned up.\n");
	return 0;
}