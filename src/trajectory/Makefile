CC      := gcc
CFLAGS  := -std=c11 -O2 -Wall -Wextra -Wpedantic -fno-omit-frame-pointer
LDFLAGS := 

INCLUDES := -Iinclude
SRC := src/xdrmini.c src/list.c src/xtc.c src/trr.c src/traj.c src/main.c src/traj_batch.c
OBJ := $(SRC:.c=.o)
BIN := traj_demo

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c include/*.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
