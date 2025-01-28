#include "dungeon_save_and_load.h"

dungeon_t dungeon;
int roomCounter = 0;

int main(int argc, char *argv[])
{
    dungeon.numRooms = MIN_NUM_ROOMS;
    dungeon.rooms = (room_t *)malloc(sizeof(room_t) * dungeon.numRooms);
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
            printf("%c", dungeon.map[i][j].ch);
        }
        printf("|\n");
    }
    printf("----------------------------------------------------------------------------------\n");
}

void initImmutableRock()
{
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        for (int x = 0; x < DUNGEON_WIDTH; x++)
        {
            dungeon.map[y][x] = ROCK_CELL;
        }
    }

    int maxX = DUNGEON_WIDTH - 1;
    int maxY = DUNGEON_HEIGHT - 1;
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        dungeon.map[y][0] = IMMUTABLE_ROCK_CELL;
        dungeon.map[y][maxX] = IMMUTABLE_ROCK_CELL;
    }
    for (int x = 0; x < DUNGEON_WIDTH; x++)
    {
        dungeon.map[0][x] = IMMUTABLE_ROCK_CELL;
        dungeon.map[maxY][x] = IMMUTABLE_ROCK_CELL;
    }
}

void addCorridors()
{
    int roomCentroids[roomCounter][2];
    for (int i = 0; i < roomCounter; ++i)
    {
        roomCentroids[i][0] = dungeon.rooms[i].posX + dungeon.rooms[i].height / 2;
        roomCentroids[i][1] = dungeon.rooms[i].posY + dungeon.rooms[i].length / 2;
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

        if (dungeon.map[x][y].ch == ROCK_CELL.ch && dungeon.map[x][y].hardness == ROCK_CELL.hardness)
        {
            dungeon.map[x][y] = CORRIDOR_CELL;
        }
    }
}

void addRoomsAndStairs()
{
    dungeon.numRooms = rand() % (MAX_NUM_ROOMS - MIN_NUM_ROOMS + 1) + MIN_NUM_ROOMS;
    int counter = 0;
    int stairsCounter = 0;

    while (counter != dungeon.numRooms)
    {
        bool roomInserted = false;
        while (!roomInserted)
        {
            int randX = rand() % (DUNGEON_HEIGHT - 2) + 1;
            int randY = rand() % (DUNGEON_WIDTH - 2) + 1;
            int randHeight = rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1) + MIN_ROOM_HEIGHT;
            int randLength = rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1) + MIN_ROOM_WIDTH;

            if (randX + randHeight <= DUNGEON_HEIGHT - 1 && randY + randLength <= DUNGEON_WIDTH - 1)
            {
                bool validRoomPositionFound = true;
                for (int i = randX - 2; i < randX + randHeight + 2; ++i)
                {
                    for (int j = randY - 2; j < randY + randLength + 2; ++j)
                    {
                        if (dungeon.map[i][j].ch != ROCK_CELL.ch || dungeon.map[i][j].hardness != ROCK_CELL.hardness)
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
                    int stairY = randY + (rand() % randLength);
                    dungeon.rooms[roomCounter].height = randHeight;
                    dungeon.rooms[roomCounter].length = randLength;
                    dungeon.rooms[roomCounter].posX = randX;
                    dungeon.rooms[roomCounter].posY = randY;
                    dungeon.rooms[roomCounter].downwardsStairs = hasDownwardsStairs;
                    dungeon.rooms[roomCounter].upwardsStairs = hasUpwardsStairs;
                    dungeon.rooms[roomCounter].stairX = stairX;
                    dungeon.rooms[roomCounter].stairY = stairY;
                    roomCounter++;

                    for (int i = randX; i < randX + randHeight; ++i)
                    {
                        for (int j = randY; j < randY + randLength; ++j)
                        {
                            if (i == stairX && j == stairY)
                            {
                                if (hasDownwardsStairs)
                                {
                                    dungeon.map[i][j] = DOWNWARD_STAIRS_CELL;
                                }
                                else if (hasUpwardsStairs)
                                {
                                    dungeon.map[i][j] = UPWARD_STAIRS_CELL;
                                }
                                else
                                {
                                    dungeon.map[i][j] = ROOM_CELL;
                                }
                            }
                            else
                            {
                                dungeon.map[i][j] = ROOM_CELL;
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