#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#define IMMUTABLE_ROCK -1
#define ROCK 0
#define ROOM 1
#define CORRIDOR 2
#define MIN_ROOMS 6
#define DUNGEON_HEIGHT 21
#define DUNGEON_WIDTH 80


struct RoomPair {
    int room1;
    int room2;
};

// rememeber dungeon is row by col so height comes first before width
int NUM_ROOMS = MIN_ROOMS;
char dungeon[DUNGEON_HEIGHT][DUNGEON_WIDTH];
static struct Room *rooms[10];
struct RoomPair connectedRooms[100];
int connectedRoomsCount = 0; 
int roomCounter = 0;

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

// function prototypes
void initDungeonLayout();
void addRoomsAndStairs();
void printDungeon();
void addCorridors();
void carveCorridor(int startX, int startY, int endX, int endY);
bool pairExists(int room1, int room2);
void addConnectionPair(int room1, int room2);

// main
int main(int argc, char *argv[])
{
    // make everything null to begin
    initDungeonLayout();
    // Add the rooms and staircases
    addRoomsAndStairs();
    // Add the corridors
    addCorridors();
    // Print out the dungeon
    printDungeon();
    return 0;
}

// Function to check if a pair already exists
bool pairExists(int room1, int room2) {
    for (int i = 0; i < connectedRoomsCount; ++i) {
        if ((connectedRooms[i].room1 == room1 && connectedRooms[i].room2 == room2) ||
            (connectedRooms[i].room1 == room2 && connectedRooms[i].room2 == room1)) {
            return true;
        }
    }
    return false;
}

// Add a connection pair to the array
void addConnectionPair(int room1, int room2) {
    if (!pairExists(room1, room2)) {
        connectedRooms[connectedRoomsCount].room1 = room1;
        connectedRooms[connectedRoomsCount].room2 = room2;
        connectedRoomsCount++;
         // Add the reverse pair to avoid duplicate paths
        connectedRooms[connectedRoomsCount].room1 = room2;
        connectedRooms[connectedRoomsCount].room2 = room1;
        connectedRoomsCount++;
    }
}

void addCorridors()
{
    // Calculate the centroids of each room
    int roomCentroids[roomCounter][2]; // Each room has a centroid (x, y)
    for (int i = 0; i < roomCounter; ++i) {
        int roomX = rooms[i]->posX + rooms[i]->height / 2;
        int roomY = rooms[i]->posY + rooms[i]->width / 2;
        roomCentroids[i][0] = roomX;
        roomCentroids[i][1] = roomY;
    }

    // Step 1: Connect rooms one by one
    for (int i = 1; i < roomCounter; ++i) {
        // Find the closest previous room to the current room
        int minDist = DUNGEON_HEIGHT * DUNGEON_WIDTH; // A large number
        int closestRoomIndex = 0;
        for (int j = 0; j < i; ++j) {
            double dist = sqrt(pow(roomCentroids[i][0] - roomCentroids[j][0], 2) + 
                               pow(roomCentroids[i][1] - roomCentroids[j][1], 2));
            if (dist < minDist) {
                minDist = dist;
                closestRoomIndex = j;
            }
        }

        // Get the coordinates of the centroids
        int startX = roomCentroids[i][0];
        int startY = roomCentroids[i][1];
        int endX = roomCentroids[closestRoomIndex][0];
        int endY = roomCentroids[closestRoomIndex][1];

        // Carve a path from start to end
        carveCorridor(startX, startY, endX, endY);

        // Add the pair of rooms to the connected pairs array
        addConnectionPair(i, closestRoomIndex);
    }

   int corridorsCarved = 0;
    while (corridorsCarved < 2) {
        // Pick a random room
        int randomRoomIndex = rand() % roomCounter;
        int randomRoomX = roomCentroids[randomRoomIndex][0];
        int randomRoomY = roomCentroids[randomRoomIndex][1];

        // Find the closest room to the random room
        double minDist = DUNGEON_HEIGHT * DUNGEON_WIDTH;  // A large number
        int closestRoomIndex = -1;
        for (int i = 0; i < roomCounter; ++i) {
            if (i != randomRoomIndex) {
                double dist = sqrt(pow(randomRoomX - roomCentroids[i][0], 2) + 
                                   pow(randomRoomY - roomCentroids[i][1], 2));
                if (dist < minDist) {
                    minDist = dist;
                    closestRoomIndex = i;
                }
            }
        }

        // Find the second closest room
        double secondMinDist = DUNGEON_HEIGHT * DUNGEON_WIDTH;  // A large number
        int secondClosestRoomIndex = -1;
        for (int i = 0; i < roomCounter; ++i) {
            if (i != randomRoomIndex && i != closestRoomIndex) {
                double dist = sqrt(pow(randomRoomX - roomCentroids[i][0], 2) + 
                                   pow(randomRoomY - roomCentroids[i][1], 2));
                if (dist < secondMinDist) {
                    secondMinDist = dist;
                    secondClosestRoomIndex = i;
                }
            }
        }

        // Check if the pair exists, if not, carve the corridor
        if (!pairExists(randomRoomIndex, secondClosestRoomIndex)) {
            // Get the coordinates of the second closest room
            int secondRoomX = roomCentroids[secondClosestRoomIndex][0];
            int secondRoomY = roomCentroids[secondClosestRoomIndex][1];

            // Carve the corridor between the random room and the second closest room
            carveCorridor(randomRoomX, randomRoomY, secondRoomX, secondRoomY);
            addConnectionPair(randomRoomIndex, secondClosestRoomIndex);
            corridorsCarved++;
        }
    }
}

