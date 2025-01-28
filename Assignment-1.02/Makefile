CC = gcc

CFLAGS = -g -Wall

LDFLAGS = -lm

TARGET = dungeon_save_and_load

SRCS = dungeon_save_and_load.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
