#pragma once

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
#include <vector>


using namespace std;
class Monster;
class Dungeon;
class my_priority_queue;

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
#define PC_HEALTH 500
#define MAX_NUM_MONSTERS 100
#define MIN_NUM_MONSTERS 7
#define MAX_NUM_ITEMS 15
#define MIN_NUM_ITEMS 10
#define FILE_MARKER "RLG327-S2025"

typedef struct leaderboardEntry {
    string name;
    int score;
    string timestamp; 
} leaderboardEntry_t ;

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
extern const cell_t POINTER_CELL;

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
    WEAR_ITEM,
    DROP_ITEM,
    EXPUNGE_ITEM,
    INSPECT_ITEM,
    NO_SELECT
} item_select_t;

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
    MONSTER_LIST,
    PLAYER_TELEPORT,
    DISTANCE_MAPS,
    HARDNESS_MAP,
    ITEM_MENU,
    INVENTORY,
    EQUIPMENT,
    LOOK_AT_MONSTER,
    MONSTER_DETAILS,
    STATS,
    SETTINGS
} mode_type_t;

extern Dungeon dungeon;
extern my_priority_queue event_queue;
extern bool gameOver;
extern bool fogOfWar;
extern bool playerToMove;
extern string gameMessage;
extern string directionMessage;
extern string turnMessage;
extern string dirNames[8];
extern int scrollOffset;
extern cell_t targetingPointerPreviousCell;
extern vector<string> invalidItemsAndMonsters;
extern item_select_t choosingCarryItem;
extern bool choosingEquipmentItem;
extern int selectedMonsterIndex;
extern bool cheatsEnabled;

// Function prototypes

void printGame(int value);
void calculateDistances(int tunneling);
void checkGameConditions(void);
void processEvents(void);
void scheduleEvent(event_type_t type, int monsterIndex, int currentTurn);
void checkKeyInput(void);
void drawMessages(void);
void updateFogMap(void);
void printCharacter(int x, int y);
void moveTargetingPointer(int key);
void initTargetingPointer(void);
pair<int, int> getPointerCellPosition(void);
bool containsString(const vector<string>& array, const string& value);
bool containsInt(const vector<int>& array, const int& value);
pair<int, int> getMinAndMaxDamage(void);
void displaySettings(void);
void showLeaderboard(void);
