#include "dungeon_path_finding.h"
#include "save_and_load.h"
#include "priority_queue.h"

// our dungeon 
dungeon_t dungeon;
int roomCounter = 0;

const cell_t IMMUTABLE_ROCK_CELL = {' ', 255};
const cell_t ROCK_CELL = {' ', 1};
const cell_t ROOM_CELL = {'.', 0};
const cell_t CORRIDOR_CELL = {'#', 0};
const cell_t UPWARD_STAIRS_CELL = {'<', 0};
const cell_t DOWNWARD_STAIRS_CELL = {'>', 0};
const cell_t PLAYER_CELL = {'@', 0};

/*
 Allows the user to choose between saving, loading, and creating dungeons
 */
int main(int argc, char *argv[])
{
    srand((unsigned int) time(NULL));
    dungeon.numRooms = MIN_NUM_ROOMS;
    dungeon.rooms = (room_t *)malloc(sizeof(room_t) * dungeon.numRooms);
    int numStairs = rand() % (MAX_NUM_STAIRS - MIN_NUM_STAIRS + 1) + MIN_NUM_STAIRS;
    dungeon.upwardStairs = (stair_t *)malloc(sizeof(stair_t) * numStairs);
    dungeon.downwardStairs = (stair_t *)malloc(sizeof(stair_t) * numStairs);
    // make a dungeon but no saving or loading
    if(argv[1] == NULL)
    {
        initImmutableRock();
        addRooms();
        addCorridors();
        addStairs(numStairs);
        initPCPosition();
    }
    // load the dungeon from disk
    else if(strcmp(argv[1], "--load") == 0 && argv[2] == NULL)
    {
        loadFile();
    }
    // make a dungeon and save it to disk
    else if(strcmp(argv[1], "--save") == 0 && argv[2] == NULL)
    {
        initImmutableRock();
        addRooms();
        addCorridors();
        addStairs(numStairs);
        initPCPosition();
        saveFile();
    }
    // load a dungeon from disk and save it
    else if((strcmp(argv[1], "--load") == 0) && (strcmp(argv[2], "--save") == 0))
    {
        loadFile();
        saveFile();
    }
    else
    {
        printf("Unsupported command configuration: Please use either --load , --save, or --load --save.\n");
        return 0;
    }
    // After dungeon is loaded/generated:
    calculateDistances(0);  // Non-tunneling
    calculateDistances(1);  // Tunneling

    // Print all three views
    printf("Standard View:\n");
    printDungeon(0, 0);
       
    printf("\nNon-Tunneling Distance Map:\n");
    printDungeon(1, 0);
       
    printf("\nTunneling Distance Map:\n");
    printDungeon(1, 1);

    cleanupDungeon();
    return 0;
}

/*
 Prints the dungeon to the terminal
 */
void printDungeon(int showDist, int tunneling) {
    printf("----------------------------------------------------------------------------------\n");
    int (*dist)[DUNGEON_WIDTH] = tunneling ? dungeon.tunnelingMap : dungeon.nonTunnelingMap;
    
    for (int y = 0; y < DUNGEON_HEIGHT; ++y) {
        printf("|");
        for (int x = 0; x < DUNGEON_WIDTH; ++x) {
            if (y == dungeon.pc.posX && x == dungeon.pc.posY) {
                printf("@");
            }
            else if (showDist) {
                if (dist[y][x] == INT_MAX) {
                    printf(" ");
                }
                else {
                    printf("%d", dist[y][x] % 10);  // Last digit of valid distances
                }
            }
            else {
                printf("%c", dungeon.map[y][x].ch);
            }
        }
        printf("|\n");
    }
    printf("----------------------------------------------------------------------------------\n");
}

