#ifndef dungeon_game_hpp
#define dungeon_game_hpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sstream>
#include <string>
#include <cmath>
#include <array>
#include "monster.hpp"

using namespace std;


// Constants for our dungeon game
#define DUNGEON_WIDTH 80
#define DUNGEON_HEIGHT 21
#define MIN_ROOM_WIDTH 4
#define MIN_ROOM_HEIGHT 3
#define MAX_ROOM_WIDTH 13
#define MAX_ROOM_HEIGHT 8
#define MIN_NUM_ROOMS 6
#define MAX_NUM_ROOMS 7
#define MIN_NUM_STAIRS 2
#define MAX_NUM_STAIRS 3
#define PC_SPEED 10
#define MAX_NUM_MONSTERS 100
#define MIN_NUM_MONSTERS 7
#define FILE_MARKER "RLG327-S2025"

typedef struct message {
    char message[200];
    time_t startTime;
    bool visible;
} message_t;

typedef struct room {
    uint8_t posX;
    uint8_t posY;
    uint8_t height;
    uint8_t width;
} room_t;

typedef struct stair {
    uint8_t posX;
    uint8_t posY;
} stair_t;

typedef struct cell
{
    char ch;
    int hardness;
} cell_t;

extern const cell_t IMMUTABLE_ROCK_CELL;
extern const cell_t ROCK_CELL;
extern const cell_t ROOM_CELL;
extern const cell_t CORRIDOR_CELL;
extern const cell_t UPWARD_STAIRS_CELL;
extern const cell_t DOWNWARD_STAIRS_CELL;
extern const cell_t PLAYER_CELL;

typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    UP_LEFT,
    UP_RIGHT,
    DOWN_LEFT,
    DOWN_RIGHT
} direction_t;

typedef enum {
    EVENT_MONSTER,
    EVENT_PC
} event_type_t;

typedef struct event {
    int turn;
    event_type_t type;
    int index;
} event_t;

typedef enum {
    PLAYER_CONTROL,
    MONSTER_LIST
} mode_type_t;

// Function prototypes
void addRooms(void);
void addStairs(void);
void printDungeon(int showDist, int tunneling);
void addCorridors(void);
void carveCorridor(int startX, int startY, int endX, int endY);
void initImmutableRock(void);
void initPCPosition(void);
bool contains(int *array, size_t size, int value);
void cleanupDungeon(void);
void calculateDistances(int tunneling);
void initMonsters(void);
void moveMonster(int index);
void attemptMove(Monster *m, int dx, int dy);
void checkGameConditions(void);
void processEvents(void);
void scheduleEvent(event_type_t type, int monsterIndex, int currentTurn);
void updateMonsterPosition(int index, int oldX, int oldY, int newX, int newY, Monster *m, bool isTunneling);
bool hasLineOfSight(int x1, int y1, int x2, int y2);
void saveFile(void);
void loadFile(void);
void movePlayer(int key);
void checkKeyInput(void);
bool checkMonsterPlacementToPC(int randX, int randY);
void attack(int distance);
void displayMessage(const char *message);
void drawMessage(void);
void changeDirection(bool clockwise, bool justChangeText);
void generateDungeon(void);
void useStairs(int key);
void resetDungeonLevel(void);
std::string getMonsterPositionString(int monsterIndex);
void displayMonsterList(void);
#endif
