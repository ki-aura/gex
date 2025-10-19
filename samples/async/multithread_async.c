#include <stdio.h>      // Standard input/output functions (e.g., printf)
#include <stdlib.h>     // Standard library functions (e.g., general utilities)
#include <pthread.h>    // POSIX Threads (pthreads) library for multi-threading
#include <unistd.h>     // POSIX operating system API (e.g., sleep)
#include <stdbool.h>    // Boolean type support (e.g., bool, true, false)

// The 'Group' structure represents a queue of work and the synchronization
// primitives for a collection of threads (tasks) that belong to it.
struct Group {
    // A mutex (mutual exclusion) lock to protect shared data in this struct
    // (tickets, members, shutdown) from **race conditions** when accessed
    // by multiple threads simultaneously.
    pthread_mutex_t mutex;

    // A condition variable to allow threads to **wait** efficiently for
    // a certain condition (tickets > 0 or shutdown == true) to become true.
    pthread_cond_t cond;

    // The number of work units ('tickets') remaining for the threads in this group.
    int tickets;

    // The total number of threads assigned to this group.
    int members;

    // A flag set to 'true' to signal all waiting threads in this group to exit.
    bool shutdown;
};

// The 'Task' structure holds the necessary information for a single thread
// (the 'Task') to execute, including its function, context, and the Group it belongs to.
struct Task {
    // The identifier for the POSIX thread.
    pthread_t thread;

    // A function pointer to the specific work this task thread should execute.
    // The function must match the signature 'void (*)(void *)'.
    void (*func)(void *ctx);

    // A generic pointer for passing task-specific data (context) to the func.
    void *ctx;

    // A pointer to the Group structure that manages this thread's work queue and sync.
    struct Group *group;
};

// This is the main function that every created thread will execute.
// It acts as a continuous loop, waiting for 'tickets' to run its task.
void *task_thread(void *arg) {
    // Cast the void* argument back to the Task structure passed during creation.
    struct Task *task = arg;
    struct Group *grp = task->group;

    // The thread's main work loop.
    while (1) {
        // --- START CRITICAL SECTION ---
        // Acquire the mutex lock. No other thread can hold this lock at the same time.
        // This protects the shared group data (tickets, shutdown) from race conditions.
        pthread_mutex_lock(&grp->mutex);

        // Wait until there is work (tickets > 0) OR shutdown is requested.
        // The 'while' loop is necessary to handle **spurious wakeups** and re-check
        // the condition after the thread wakes up. This is the correct pattern.
        while (grp->tickets == 0 && !grp->shutdown) {
            // Atomically **releases the mutex** and **blocks the thread** on the condition variable.
            // When another thread calls pthread_cond_signal/broadcast, this thread wakes up
            // and **reacquires the mutex** before returning from this call.
            pthread_cond_wait(&grp->cond, &grp->mutex);
        }

        // After waking up and reacquiring the lock, check the shutdown flag.
        if (grp->shutdown) {
            // If shutdown is true, release the lock and exit the loop/thread.
            pthread_mutex_unlock(&grp->mutex);
            break;  // exit loop and thread
        }

        // If not shutting down, a ticket must be available (tickets > 0).
        grp->tickets--;  // Consume one ticket, indicating this thread will execute work.

        // Release the mutex lock **before** executing the task's function.
        // This allows other threads to acquire the lock to check for tickets or wait
        // while this thread is busy executing its (potentially long) work.
        pthread_mutex_unlock(&grp->mutex);
        // --- END CRITICAL SECTION ---

        // Execute the actual work function with its context data.
        // The task is executed **outside** the mutex lock.
        task->func(task->ctx);
    }

    // A thread function must return a void*.
    return NULL;
}

// -----------------------------------------------------------------------------

// Example task functions to be run by threads.
// They must match the signature 'void (*)(void *)'.

// A task that maintains a local state (static counter) and prints it.
void task1_func(void *ctx) {
    // 'static' ensures the counter is shared among all calls to this function,
    // but its incrementing *might* still be a race condition if multiple threads
    // call it simultaneously, though 'printf' often serializes output.
    // For a real-world scenario, the static counter should be protected if
    // its exact value consistency across threads is critical.
    static int counter = 0;
    printf("Task 1 running, counter=%d\n", ++counter);
}

// A task that takes a string message via its context pointer and prints it.
void task2_func(void *ctx) {
    printf("Task 2 running: message = %s\n", (char *)ctx);
}

// -----------------------------------------------------------------------------

