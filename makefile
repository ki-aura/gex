CC      = gcc
CFLAGS  = -Wextra -g
LIBS    = -lncurses -lpanel
TARGET  = gex
SRC     = gex.c dp.c view_mode.c file_handling.c
OBJ     = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
