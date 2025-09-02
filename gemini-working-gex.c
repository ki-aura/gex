#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>

#define MIN_HEX_WIDTH 16
#define MIN_HEIGHT 16

// Struct to hold file information
typedef struct {
    char *filename;
    int fd;
    off_t size;
    unsigned char *data;
} file_info_t;

// Struct to hold editor state
typedef struct {
    long long cursor_pos;       // Current byte offset
    long long view_start_offset;  // Byte offset of the top-left of the view
    int active_pane;            // 0 for hex, 1 for ascii
    int edit_mode;              // 0 for move, 1 for edit
    int window_height;
    int window_width;
    int bytes_per_line;
} editor_state_t;

// Global structs
file_info_t file_info;
editor_state_t state;

// Function prototypes
void cleanup();
void draw_editor();
void draw_statusbar();
void draw_modebar();
void handle_resize();
void handle_input(int ch);
void move_cursor(int ch);
void handle_edit_mode(int ch);
int is_hex_char(int ch);
int hex_to_int(int c);

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    file_info.filename = argv[1];

    // Open the file
    file_info.fd = open(file_info.filename, O_RDWR);
    if (file_info.fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Get file size
    struct stat st;
    if (fstat(file_info.fd, &st) == -1) {
        perror("Error getting file size");
        return EXIT_FAILURE;
    }
    file_info.size = st.st_size;

    // Handle empty file case
    if (file_info.size == 0) {
        fprintf(stderr, "Warning: The file is empty. No data to display or edit.\n");
        file_info.data = NULL;
    } else {
        // Map the file into memory using mmap
        file_info.data = mmap(NULL, file_info.size, PROT_READ | PROT_WRITE, MAP_SHARED, file_info.fd, 0);
        if (file_info.data == MAP_FAILED) {
            perror("Error mapping file to memory");
            return EXIT_FAILURE;
        }
    }

    // Initialize ncurses
    initscr();
    raw(); // Pass all characters to program, including Ctrl+C
    noecho(); // Don't echo typed characters
    keypad(stdscr, TRUE); // Enable function keys
    curs_set(1); // Make cursor visible

    // Set initial state
    state.cursor_pos = 0;
    state.view_start_offset = 0;
    state.active_pane = 0; // Start in hex pane
    state.edit_mode = 0; // Start in move mode

    handle_resize(); // Initial draw

    int ch;
    while ((ch = getch()) != 'q') {
        if (ch == KEY_RESIZE) {
            handle_resize();
        } else {
            handle_input(ch);
        }
    }

    // Cleanup
    cleanup();

    return EXIT_SUCCESS;
}

// Clean up resources and exit ncurses
void cleanup() {
    if (file_info.data && file_info.size > 0) {
        // Sync changes back to the file
        if (msync(file_info.data, file_info.size, MS_SYNC) == -1) {
            perror("Error syncing changes to disk");
        }
        munmap(file_info.data, file_info.size);
    }
    close(file_info.fd);
    endwin();
}

// Draw the entire editor screen
void draw_editor() {
    clear();
    draw_statusbar();

    if (state.window_width < (MIN_HEX_WIDTH * 3) + MIN_HEX_WIDTH + 10 || state.window_height < MIN_HEIGHT) {
        mvprintw(state.window_height / 2, (state.window_width - 30) / 2, "Window is too small!");
        refresh();
        return;
    }

    // Calculate bytes per line based on window width
    state.bytes_per_line = (state.window_width - (8 + 2 * (MIN_HEX_WIDTH * 2))) / 3;
    if (state.bytes_per_line < MIN_HEX_WIDTH) {
        state.bytes_per_line = MIN_HEX_WIDTH;
    }

    int lines = state.window_height - 4; // Reserve space for status bars and margins
    long long end_offset = state.view_start_offset + (long long)lines * state.bytes_per_line;
    if (end_offset > file_info.size) {
        end_offset = file_info.size;
    }

    for (int i = 0; i < lines; i++) {
        long long line_offset = state.view_start_offset + (long long)i * state.bytes_per_line;
        if (line_offset >= file_info.size) {
            break;
        }

        // Draw address
        mvprintw(i + 2, 0, "%08llX:", line_offset);

        // Draw hex and ascii
        int hex_x = 10;
        int ascii_x = hex_x + state.bytes_per_line * 3 + 2;

        for (int j = 0; j < state.bytes_per_line; j++) {
            long long current_offset = line_offset + j;
            if (current_offset >= file_info.size) {
                break;
            }

            unsigned char byte = file_info.data[current_offset];

            // Highlight the current byte
            if (current_offset == state.cursor_pos) {
                attron(A_REVERSE);
            }

            // Draw hex value
            mvprintw(i + 2, hex_x + j * 3, "%02X ", byte);

            // Draw ascii value
            mvprintw(i + 2, ascii_x + j, "%c", isprint(byte) ? byte : '.');

            // Turn off highlighting
            if (current_offset == state.cursor_pos) {
                attroff(A_REVERSE);
            }
        }
    }

    draw_modebar();
    refresh();
}

