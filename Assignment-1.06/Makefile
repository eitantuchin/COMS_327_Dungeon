CC = clang++
CXXFLAGS = -g -Wall -std=c++11
LDFLAGS = -lm -lncurses
TARGET = dungeon_game

# List all your .cpp files WITH their paths
SRCS = sources/dungeon_game.cpp \
       sources/priority_queue.cpp \
       sources/monster.cpp \
       sources/pc.cpp \
       sources/dungeon.cpp \
       sources/character.cpp

# Replace .cpp with .o
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Tell make that .o files come from sources/*.cpp
sources/%.o: sources/%.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

