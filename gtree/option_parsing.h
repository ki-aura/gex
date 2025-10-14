#ifndef OPTION_PARSING_H
#define OPTION_PARSING_H

#include <stdbool.h>

// Structure to hold all parsed command-line options
typedef struct {
    bool show_help;			// -h
    bool show_file_stats;	// -s
    bool follow_links;		// -l
    bool show_files;		// -f
    bool colour_files;		// -c
    int max_depth;   		// -dN
} Options;

void parse_options(int argc, char *argv[], Options *opts, int default_depth, int *first_file_index);
void show_help(void);

#endif  