#define NUM_THREADS 4 // Total number of threads to create in the pool.

int main() {
    // Declare two separate Group structures to manage two different pools of tasks.
    // Threads in g1 will run task1_func; threads in g2 will run task2_func.
    struct Group g1, g2;

    // --- Initialize Group 1 ---
    // Initialize the mutex and condition variable. The second argument (NULL) means
    // default attributes (e.g., standard, non-recursive mutex).
    pthread_mutex_init(&g1.mutex, NULL);
    pthread_cond_init(&g1.cond, NULL);
    g1.tickets = 0;
    g1.members = 0;
    g1.shutdown = false;

    // --- Initialize Group 2 ---
    pthread_mutex_init(&g2.mutex, NULL);
    pthread_cond_init(&g2.cond, NULL);
    g2.tickets = 0;
    g2.members = 0;
    g2.shutdown = false;

    // Array of Task structures to hold thread metadata and configuration.
    struct Task tasks[NUM_THREADS] = {
        // Task 0: Runs task1_func, no context, belongs to Group 1 (g1)
        { .func = task1_func, .ctx = NULL,        .group = &g1 },
        // Task 1: Runs task2_func, context is "hello world", belongs to Group 2 (g2)
        { .func = task2_func, .ctx = "hello world", .group = &g2 },
        // Task 2: Runs task1_func, no context, belongs to Group 1 (g1)
        { .func = task1_func, .ctx = NULL,        .group = &g1 },
        // Task 3: Runs task2_func, context is "hello dino", belongs to Group 2 (g2)
        { .func = task2_func, .ctx = "hello dino",  .group = &g2 }
    };

    // Count members per group by iterating over the tasks array.
    for (int i = 0; i < NUM_THREADS; i++) {
        tasks[i].group->members++; // Increment the member count of the assigned group.
    }

    // Launch threads.
    for (int i = 0; i < NUM_THREADS; i++) {
        // Create a new thread:
        // 1. Thread ID stored in tasks[i].thread.
        // 2. Attributes set to NULL (default).
        // 3. Thread function is task_thread.
        // 4. Argument to task_thread is a pointer to the Task structure.
        pthread_create(&tasks[i].thread, NULL, task_thread, &tasks[i]);
    }

    // Main loop: simulates work being added to the groups over time.
    for (int i = 0; i < 5; i++) {
        // Wait for 1 second before broadcasting new work.
        sleep(1);

        // --- Wake task1 group (g1) ---
        pthread_mutex_lock(&g1.mutex);       // Lock the group's state
        g1.tickets += g1.members;           // Add one ticket for *each* member in the group.
                                            // This means each member can execute its task once.
        pthread_cond_broadcast(&g1.cond);   // Wake up **all** threads waiting on g1's condition variable.
                                            // They will all compete to acquire the mutex and consume a ticket.
        pthread_mutex_unlock(&g1.mutex);     // Unlock the group's state

        // --- Wake task2 group (g2) ---
        pthread_mutex_lock(&g2.mutex);
        g2.tickets += g2.members;
        pthread_cond_broadcast(&g2.cond);
        pthread_mutex_unlock(&g2.mutex);
    }

    // -----------------------------------------------------------------------------
    // SHUTDOWN SEQUENCE: Setting the shutdown flag and broadcasting to wake up all threads.
    // -----------------------------------------------------------------------------

    // Signal Group 1 threads to shutdown
    pthread_mutex_lock(&g1.mutex);
    g1.shutdown = true;                 // Set the exit flag.
    pthread_cond_broadcast(&g1.cond);   // Wake all threads so they can see the flag and exit.
    pthread_mutex_unlock(&g1.mutex);

    // Signal Group 2 threads to shutdown
    pthread_mutex_lock(&g2.mutex);
    g2.shutdown = true;                 // Set the exit flag.
    pthread_cond_broadcast(&g2.cond);   // Wake all threads.
    pthread_mutex_unlock(&g2.mutex);

    // Join all threads. This makes the main thread **wait** for each of the
    // task threads to finish execution (i.e., exit the 'task_thread' function).
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(tasks[i].thread, NULL);
    }

    printf("All threads finished, exiting.\n");

    // Clean up the synchronization primitives. This is good practice.
    pthread_mutex_destroy(&g1.mutex);
    pthread_cond_destroy(&g1.cond);
    pthread_mutex_destroy(&g2.mutex);
    pthread_cond_destroy(&g2.cond);

    return 0;
}