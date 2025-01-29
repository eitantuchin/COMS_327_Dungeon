#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <arpa/inet.h>

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
#define FILE_MARKER "RLG327-S2025"


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

typedef struct pc {
    uint8_t posX;
    uint8_t posY;
} pc_t;

typedef struct cell
{
    char ch;
    int hardness;
} cell_t;

const cell_t IMMUTABLE_ROCK_CELL = {' ', 255};
const cell_t ROCK_CELL = {' ', 100};
const cell_t ROOM_CELL = {'.', 0};
const cell_t CORRIDOR_CELL = {'#', 0};
const cell_t UPWARD_STAIRS_CELL = {'<', 0};
const cell_t DOWNWARD_STAIRS_CELL = {'>', 0};
const cell_t PLAYER_CELL = {'@', 0};

typedef struct dungeon {
    cell_t map[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    room_t *rooms;
    uint16_t numRooms;
    pc_t pc;
    stair_t *upwardStairs;
    stair_t *downwardStairs;
    uint16_t numUpwardsStairs;
    uint16_t numDownwardsStairs;
} dungeon_t;

// Function prototypes
void addRooms(void);
void addStairs(int numStairs);
void printDungeon(void);
void addCorridors(void);
void carveCorridor(int startX, int startY, int endX, int endY);
void initImmutableRock(void);
void initPCPosition(void);
bool contains(int *array, size_t size, int value);
void saveFile(void);
void loadFile(void);