// Draw the status bar at the top
void draw_statusbar() {
    int line = 0;
    mvprintw(line, 0, "File: %s", file_info.filename);
    mvprintw(line, state.window_width - 25, "Offset: %08llX", state.cursor_pos);
    mvhline(line + 1, 0, 0, state.window_width);
}

// Draw the mode bar at the bottom
void draw_modebar() {
    int line = state.window_height - 1;
    mvhline(line - 1, 0, 0, state.window_width);
    mvprintw(line, 0, "Mode: %s", state.edit_mode ? "EDIT" : "MOVE");
    if (state.edit_mode) {
        if (state.active_pane == 0) {
            mvprintw(line, state.window_width - 15, "Editing: HEX");
        } else {
            mvprintw(line, state.window_width - 15, "Editing: ASCII");
        }
    } else {
        mvprintw(line, state.window_width - 15, "Pane: %s", state.active_pane ? "ASCII" : "HEX");
    }
}

// Handle terminal resizing
void handle_resize() {
    getmaxyx(stdscr, state.window_height, state.window_width);
    draw_editor();
}

// Main input handler
void handle_input(int ch) {
    if (state.edit_mode) {
        handle_edit_mode(ch);
    } else {
        if (ch == 'E' || ch == 'e') {
            state.edit_mode = 1;
        } else if (ch == '\t') {
            state.active_pane = !state.active_pane;
        } else {
            move_cursor(ch);
        }
    }
    draw_editor();
}

// Handle movement keys
void move_cursor(int ch) {
    long long old_pos = state.cursor_pos;
    int bytes_per_line = state.bytes_per_line;

    switch (ch) {
        case KEY_UP:
            state.cursor_pos -= bytes_per_line;
            break;
        case KEY_DOWN:
            state.cursor_pos += bytes_per_line;
            break;
        case KEY_LEFT:
            state.cursor_pos -= 1;
            break;
        case KEY_RIGHT:
            state.cursor_pos += 1;
            break;
        case KEY_PPAGE: // Page Up
            state.cursor_pos -= (state.window_height - 4) * bytes_per_line;
            break;
        case KEY_NPAGE: // Page Down
            state.cursor_pos += (state.window_height - 4) * bytes_per_line;
            break;
        case KEY_HOME:
            state.cursor_pos = state.view_start_offset;
            break;
        case KEY_END:
            state.cursor_pos = state.view_start_offset + (state.window_height - 4) * bytes_per_line - 1;
            break;
    }

    // Boundary checks
    if (state.cursor_pos < 0) {
        state.cursor_pos = 0;
    }
    if (state.cursor_pos >= file_info.size) {
        state.cursor_pos = file_info.size - 1;
    }

    // Adjust view offset if cursor moves off-screen
    if (state.cursor_pos < state.view_start_offset) {
        state.view_start_offset = state.cursor_pos / bytes_per_line * bytes_per_line;
    }
    if (state.cursor_pos >= state.view_start_offset + (long long)(state.window_height - 3) * bytes_per_line) {
        state.view_start_offset = (state.cursor_pos / bytes_per_line - (state.window_height - 4)) * bytes_per_line;
    }
    if (state.view_start_offset < 0) {
        state.view_start_offset = 0;
    }
}

// Handle editing input
void handle_edit_mode(int ch) {
    if (ch == 27) { // Escape key
        state.edit_mode = 0;
        return;
    }
    
    if (file_info.size == 0) {
        // Can't edit an empty file
        return;
    }

    if (state.active_pane == 0) { // Hex edit mode
        static int hex_digit_count = 0;
        static int first_digit = 0;

        if (is_hex_char(ch)) {
            if (hex_digit_count == 0) {
                first_digit = hex_to_int(ch);
                hex_digit_count = 1;
            } else {
                int second_digit = hex_to_int(ch);
                unsigned char new_byte = (first_digit << 4) | second_digit;
                file_info.data[state.cursor_pos] = new_byte;
                
                // Sync change to file
                if (msync(file_info.data + state.cursor_pos, 1, MS_ASYNC) == -1) {
                    // It's good practice to handle this error, though for this simple app, we can just print
                    // and continue. For a real app, you might want a more robust error display.
                    // perror("Error syncing data");
                }
                
                // Move to next byte
                move_cursor(KEY_RIGHT);
                hex_digit_count = 0;
            }
        }
    } else { // ASCII edit mode
        if (isprint(ch)) {
            file_info.data[state.cursor_pos] = (unsigned char)ch;
            
            // Sync change to file
            if (msync(file_info.data + state.cursor_pos, 1, MS_ASYNC) == -1) {
                // perror("Error syncing data");
            }
            
            // Move to next byte
            move_cursor(KEY_RIGHT);
        }
    }
}

// Helper functions for hex editing
int is_hex_char(int ch) {
    return isdigit(ch) || (tolower(ch) >= 'a' && tolower(ch) <= 'f');
}

int hex_to_int(int c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return 0;
}
