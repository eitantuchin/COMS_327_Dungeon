CC = g++
CXXFLAGS = -g -Wall -std=c++11
LDFLAGS = -lm -lncurses
TARGET = dungeon_game

SRCS = sources/dungeon_game.cpp \
       sources/priority_queue.cpp \
       sources/monster.cpp \
       sources/pc.cpp \
       sources/dungeon.cpp \
       sources/character.cpp \
       sources/parse.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

sources/%.o: sources/%.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

