#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

#define MIN_ROOMS 6
#define DUNGEON_HEIGHT 21
#define DUNGEON_WIDTH 80

typedef struct
{
    char ch;
    int hardness;
} Cell;

const Cell IMMUTABLE_ROCK_CELL = {' ', INT_MAX};
const Cell ROCK_CELL = {' ', 100};
const Cell ROOM_CELL = {'.', 0};
const Cell CORRIDOR_CELL = {'#', 0};
const Cell UPWARD_STAIRS_CELL = {'<', 0};
const Cell DOWNWARD_STAIRS_CELL = {'>', 0};
Cell dungeon[DUNGEON_HEIGHT][DUNGEON_WIDTH];

struct Room
{
    int height;
    int width;
    int posX;
    int posY;
    bool upwardsStairs;
    bool downwardsStairs;
    int stairX;
    int stairY;
};

int NUM_ROOMS = MIN_ROOMS;
static struct Room *rooms[10];
int roomCounter = 0;

// Function prototypes
void addRoomsAndStairs();
void printDungeon();
void addCorridors();
void carveCorridor(int startX, int startY, int endX, int endY);
void initImmutableRock();

int main(int argc, char *argv[])
{
    srand(time(NULL));
    initImmutableRock();
    addRoomsAndStairs();
    addCorridors();
    printDungeon();
    return 0;
}

void printDungeon()
{
    printf("----------------------------------------------------------------------------------\n");
    for (int i = 0; i < DUNGEON_HEIGHT; ++i)
    {
        printf("|");
        for (int j = 0; j < DUNGEON_WIDTH; ++j)
        {
            printf("%c", dungeon[i][j].ch);
        }
        printf("|\n");
    }
    printf("----------------------------------------------------------------------------------");
}

void initImmutableRock()
{
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        for (int x = 0; x < DUNGEON_WIDTH; x++)
        {
            dungeon[y][x] = ROCK_CELL;
        }
    }

    int maxX = DUNGEON_WIDTH - 1;
    int maxY = DUNGEON_HEIGHT - 1;
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        dungeon[y][0] = IMMUTABLE_ROCK_CELL;
        dungeon[y][maxX] = IMMUTABLE_ROCK_CELL;
    }
    for (int x = 0; x < DUNGEON_WIDTH; x++)
    {
        dungeon[0][x] = IMMUTABLE_ROCK_CELL;
        dungeon[maxY][x] = IMMUTABLE_ROCK_CELL;
    }
}

void addCorridors()
{
    int roomCentroids[roomCounter][2];
    for (int i = 0; i < roomCounter; ++i)
    {
        roomCentroids[i][0] = rooms[i]->posX + rooms[i]->height / 2;
        roomCentroids[i][1] = rooms[i]->posY + rooms[i]->width / 2;
    }

    for (int i = 1; i < roomCounter; ++i)
    {
        int minDist = DUNGEON_HEIGHT * DUNGEON_WIDTH;
        int closestRoomIndex = 0;
        for (int j = 0; j < i; ++j)
        {
            double dist = sqrt(pow(roomCentroids[i][0] - roomCentroids[j][0], 2) +
                               pow(roomCentroids[i][1] - roomCentroids[j][1], 2));
            if (dist < minDist)
            {
                minDist = dist;
                closestRoomIndex = j;
            }
        }

        int startX = roomCentroids[i][0];
        int startY = roomCentroids[i][1];
        int endX = roomCentroids[closestRoomIndex][0];
        int endY = roomCentroids[closestRoomIndex][1];
        carveCorridor(startX, startY, endX, endY);
    }
}

void carveCorridor(int startX, int startY, int endX, int endY)
{
    int x = startX;
    int y = startY;
    bool xWentLast = false;

    while (x != endX || y != endY)
    {
        if (!xWentLast)
        {
            if (x < endX)
                x++;
            else if (x > endX)
                x--;
            xWentLast = true;
        }
        else
        {
            if (y < endY)
                y++;
            else if (y > endY)
                y--;
            xWentLast = false;
        }

        if (dungeon[x][y].ch == ROCK_CELL.ch && dungeon[x][y].hardness == ROCK_CELL.hardness)
        {
            dungeon[x][y] = CORRIDOR_CELL;
        }
    }
}

void addRoomsAndStairs()
{
    int minRoomHeight = 3;
    int minRoomWidth = 4;
    NUM_ROOMS = rand() % 2 + 6;
    int counter = 0;
    int stairsCounter = 0;

    while (counter != NUM_ROOMS)
    {
        bool roomInserted = false;
        while (!roomInserted)
        {
            int randX = rand() % (DUNGEON_HEIGHT - 2) + 1;
            int randY = rand() % (DUNGEON_WIDTH - 2) + 1;
            int randHeight = rand() % 6 + minRoomHeight;
            int randWidth = rand() % 10 + minRoomWidth;

            if (randX + randHeight <= DUNGEON_HEIGHT - 1 && randY + randWidth <= DUNGEON_WIDTH - 1)
            {
                bool validRoomPositionFound = true;
                for (int i = randX - 2; i < randX + randHeight + 2; ++i)
                {
                    for (int j = randY - 2; j < randY + randWidth + 2; ++j)
                    {
                        if (dungeon[i][j].ch != ROCK_CELL.ch || dungeon[i][j].hardness != ROCK_CELL.hardness)
                        {
                            validRoomPositionFound = false;
                            break;
                        }
                    }
                    if (!validRoomPositionFound)
                        break;
                }

                if (validRoomPositionFound)
                {
                    bool hasUpwardsStairs = false;
                    bool hasDownwardsStairs = false;
                    if (stairsCounter == 0)
                    {
                        hasUpwardsStairs = true;
                        stairsCounter++;
                    }
                    else if (stairsCounter == 1)
                    {
                        hasDownwardsStairs = true;
                        stairsCounter++;
                    }

                    int stairX = randX + (rand() % randHeight);
                    int stairY = randY + (rand() % randWidth);
                    struct Room *room = (struct Room *)malloc(sizeof(struct Room));
                    room->height = randHeight;
                    room->width = randWidth;
                    room->posX = randX;
                    room->posY = randY;
                    room->downwardsStairs = hasDownwardsStairs;
                    room->upwardsStairs = hasUpwardsStairs;
                    room->stairX = stairX;
                    room->stairY = stairY;
                    rooms[roomCounter] = room;
                    roomCounter++;

                    for (int i = randX; i < randX + randHeight; ++i)
                    {
                        for (int j = randY; j < randY + randWidth; ++j)
                        {
                            if (i == stairX && j == stairY)
                            {
                                if (hasDownwardsStairs)
                                {
                                    dungeon[i][j] = DOWNWARD_STAIRS_CELL;
                                }
                                else if (hasUpwardsStairs)
                                {
                                    dungeon[i][j] = UPWARD_STAIRS_CELL;
                                }
                                else
                                {
                                    dungeon[i][j] = ROOM_CELL;
                                }
                            }
                            else
                            {
                                dungeon[i][j] = ROOM_CELL;
                            }
                        }
                    }
                    roomInserted = true;
                    counter++;
                }
            }
        }
    }
}
