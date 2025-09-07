# Compiler selection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC = clang
    ASAN_FLAGS = -fsanitize=address,undefined,leak
    TSAN_FLAGS = -fsanitize=thread,undefined
else
    CC = gcc
    ASAN_FLAGS = -fsanitize=address,undefined
    TSAN_FLAGS = -fsanitize=thread,undefined
endif

# Common flags
CFLAGS_COMMON  = -Wextra
CFLAGS_DEBUG   = -g
LIBS           = -lncurses -lpanel -lmenu
TARGET         = gex
SRC            = gex.c view_mode.c file_handling.c edit_mode.c
OBJ            = $(SRC:.c=.o)

.PHONY: all clean tidy asan tsan release

# Default target
all: asan

# Release build
release: CFLAGS = $(CFLAGS_COMMON)
release: $(TARGET)

# AddressSanitizer build
asan: CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $(ASAN_FLAGS)
asan: $(TARGET)

# ThreadSanitizer build
tsan: CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $(TSAN_FLAGS)
tsan: $(TARGET)

# clang-tidy targets
tidy:
	xcrun clang-tidy $(SRC) \
		-checks='clang-diagnostic-*,clang-analyzer-*,misc-unused-parameters' \
		-- -Wall -Wextra

maxtidy:
	xcrun clang-tidy $(SRC) \
		-checks='clang-diagnostic-*,clang-analyzer-*,misc-*,-misc-include-cleaner' \
		-- -Wall -Wextra

calls:
	xcrun clang-tidy edit_mode.c \
	    -checks='clang-analyzer-debug.CallGraph' \
	    -- -Wall -Wextra

# Build rules
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(TARGET) $(OBJ)