void calculateDistances(int tunneling) {
    // using the priority queue and dijikstra's algorithm
    priority_queue_t *pq = pq_create(DUNGEON_HEIGHT * DUNGEON_WIDTH);
    int (*dist)[DUNGEON_WIDTH] = tunneling ? dungeon.tunnelingMap : dungeon.nonTunnelingMap;

    // Initialize distances
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            dist[y][x] = INT_MAX;
        }
    }

    // Start with PC position
    int pc_y = dungeon.pc.posY;
    int pc_x = dungeon.pc.posX;
    dist[pc_x][pc_y] = 0;
    pq_insert(pq, pc_y, pc_x, 0);

    while (!pq_is_empty(pq)) {
        pq_node_t node = pq_extract_min(pq);
        int x = node.x;
        int y = node.y;

        // Check all 8 neighbors
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                int nx = x + dx;
                int ny = y + dy;

                // Bounds check
                if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT) continue;

                // Check passability
                cell_t cell = dungeon.map[ny][nx];
                if (!tunneling && cell.hardness > 0) continue;  // Non-tunneler can't pass through rock
                if (cell.hardness == 255) continue;             // Immutable rock

                // Calculate weight
                int weight = 1;
                if (tunneling && cell.hardness > 0) { // tunneler meets rock
                    weight = 1 + (cell.hardness / 85);
                }

                // Update distance
                int new_dist = dist[y][x] + weight;
                if (new_dist < dist[ny][nx]) {
                    dist[ny][nx] = new_dist;
                    pq_insert(pq, nx, ny, new_dist);
                }
            }
        }
    }
    pq_destroy(pq);
}

void cleanupDungeon(void) {
    free(dungeon.rooms);
    free(dungeon.upwardStairs);
    free(dungeon.downwardStairs);
}

/*
 Initializes all the immutable rock and regular rock
 */
