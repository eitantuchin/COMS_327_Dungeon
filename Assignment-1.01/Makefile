CC = gcc
CFLAGS = -g -Wall

SRC = dungeon_generation.c
OBJ = $(SRC:.c=.o)

OUT = dungeon_generation

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

clean:
	rm -f $(OBJ) $(OUT)
