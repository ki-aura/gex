# Compiler selection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC = clang
else
    CC = gcc
endif

# Common flags
CFLAGS_COMMON = -Wextra
TARGET        = gex
SRC           = gex.c file_handling.c gex_helper_funcs.c keyb_man.c win_man.c
OBJ           = $(SRC:.c=.o)

# NCURSES flags
ifeq ($(UNAME_S),Darwin)
    # macOS: use system ncurses, panel, menu
    NCURSES_FLAGS = -lncurses -lpanel -lmenu
else
    # Linux: use Homebrew's ncurses
    NCURSES_PREFIX := $(shell brew --prefix ncurses)
    NCURSES_FLAGS = -I$(NCURSES_PREFIX)/include -L$(NCURSES_PREFIX)/lib -lncurses -lpanel -lmenu
endif

.PHONY: all clean release

# Default target
all: release

# Release build
release: CFLAGS = $(CFLAGS_COMMON)
release: $(TARGET)

# Build rules
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(NCURSES_FLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(TARGET) $(OBJ)
