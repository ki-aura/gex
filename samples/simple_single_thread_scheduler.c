#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_TASKS 10

typedef void (*task_cb)(void);

typedef struct {
    int interval_ms;       // how often to run
    int initial_delay_ms;  // optional initial delay
    task_cb callback;
    struct timespec next_run; // next scheduled time
} Task;

Task tasks[MAX_TASKS];
int task_count = 0;

// Utility: add milliseconds to timespec
void timespec_add_ms(struct timespec *t, int ms) {
    t->tv_sec += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000;
    if (t->tv_nsec >= 1000000000) {
        t->tv_sec += 1;
        t->tv_nsec -= 1000000000;
    }
}

// Sleep until the earliest task is due, or do minimal sleep if it's already past
void sleep_until_next_task(struct timespec *earliest) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long sec = earliest->tv_sec - now.tv_sec;
    long nsec = earliest->tv_nsec - now.tv_nsec;

    // Adjust for negative nanoseconds
    if (nsec < 0) { 
        sec -= 1;
        nsec += 1000000000;
    }

    // Clamp to zero to avoid negative sleep
    if (sec < 0) sec = 0;
    if (nsec < 0) nsec = 0;

    struct timespec sleep_time = { sec, nsec };

    // On macOS, sleeping too short can be ignored, so clamp to 1 ms minimum
    if (sleep_time.tv_sec == 0 && sleep_time.tv_nsec < 1000000)
        sleep_time.tv_nsec = 1000000;

    nanosleep(&sleep_time, NULL);
}


// Compare timespecs: return <0 if a<b, 0 if equal, >0 if a>b
int timespec_cmp(const struct timespec *a, const struct timespec *b) {
    if (a->tv_sec != b->tv_sec)
        return (a->tv_sec > b->tv_sec) ? 1 : -1;
    if (a->tv_nsec != b->tv_nsec)
        return (a->tv_nsec > b->tv_nsec) ? 1 : -1;
    return 0;
}

// Register a new task with optional initial delay
void add_task(task_cb cb, int interval_ms, int initial_delay_ms) {
    if (task_count >= MAX_TASKS) return;
    Task *t = &tasks[task_count++];
    t->interval_ms = interval_ms;
    t->initial_delay_ms = initial_delay_ms;
    t->callback = cb;

    // Set next_run = now + initial_delay
    clock_gettime(CLOCK_MONOTONIC, &t->next_run);
    timespec_add_ms(&t->next_run, initial_delay_ms);
}

// Example tasks
void snape() {
 	static int counter = 0;
 	if (counter++ < 2) printf("Snape\n"); 
 	if (counter == 4) counter = 0;
 }
void severous() { printf("Severous Snape\n"); }
void dumbledor() { printf("Dumbledor!\n"); }
void potter() { printf("Harry Potter...\n"); }
void ron() { printf("Ron Weeeeeeeeeasly\n"); }

int main() {
    add_task(snape, 1000, 0);           // every 1s, start immediately
    add_task(severous, 4000, 2000); 	// every 4s, first run 2s in
    add_task(dumbledor, 4000, 3000);    
    add_task(ron, 7000, 6000);    
   	add_task(potter, 250, 13000);    

    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        // Track earliest next run
        struct timespec *earliest = NULL;

        for (int i = 0; i < task_count; i++) {
            Task *t = &tasks[i];

            // Run all tasks whose next_run <= now
            while (timespec_cmp(&now, &t->next_run) >= 0) {
                t->callback();
                timespec_add_ms(&t->next_run, t->interval_ms); // keep on original schedule
            }

            if (!earliest || timespec_cmp(&t->next_run, earliest) < 0)
                earliest = &t->next_run;
        }

		if (earliest) sleep_until_next_task(earliest);

    }

    return 0;
}

