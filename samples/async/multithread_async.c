#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

struct Group {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int tickets;    // how many runs remaining
    int members;    // threads in this group
    bool shutdown;  // flag to tell threads to exit
};

struct Task {
    pthread_t thread;
    void (*func)(void *ctx);
    void *ctx;
    struct Group *group;
};

void *task_thread(void *arg) {
    struct Task *task = arg;
    struct Group *grp = task->group;

    while (1) {
        pthread_mutex_lock(&grp->mutex);

        // Wait until there is work or shutdown is requested
        while (grp->tickets == 0 && !grp->shutdown) {
            pthread_cond_wait(&grp->cond, &grp->mutex);
        }

        if (grp->shutdown) {
            pthread_mutex_unlock(&grp->mutex);
            break;  // exit loop and thread
        }

        grp->tickets--;  // consume one ticket
        pthread_mutex_unlock(&grp->mutex);

        task->func(task->ctx);
    }

    return NULL;
}

// Example task functions
void task1_func(void *ctx) {
    static int counter = 0;
    printf("Task 1 running, counter=%d\n", ++counter);
}

void task2_func(void *ctx) {
    printf("Task 2 running: message = %s\n", (char *)ctx);
}

#define NUM_THREADS 4

int main() {
    struct Group g1, g2;
    pthread_mutex_init(&g1.mutex, NULL);
    pthread_cond_init(&g1.cond, NULL);
    g1.tickets = 0;
    g1.members = 0;
    g1.shutdown = false;

    pthread_mutex_init(&g2.mutex, NULL);
    pthread_cond_init(&g2.cond, NULL);
    g2.tickets = 0;
    g2.members = 0;
    g2.shutdown = false;

    struct Task tasks[NUM_THREADS] = {
        { .func = task1_func, .ctx = NULL,          .group = &g1 },
        { .func = task2_func, .ctx = "hello world", .group = &g2 },
        { .func = task1_func, .ctx = NULL,          .group = &g1 },
        { .func = task2_func, .ctx = "hello dino",  .group = &g2 }
    };

    // Count members per group
    for (int i = 0; i < NUM_THREADS; i++) {
        tasks[i].group->members++;
    }

    // Launch threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&tasks[i].thread, NULL, task_thread, &tasks[i]);
    }

    // Main loop: give each thread one ticket per broadcast
    for (int i = 0; i < 5; i++) {
        sleep(1);

        // Wake task1 group
        pthread_mutex_lock(&g1.mutex);
        g1.tickets += g1.members;
        pthread_cond_broadcast(&g1.cond);
        pthread_mutex_unlock(&g1.mutex);

        // Wake task2 group
        pthread_mutex_lock(&g2.mutex);
        g2.tickets += g2.members;
        pthread_cond_broadcast(&g2.cond);
        pthread_mutex_unlock(&g2.mutex);
    }

    // Signal threads to shutdown
    pthread_mutex_lock(&g1.mutex);
    g1.shutdown = true;
    pthread_cond_broadcast(&g1.cond);
    pthread_mutex_unlock(&g1.mutex);

    pthread_mutex_lock(&g2.mutex);
    g2.shutdown = true;
    pthread_cond_broadcast(&g2.cond);
    pthread_mutex_unlock(&g2.mutex);

    // Join all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(tasks[i].thread, NULL);
    }

    printf("All threads finished, exiting.\n");
    return 0;
}
