#include <ncurses.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void handle_sigint(int sig) {
    (void)sig;
    endwin();   // restore terminal on Ctrl-C
    exit(EXIT_SUCCESS);
}

int main(void) {
    // Catch Ctrl-C to cleanup ncurses
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    initscr();
    cbreak();
    noecho();
    curs_set(0);

    struct timespec now_monotonic, next_tick;

    // Align next_tick to the next whole second
    clock_gettime(CLOCK_MONOTONIC, &now_monotonic);
    next_tick.tv_sec  = now_monotonic.tv_sec + 1;
    next_tick.tv_nsec = 0;

    while (1) {
        // Display wall-clock time
        time_t t_real;
        struct tm *tm_real;
        t_real = time(NULL);
        tm_real = localtime(&t_real);

        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_real);

        mvprintw(0, 0, "Time: %s", buf);
        refresh();

        // Sleep until next whole second using monotonic clock
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        long sec  = next_tick.tv_sec  - now.tv_sec;
        long nsec = next_tick.tv_nsec - now.tv_nsec;
        if (nsec < 0) { sec--; nsec += 1000000000; }
        if (sec < 0) { sec = 0; nsec = 0; }

        struct timespec sleep_time = { sec, nsec };
        nanosleep(&sleep_time, NULL);

        // Schedule next tick
        next_tick.tv_sec += 1;
    }

    endwin();
    return 0;
}
