#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <curses.h>
#include <term.h>

static volatile sig_atomic_t tick = 0;
static volatile sig_atomic_t resize_pending = 0;

// --- Signal Handlers ---
static void sigalrm_handler(int signo) { (void)signo; tick = 1; }
static void sigwinch_handler(int signo) { (void)signo; resize_pending = 1; }

// --- Draw Clock ---
static void draw_clock(void) {
    struct timeval tv;
    struct tm tm;
    char buf[16];

    if (gettimeofday(&tv, NULL) == -1) return;
    localtime_r(&tv.tv_sec, &tm);
    strftime(buf, sizeof buf, "%H:%M:%S", &tm);

    // Query current terminal width
    struct winsize ws;
    int cols = 80; // fallback
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0)
        cols = ws.ws_col;

    int row = 0;
    int col = cols - (int)strlen(buf) - 1;
    if (col < 0) col = 0;

    char *sc  = tigetstr("sc");
    char *rc  = tigetstr("rc");
    char *vi  = tigetstr("vi");
    char *ve  = tigetstr("ve");
    char *ce  = tigetstr("el");
    char *cup = tigetstr("cup");

    if (sc) putp(sc);
    if (vi) putp(vi);
    if (cup) putp(tparm(cup, row, col));
    fputs(buf, stdout);
    if (ce) putp(ce);
    if (rc) putp(rc);
    if (ve) putp(ve);

    fflush(stdout);
}

// --- Main ---
int main(void) {
    if (setupterm(NULL, STDOUT_FILENO, NULL) == ERR) {
        fprintf(stderr, "setupterm failed\n");
        return 1;
    }

    // --- Setup signals ---
    signal(SIGALRM, sigalrm_handler);
    signal(SIGWINCH, sigwinch_handler);

    // --- Timer: 1 second ---
    struct itimerval it = {0};
    it.it_interval.tv_sec = 1;
    it.it_value.tv_sec = 1;
    setitimer(ITIMER_REAL, &it, NULL);

    // --- Main loop ---
    while (1) {
        pause();

        if (resize_pending) {
            resize_pending = 0;
            // nothing else needed: draw_clock() will query terminal size dynamically
        }

        if (tick) {
            tick = 0;
            draw_clock();
        }
    }
}
