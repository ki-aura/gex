#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

typedef void (*task_func)(struct timespec *scheduled, struct timespec *actual, void *ctx);

typedef struct {
    task_func func;
    void *ctx;
    struct timespec next_run;  // absolute time for next execution
    long interval_ms;
} Task;

#define HEAP_MAX 128

typedef struct {
    Task *tasks[HEAP_MAX];
    int size;
} TaskHeap;

static void heap_push(TaskHeap *h, Task *t) {
    int i = h->size++;
	if (h->size > HEAP_MAX) { fprintf(stderr, "Heap full\n"); return;}
    h->tasks[i] = t;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->tasks[p]->next_run.tv_sec < t->next_run.tv_sec ||
            (h->tasks[p]->next_run.tv_sec == t->next_run.tv_sec &&
             h->tasks[p]->next_run.tv_nsec <= t->next_run.tv_nsec))
            break;
        h->tasks[i] = h->tasks[p];
        i = p;
    }
    h->tasks[i] = t;
}

static Task *heap_pop(TaskHeap *h) {
    if (h->size == 0) return NULL;
    Task *ret = h->tasks[0];
    Task *last = h->tasks[--h->size];
    int i = 0;
    while (2 * i + 1 < h->size) {
        int l = 2 * i + 1, r = l + 1, smallest = l;
        if (r < h->size) {
            if (h->tasks[r]->next_run.tv_sec < h->tasks[l]->next_run.tv_sec ||
                (h->tasks[r]->next_run.tv_sec == h->tasks[l]->next_run.tv_sec &&
                 h->tasks[r]->next_run.tv_nsec < h->tasks[l]->next_run.tv_nsec))
                smallest = r;
        }
        if (last->next_run.tv_sec < h->tasks[smallest]->next_run.tv_sec ||
            (last->next_run.tv_sec == h->tasks[smallest]->next_run.tv_sec &&
             last->next_run.tv_nsec < h->tasks[smallest]->next_run.tv_nsec))
            break;
        h->tasks[i] = h->tasks[smallest];
        i = smallest;
    }
    h->tasks[i] = last;
    return ret;
}

static void timespec_add_ms(struct timespec *t, long ms) {
    t->tv_sec += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000;
    if (t->tv_nsec >= 1000000000) {
        t->tv_sec++;
        t->tv_nsec -= 1000000000;
    }
}

static double timespec_diff(struct timespec *a, struct timespec *b) {
    return (b->tv_sec - a->tv_sec) + (b->tv_nsec - a->tv_nsec) / 1e9;
}

static void sleep_until(const struct timespec *target) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    struct timespec delta;
    delta.tv_sec = target->tv_sec - now.tv_sec;
    delta.tv_nsec = target->tv_nsec - now.tv_nsec;
    if (delta.tv_nsec < 0) {
        delta.tv_sec--;
        delta.tv_nsec += 1000000000;
    }
    if (delta.tv_sec < 0) return;
    nanosleep(&delta, NULL);
}



// Example task functions
void task_snape(struct timespec *scheduled, struct timespec *actual, void *ctx) {
 	static int counter = 0;
 	if (counter++ < 2) printf("Snape\n"); 
 	if (counter == 4) counter = 0;
}

void task_wiz(struct timespec *scheduled, struct timespec *actual, void *ctx) {
    double diff = timespec_diff(scheduled, actual);
    printf("%s\n", (char *)ctx);
}

void task_timer(struct timespec *scheduled, struct timespec *actual, void *ctx) {
    double diff = timespec_diff(scheduled, actual);
    printf("Timer running: message=%s, Scheduled=%ld.%09ld, Actual=%ld.%09ld, Diff=%+.6f s\n",
           (char *)ctx,
           scheduled->tv_sec, scheduled->tv_nsec,
           actual->tv_sec, actual->tv_nsec,
           diff);
}

// Adds a new task to the heap with given callback, initial delay, interval, and context
void add_task(TaskHeap *heap, task_func func, long initial_delay_ms, long interval_ms, void *ctx) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    Task *t = malloc(sizeof(Task));
    t->func = func;
    t->ctx = ctx;
    t->interval_ms = interval_ms;
    t->next_run = now;
    timespec_add_ms(&t->next_run, initial_delay_ms);

    heap_push(heap, t);
}

void cleanup_heap(TaskHeap *heap) {
    for (size_t i = 0; i < heap->size; i++) {
        free(heap->tasks[i]);
    }
    heap->size = 0;
}

// Scheduler
int main() {
    TaskHeap heap = { .size = 0 };

    // Add tasks
    add_task(&heap, task_wiz, 0, 500, "tick");
    add_task(&heap, task_snape, 0, 1000, NULL);
    add_task(&heap, task_wiz, 2000, 4000, "Severus Snape");
    add_task(&heap, task_wiz, 3000, 4000, "Dumbledor!");
    add_task(&heap, task_wiz, 6000, 7000, "Ron WEEEEEEEEEASLEY");
    add_task(&heap, task_wiz, 13000, 250, "Harry Potter...");
    add_task(&heap, task_timer, 0, 9000, "time..");

	while (true) {
		Task *next = heap_pop(&heap);
		if (!next) break;
	
		// Store scheduled time for this run
		struct timespec scheduled_for_this_run = next->next_run;
	
		// Sleep until the scheduled time (if in the past, returns immediately)
		sleep_until(&scheduled_for_this_run);
	
		// Get actual run time
		struct timespec actual;
		clock_gettime(CLOCK_MONOTONIC, &actual);
	
		// Call task function
	 //   next->callback(&scheduled_for_this_run, &actual, next->ctx);
		  next->func(&scheduled_for_this_run, &actual, next->ctx);
		// Compute next scheduled run based on **previous scheduled time**, not actual
		timespec_add_ms(&next->next_run, next->interval_ms);
	
		// Push back into the heap
		heap_push(&heap, next);
	}

    cleanup_heap(&heap);
    return 0;
}