void carveCorridor(int startX, int startY, int endX, int endY)
{
    int x = startX;
    int y = startY;

    // Track all corridor positions to replace one with a staircase later
    int corridorPositions[100][2]; // Array to store the positions of corridor cells
    int corridorCount = 0;
    bool xWentLast = false;
    // Ensure the corridor reaches the destination
    // add randomness here
    
    while (x != endX || y != endY) {

        if (!xWentLast) {
            if (x < endX) {
                x++;
            }
            else if (x > endX) {
                x--;
            }
            xWentLast = true;
        }
        else {
            if (y < endY) {
                y++;
            }
            else if (y > endY) {
                y--;
            } 
            xWentLast = false;
        }

        // Carve the path to the destination
        if (dungeon[x][y] == ' ') {
            dungeon[x][y] = '#'; // Carve the path
            corridorPositions[corridorCount][0] = x;
            corridorPositions[corridorCount][1] = y;
            corridorCount++;
        }
    }
}


void addRoomsAndStairs()
{
    int minRoomHeight = 3;
    int minRoomWidth = 4;
    srand(time(NULL));
    NUM_ROOMS = rand() % 2 + 6; // random number of rooms, at least 6 and up to 7
    int counter = 0;
    int stairsCounter = 0;
    while (counter != NUM_ROOMS) // until we have enough rooms
    {
        bool roomInserted = false;
        
        while (!roomInserted) // finds valid positions for rooms and adds them to rooms array
        {
            int randX = rand() % (DUNGEON_HEIGHT - 2) + 1;
            int randY = rand() % (DUNGEON_WIDTH - 2) + 1;
            int randHeight = rand() % 6 + minRoomHeight;
            int randWidth = rand() % 10 + minRoomWidth;

            // Check if the room fits within the dungeon bounds
            if (randX + randHeight <= DUNGEON_HEIGHT - 1 && randY + randWidth <= DUNGEON_WIDTH - 1)
            {
                bool validRoomPositionFound = true;
                // Check if the room overlaps with existing rooms
                for (int i = randX - 2; i < randX + randHeight + 2; ++i)
                {
                    for (int j = randY - 2; j < randY + randWidth + 2; ++j)
                    {
                        if (dungeon[i][j] != ' ') // If any cell is not empty, the room cannot be placed here
                        {
                            validRoomPositionFound = false;
                            break;
                        }
                    }
                    if (!validRoomPositionFound) {
                        break;
                    }
                }

                if (validRoomPositionFound)
                {
                    bool hasUpwardsStairs = false;
                    bool hasDownwardsStairs = false;
                    if (stairsCounter == 0) { // one upwards stairs
                        stairsCounter++;
                        hasUpwardsStairs = true;
                    }
                    else if (stairsCounter == 1) { // one downwards stairs
                        stairsCounter++;
                        hasDownwardsStairs = true;
                    }
                    int stairX = randX + (rand() % (randHeight));
                    int stairY = randY + (rand() % (randWidth));
                    // Dynamically allocate memory for the Room struct
                    struct Room* room = (struct Room*)malloc(sizeof(struct Room)); // Allocate memory for room
                    room->height = randHeight;
                    room->width = randWidth;
                    room->posX = randX;
                    room->posY = randY;
                    room->downwardsStairs = hasDownwardsStairs;
                    room->upwardsStairs = hasUpwardsStairs;
                    room->stairX = stairX;
                    room->stairY = stairY;

                    // Add the Room to the rooms array
                    rooms[roomCounter] = room;
                    roomCounter++;

                    // Add the room to the dungeon (mark the room's area with '.')
                    for (int i = randX; i < randX + randHeight; ++i)
                    {
                        for (int j = randY; j < randY + randWidth; ++j)
                        {
                            if (i == stairX && j == stairY) {
                              if (hasDownwardsStairs) {
                                 dungeon[i][j] = '>';
                              }
                              else if (hasUpwardsStairs) {
                                 dungeon[i][j] = '<';
                              }
                              else {
                                dungeon[i][j] = '.';
                              }
                            }
                            else {
                                dungeon[i][j] = '.';
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

void initDungeonLayout()
{
    for (int i = 0; i < DUNGEON_HEIGHT; ++i)
    {
        for (int j = 0; j < DUNGEON_WIDTH; ++j)
        {
            dungeon[i][j] = ' ';
        }
    }
}

void printDungeon()
{
    printf("----------------------------------------------------------------------------------\n");
    for (int i = 0; i < DUNGEON_HEIGHT; ++i)
    {
        printf("|");
        for (int j = 0; j < DUNGEON_WIDTH; ++j)
        {
            printf("%c", dungeon[i][j]);
        }
        printf("|\n");
    }
    printf("----------------------------------------------------------------------------------");
}
