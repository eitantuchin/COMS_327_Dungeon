CC = clang++

CXXFLAGS = -g -Wall -std=c++11

LDFLAGS = -lm -lncurses

TARGET = dungeon_game

SRCS = dungeon_game.cpp priority_queue.cpp monster.cpp pc.cpp dungeon.cpp character.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

