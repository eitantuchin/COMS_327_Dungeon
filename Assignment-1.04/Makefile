CC = gcc

CFLAGS = -g -Wall

LDFLAGS = -lm -lncurses

TARGET = dungeon_monsters

SRCS = dungeon_monsters.c priority_queue.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

