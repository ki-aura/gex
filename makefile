CC           = clang
CFLAGS_COMMON  = -Wextra
CFLAGS_DEBUG = -g
LIBS         = -lncurses -lpanel -lmenu
TARGET       = gex
SRC          = gex.c dp.c view_mode.c file_handling.c edit_mode.c
OBJ          = $(SRC:.c=.o)

.PHONY: all clean tidy asan tsan release

all: asan

release: CFLAGS = $(CFLAGS_COMMON)
release: $(TARGET)

asan: CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG) -fsanitize=address,undefined,leak
asan: $(TARGET)

tsan: CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG) -fsanitize=thread,undefined
tsan: $(TARGET)

tidy:
	xcrun clang-tidy $(SRC) -- $(CFLAGS_COMMON) $(LIBS)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
	
