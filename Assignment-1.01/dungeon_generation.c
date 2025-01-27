#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

// rememeber dungeon is row by col so height comes first before width
char dungeon[21][80];
static int DUNGEON_HEIGHT = 21;
static int DUNGEON_WIDTH = 80;
static struct Room *rooms[10];
int roomCounter = 0;
bool addedCorrdorStairs = false;
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

    // Connect rooms one by one
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
    }
}

void carveCorridor(int startX, int startY, int endX, int endY)
{
    int x = startX;
    int y = startY;

    // Track all corridor positions to replace one with a staircase later
    int corridorPositions[100][2]; // Array to store the positions of corridor cells
    int corridorCount = 0;
    
    // Introduce more random direction changes
    bool horizontalFirst = rand() % 2 == 0; // Start moving either horizontally or vertically
    int directionChanges = rand() % 5 + 5;  // Random number of direction changes (between 5 and 9)

    for (int i = 0; i < directionChanges; ++i) {
        // Randomly choose whether to move horizontally or vertically
        if (horizontalFirst) {
            if (x < endX) x++;    // Move right
            else if (x > endX) x--;  // Move left
        } else {
            if (y < endY) y++;    // Move down
            else if (y > endY) y--;  // Move up
        }

        // Carve the corridor at the new position
        if (dungeon[x][y] == ' ') {
            dungeon[x][y] = '#'; // Carve the path
            corridorPositions[corridorCount][0] = x;
            corridorPositions[corridorCount][1] = y;
            corridorCount++;
        }

        // Introduce some randomness in direction after each step
        if (rand() % 3 == 0) {  // 1 in 3 chance to change direction
            horizontalFirst = !horizontalFirst;  // Flip the direction for the next step
        }

        // Randomize the number of steps taken in the current direction (between 2 and 4)
        int steps = rand() % 3 + 2;  // Move 2 to 4 steps in the current direction
        for (int j = 0; j < steps; ++j) {
            // Move the current number of steps
            if (horizontalFirst) {
                if (x < endX) x++;
                else if (x > endX) x--;
            } else {
                if (y < endY) y++;
                else if (y > endY) y--;
            }

            // Carve the corridor
            if (dungeon[x][y] == ' ') {
                dungeon[x][y] = '#';
                corridorPositions[corridorCount][0] = x;
                corridorPositions[corridorCount][1] = y;
                corridorCount++;
            }
        }
    }

    // Ensure the corridor reaches the destination
    while (x != endX || y != endY) {
        if (x < endX) x++;
        else if (x > endX) x--;

        if (y < endY) y++;
        else if (y > endY) y--;

        // Carve the path to the destination
        if (dungeon[x][y] == ' ') {
            dungeon[x][y] = '#'; // Carve the path
            corridorPositions[corridorCount][0] = x;
            corridorPositions[corridorCount][1] = y;
            corridorCount++;
        }
    }

    // Randomly choose one corridor position to replace with a staircase
    /*if (!addedCorrdorStairs) {
        addedCorrdorStairs = true;
        int randomCorridorIndex = rand() % corridorCount;
        int staircaseX = corridorPositions[randomCorridorIndex][0];
        int staircaseY = corridorPositions[randomCorridorIndex][1];

        // Randomly choose up or down staircase
        if (rand() % 2 == 0) {
            dungeon[staircaseX][staircaseY] = '<'; // Upwards staircase
        } else {
            dungeon[staircaseX][staircaseY] = '>'; // Downwards staircase
        }
    }
    */
}


void addRoomsAndStairs()
{
    int minRoomHeight = 3;
    int minRoomWidth = 4;
    srand(time(NULL));
    int numRooms = rand() % 2 + 6; // random number of rooms, at least 6 and up to 7
    int counter = 0;
    int stairsCounter = 0;
    while (counter != numRooms) // until we have enough rooms
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
