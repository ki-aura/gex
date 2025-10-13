#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>     // For strncpy, strcmp, strrchr, memset, memcpy, snprintf
#include <sys/stat.h>   // For struct stat, lstat, stat, S_ISDIR, S_ISLNK (POSIX)
#include <libgen.h>     // For basename if needed (not used here)
#include <unistd.h>     // For readlink (POSIX)
#include <inttypes.h>   // For intmax_t
#include "option_parsing.h"
#include "gtree.h"


// -----------------------------------------------------
// ------------------ Options Parsing ------------------
// -----------------------------------------------------

// Structure for command line option help definitions
typedef struct {
    const char *name;
    const char *help;
} HelpDef;

// Table for help messages for options
HelpDef help_table[] = {
    {"-h",   "Display this help message"},
	{"-s",   "Show File & Size totals for populated directories"},
	{"-l",   "Follow Sym-Link directories (disables loop-detection if not specified)"},
	{"-f",   "Show Individual Files"},
    {"-d N", "Maximum depth (will always run to a minimum of 2)"},
    {NULL, NULL} // sentinel: marks the end of the array
};

// List of supported options for getopt(). 'd:' means -d requires an argument.
const char option_list[] = "hslfd:";

// Parses command line arguments using POSIX getopt() and sets the Options struct.
void parse_options(int argc, char *argv[], Options *opts, int default_depth, int *first_file_index) {
    *opts = (Options){0};           	 // Initialize all fields to 0 / false
    opts->max_depth = default_depth;     // Default max depth
    int opt;
    // Loop through options using getopt. getopt returns -1 when no more options are found.
    while ((opt = getopt(argc, argv, option_list)) != -1) { 
        switch (opt) {
            case 'h': opts->show_help = true; break;
            case 's': opts->show_file_stats = true; break;
            case 'l': opts->follow_links = true; break;
            case 'f': opts->show_files = true; break;
            case 'd': {
                int n = atoi(optarg);        // optarg holds the argument for the current option (-d N)
                if (n < 2) n = 2;            // Enforce minimum depth
                if (n > default_depth) n = default_depth; // Prevent array overflow/extreme depth
                opts->max_depth = n;
                break;
			}
            default:
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                exit(EXIT_FAILURE);
        }
    }

    // After getopt finishes, optind is the index of the first non-option argument (the start path).
    if (optind < argc) *first_file_index = optind;
	else *first_file_index = -1;  
}

// Print help message using the help_table
void show_help(void){
	fprintf(stderr, "Usage: fs [options] starting_directory \n");
	fprintf(stderr, "Options:\n");
	for (HelpDef *opt = help_table; opt->name; opt++) {
		fprintf(stderr, "  %s\t%s\n", opt->name, opt->help);
	}
	fprintf(stderr, "Version %s\n", TH_VERSION);
}
