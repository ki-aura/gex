# Compiler selection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC = clang
else
    CC = gcc
endif

# Common flags
CFLAGS_COMMON = 
TARGET        = gtree
SRC           = gtree.c visit_hash.c option_parsing.c memsafe.c print.c
OBJ           = $(SRC:.c=.o)

.PHONY: all clean release tidy

# Default target
all: release

# Release build
release: CFLAGS = $(CFLAGS_COMMON)
release: $(TARGET)

# debug build
debug: CFLAGS = -Wall -Wextra -fsanitize=address -g -O1
debug: $(TARGET)


tidy:
	xcrun clang-tidy $(SRC) \
		-checks='clang-diagnostic-*,clang-analyzer-*,misc-*' \
		-- -Wall -Wextra

bigtidy:
	xcrun clang-tidy $(SRC) \
		-checks='clang-diagnostic-*,clang-analyzer-*,misc-*,-misc-include-cleaner' \
		-- -Wall -Wextra -Wshadow -Wconversion -Wsign-conversion -Wcast-qual -Wpedantic

maxtidy:
	xcrun clang-tidy $(SRC) \
		-checks='clang-diagnostic-*,clang-analyzer-*,misc-*,-misc-include-cleaner, bugprone-*,-bugprone-reserved-identifier' \
		-- -Wall -Wextra -Wshadow -Wconversion -Wsign-conversion -Wcast-qual -Wpedantic

# Build rules
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(TARGET) $(OBJ)