void initImmutableRock(void)
{
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        for (int x = 0; x < DUNGEON_WIDTH; x++)
        {
            // each rock has a different hardness level
            dungeon.map[y][x] = ROCK_CELL;
            int randomHardness = rand() % 254 + 1; // max hardness is 255 - 1
            dungeon.map[y][x].hardness = randomHardness;
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

/*
 Adds the corridors to the dungeon using Euclidean distance
 */
void addCorridors(void)
{
    int roomCentroids[roomCounter][2];
    for (int i = 0; i < roomCounter; ++i)
    {
        // init room centroids for all rooms
        roomCentroids[i][0] = dungeon.rooms[i].posX + dungeon.rooms[i].height / 2;
        roomCentroids[i][1] = dungeon.rooms[i].posY + dungeon.rooms[i].width / 2;
    }

    // find smallest distance
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

/*
 Creates a corridor on the dungeon map
 */
void carveCorridor(int startX, int startY, int endX, int endY)
{
    int x = startX;
    int y = startY;
    bool xWentLast = false;

    while (x != endX || y != endY)
    {
        // xWentLast ensures that when we have diagonal movement that we insert a corridor in a cardinal direction
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

        if (dungeon.map[x][y].ch == ROCK_CELL.ch &&
            dungeon.map[x][y].hardness < 255) {
            dungeon.map[x][y] = CORRIDOR_CELL;
        }
    }
}

/*
 Adds the rooms to the dungeon
 */
void addRooms(void)
{
    dungeon.numRooms = rand() % (MAX_NUM_ROOMS - MIN_NUM_ROOMS + 1) + MIN_NUM_ROOMS;
    int counter = 0;

    while (counter != dungeon.numRooms)
    {
        bool roomInserted = false;
        while (!roomInserted)
        {
            int randX = rand() % (DUNGEON_HEIGHT - 2) + 1;
            int randY = rand() % (DUNGEON_WIDTH - 2) + 1;
            int randHeight = rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1) + MIN_ROOM_HEIGHT;
            int randWidth = rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1) + MIN_ROOM_WIDTH;

            if (randX + randHeight <= DUNGEON_HEIGHT - 1 && randY + randWidth <= DUNGEON_WIDTH - 1)
            {
                bool validRoomPositionFound = true;
                // ensure that we don't create the room where immutable rock lies
                for (int i = randX - 2; i < randX + randHeight + 2; ++i)
                {
                    for (int j = randY - 2; j < randY + randWidth + 2; ++j)
                    {
                        if (dungeon.map[i][j].ch != ROCK_CELL.ch)
                        {
                            validRoomPositionFound = false;
                            break;
                        }
                    }
                    if (!validRoomPositionFound)
                        break;
                }
                // add the room to the dungeon
                if (validRoomPositionFound)
                {
                    dungeon.rooms[roomCounter].height = randHeight;
                    dungeon.rooms[roomCounter].width = randWidth;
                    dungeon.rooms[roomCounter].posX = randX;
                    dungeon.rooms[roomCounter].posY = randY;
                    roomCounter++;

                    for (int i = randX; i < randX + randHeight; ++i)
                    {
                        for (int j = randY; j < randY + randWidth; ++j)
                        {
                            dungeon.map[i][j] = ROOM_CELL;
                        }
                    }
                    roomInserted = true;
                    counter++;
                }
            }
        }
    }
}

/*
 Initializes the PC's position inside the dungeon
 */
void initPCPosition(void)
{
    int randomRoomNum = rand() % dungeon.numRooms;
    room_t room = dungeon.rooms[randomRoomNum];
    int getX = room.posX;
    int getY = room.posY;
    dungeon.pc.posX = getX + 2; // starting X position of the PC
    dungeon.pc.posY = getY + 2; // starting Y position of the PC
    if (dungeon.map[dungeon.pc.posX][dungeon.pc.posY].ch != '<' ||
        dungeon.map[dungeon.pc.posX][dungeon.pc.posY].ch != '>')
    {
        dungeon.map[dungeon.pc.posX][dungeon.pc.posY] = PLAYER_CELL;
    }
    else
    {
        initPCPosition(); // recursively call to get another random position because we cannot have the PC position override a staircase
    }
}

/*
 Adds the staircases to the dungeon and ensures no room contains multiple stairs and that we have at least one type of each staircase
 */
void addStairs(int numStairs)
{
    int randomRoomNums[MAX_NUM_STAIRS];
    while (dungeon.numUpwardsStairs + dungeon.numDownwardsStairs != numStairs) {
        // ensure at least one upwards stairs is inside the dungeon
        int randomRoomNum;
        while (true) {
            randomRoomNum = rand() % dungeon.numRooms;
            size_t size = sizeof(randomRoomNums) / sizeof(randomRoomNums[0]);
            if (contains(randomRoomNums, size, randomRoomNum)) {
                continue;
            }
            randomRoomNums[dungeon.numUpwardsStairs + dungeon.numDownwardsStairs] = randomRoomNum;
            break;
        }
        room_t room = dungeon.rooms[randomRoomNum];
        //printf("Room width is: %i \n", room.width);
        //printf("Room height is: %i \n", room.height);
        int randomX = room.posX + rand() % room.height;
        int randomY = room.posY + rand() % room.width;
        stair_t stair;
        stair.posX = randomX;
        stair.posY = randomY;
        if (dungeon.numUpwardsStairs + dungeon.numDownwardsStairs == 0)
        {
            dungeon.map[randomX][randomY] = UPWARD_STAIRS_CELL;
            dungeon.upwardStairs[dungeon.numUpwardsStairs] = stair;
            dungeon.numUpwardsStairs++;
        }
        // ensure at least one downwards stairs is inside the dungeon
        else if (dungeon.numUpwardsStairs + dungeon.numDownwardsStairs == 1)
        {
            dungeon.map[randomX][randomY] = DOWNWARD_STAIRS_CELL;
            dungeon.downwardStairs[dungeon.numDownwardsStairs] = stair;
            dungeon.numDownwardsStairs++;
        }
        else
        {
            int randomStairCaseDirection = rand() % 2 + 1;
            if (randomStairCaseDirection == 1) {
                dungeon.map[randomX][randomY] = UPWARD_STAIRS_CELL;
                dungeon.upwardStairs[dungeon.numUpwardsStairs] = stair;
                dungeon.numUpwardsStairs++;
            }
            else {
                dungeon.map[randomX][randomY] = DOWNWARD_STAIRS_CELL;
                dungeon.downwardStairs[dungeon.numDownwardsStairs] = stair;
                dungeon.numDownwardsStairs++;
            }
        }
    }
}

/*
 Checks whether an element exists within given array
 */
bool contains(int *array, size_t size, int value) {
    for (size_t i = 0; i < size; i++) {
        if (array[i] == value) {
            return true; // Value found
        }
    }
    return false; // Value not found
}
