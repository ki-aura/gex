CC       = clang
CFLAGS_COMMON = -Wextra -g
LIBS     = -lncurses -lpanel -lmenu
TARGET   = gex
SRC      = gex.c dp.c view_mode.c file_handling.c edit_mode.c
OBJ      = $(SRC:.c=.o)

.PHONY: all clean tidy asan tsan ubsan

all: asan  # Default build with AddressSanitizer for general purpose debugging

asan: CFLAGS = $(CFLAGS_COMMON) -fsanitize=address,undefined
asan: $(TARGET)

tsan: CFLAGS = $(CFLAGS_COMMON) -fsanitize=thread,undefined
tsan: $(TARGET)

tidy:
	xcrun clang-tidy $(SRC) -- $(CFLAGS_COMMON) $(LIBS)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
	
