#include "dungeon_save_and_load.h"

// our dungeon 
dungeon_t dungeon;
int roomCounter = 0;

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
    // print the dungeon everytime
    printDungeon();
    return 0;
}

/*
 Loads a file from disk that contains data for the construction of a dungeon
 */
void loadFile(void){
    initImmutableRock();
    
    // Construct file path
    char *path = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + strlen("/.rlg327/dungeon") + 1));
    if (path == NULL) {
        perror("Failed to allocate memory for file path");
        return;
    }
    strcat(path, getenv("HOME"));
    strcat(path, "/.rlg327/dungeon");

    // Open the file
    FILE *f = fopen(path, "r");
    
    if (f == NULL) {
        perror("Failed to open file");
        free(path);
        return;
    }
    

    // Read file type and version
    char fileType[13];
    uint32_t versionNum;
    uint32_t fileSize;

    fread(&fileType, sizeof(char), 12, f);
    fread(&versionNum, sizeof(uint32_t), 1, f);
    fileType[12] = '\0';

    // Validate file type and version
    if (strcmp(fileType, FILE_MARKER) != 0) {
        printf("Invalid file type: %s\n", fileType);
        fclose(f);
        free(path);
        return;
    }
    if (htonl(versionNum) != 0) {
        printf("Invalid version number: %u\n", versionNum);
        fclose(f);
        free(path);
        return;
    }

    fread(&fileSize, sizeof(uint32_t), 1, f);
    fileSize = htonl(fileSize);
    // printf("File size: %u bytes\n", fileSize);

    // Read player position
    fread(&dungeon.pc.posX, sizeof(uint8_t), 1, f);
    fread(&dungeon.pc.posY, sizeof(uint8_t), 1, f);

    // Ensure player position is within bounds
    if (dungeon.pc.posX >= DUNGEON_HEIGHT || dungeon.pc.posY >= DUNGEON_WIDTH) {
        printf("Invalid player position\n");
        printf("Player's X pos: %i\n", dungeon.pc.posX);
        printf("Player's Y pos: %i\n", dungeon.pc.posY);
        fclose(f);
        free(path);
        return;
    }
    dungeon.map[dungeon.pc.posX][dungeon.pc.posY] = PLAYER_CELL;

    // Load dungeon map hardness
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            fread(&dungeon.map[y][x].hardness, sizeof(uint8_t), 1, f);
        }
    }

    // Read number of rooms and validate it
    fread(&dungeon.numRooms, sizeof(uint16_t), 1, f);
    dungeon.numRooms = htons(dungeon.numRooms);
    if (dungeon.numRooms > MAX_NUM_ROOMS || dungeon.numRooms < MIN_NUM_ROOMS) {
        printf("Invalid number of rooms: %u\n", dungeon.numRooms);
        fclose(f);
        free(path);
        return;
    }

    // Allocate memory for rooms
    dungeon.rooms = (room_t *)malloc(sizeof(room_t) * dungeon.numRooms);
    if (dungeon.rooms == NULL) {
        perror("Failed to allocate memory for rooms");
        fclose(f);
        free(path);
        return;
    }

    // Rebuild rooms
    for (int i = 0; i < dungeon.numRooms; i++) {
        fread(&dungeon.rooms[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon.rooms[i].posY, sizeof(uint8_t), 1, f);
        fread(&dungeon.rooms[i].width, sizeof(uint8_t), 1, f);
        fread(&dungeon.rooms[i].height, sizeof(uint8_t), 1, f);

        // Validate room positions and sizes
        if (dungeon.rooms[i].posY >= DUNGEON_WIDTH || dungeon.rooms[i].posX >= DUNGEON_HEIGHT ||
            dungeon.rooms[i].posY + dungeon.rooms[i].width > DUNGEON_WIDTH ||
            dungeon.rooms[i].posX + dungeon.rooms[i].height > DUNGEON_HEIGHT) {
            printf("Invalid room position or size for room %d\n", i);
            fclose(f);
            free(path);
            return;
        }

        // Mark room cells on the map
        for (int n = dungeon.rooms[i].posX; n < dungeon.rooms[i].posX + dungeon.rooms[i].height; n++) {
            for (int m = dungeon.rooms[i].posY; m < dungeon.rooms[i].posY + dungeon.rooms[i].width; m++) {
                /*if (n >= DUNGEON_WIDTH || m >= DUNGEON_HEIGHT) {
                    printf("Room exceeds dungeon bounds: Room %d at (%d,%d)\n", i, n, m);
                    fclose(f);
                    free(path);
                    return;
                }
                 */
                if (dungeon.map[n][m].ch != '@') {
                    dungeon.map[n][m] = ROOM_CELL;
                }
            }
        }
    }

    // Rebuilding the upstairs and downstairs
    fread(&dungeon.numUpwardsStairs, sizeof(uint16_t), 1, f);
    dungeon.numUpwardsStairs = htons(dungeon.numUpwardsStairs);
    dungeon.upwardStairs = malloc(sizeof(stair_t) * dungeon.numUpwardsStairs);

    // Read stair positions and place them in the map
    for (int i = 0; i < dungeon.numUpwardsStairs; i++) {
        fread(&dungeon.upwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon.upwardStairs[i].posY, sizeof(uint8_t), 1, f);
        dungeon.map[dungeon.upwardStairs[i].posX][dungeon.upwardStairs[i].posY] = UPWARD_STAIRS_CELL;
    }
    
    fread(&dungeon.numDownwardsStairs, sizeof(uint16_t), 1, f);
    dungeon.numDownwardsStairs = htons(dungeon.numDownwardsStairs);
    dungeon.downwardStairs = malloc(sizeof(stair_t) * dungeon.numDownwardsStairs);
    for (int i = 0; i < dungeon.numDownwardsStairs; i++) {
        fread(&dungeon.downwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon.downwardStairs[i].posY, sizeof(uint8_t), 1, f);
        dungeon.map[dungeon.downwardStairs[i].posX][dungeon.downwardStairs[i].posY] = DOWNWARD_STAIRS_CELL;
    }

    // Rebuild corridors
    for (int i = 0; i < DUNGEON_HEIGHT; i++) {
        for (int j = 0; j < DUNGEON_WIDTH; j++) {
            if (dungeon.map[i][j].hardness == 0 && dungeon.map[i][j].ch == ' ') {
                dungeon.map[i][j] = CORRIDOR_CELL;
            }
        }
    }

    fclose(f);
    free(path);
    return;
}

/*
 Saves the current dungeon configuration to disk
 */
void saveFile(void) {
    
    // Ready data in big-endian format
    uint32_t versionNum = 0;
    versionNum = htonl(versionNum);
    
    uint32_t fileSize = 1708 + dungeon.numRooms * 4 + dungeon.numUpwardsStairs * 2 + dungeon.numDownwardsStairs * 2;
    fileSize = htonl(fileSize);
    
    uint16_t numRooms = dungeon.numRooms;
    numRooms = htons(numRooms);
    
    uint16_t numUpStrs = dungeon.numUpwardsStairs;
    numUpStrs = htons(numUpStrs);
    uint16_t numDownStrs = dungeon.numDownwardsStairs;
    numDownStrs = htons(numDownStrs);

    //Opens a path for the file
    char *path = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) +strlen("/.rlg327/dungeon") + 1));
    strcat(path, getenv("HOME"));
    strcat(path, "/.rlg327/dungeon");
    FILE *f = NULL;
    f = fopen(path, "w");
    if (f == NULL) {
       // perror("Failed to open file");
        free(path);
        return;  
    }
   

    //Writes type of file, version, size, location of player, and hardness
    fwrite(&FILE_MARKER, 12, 1, f);
    fwrite(&versionNum, sizeof(uint32_t), 1, f);
    fwrite(&fileSize, sizeof(uint32_t), 1, f);

    fwrite(&dungeon.pc.posX, sizeof(uint8_t), 1, f);
    fwrite(&dungeon.pc.posY, sizeof(uint8_t), 1, f);
    
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        for (int x = 0; x < DUNGEON_WIDTH; x++)
        {
            fwrite(&dungeon.map[y][x].hardness, sizeof(uint8_t), 1, f);
        }
    }
    
    //Writes number of rooms, locations and sizes
    fwrite(&numRooms, sizeof(uint16_t), 1, f);
    for(int i = 0; i < dungeon.numRooms; ++i){
        fwrite(&dungeon.rooms[i].posX, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.rooms[i].posY, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.rooms[i].width, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.rooms[i].height, sizeof(uint8_t), 1, f);
    }

    // Writes numer of upward and downward stairs
    fwrite(&numUpStrs, sizeof(uint16_t), 1, f);
    for(int i = 0; i < dungeon.numUpwardsStairs; ++i) {
        fwrite(&dungeon.upwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.upwardStairs[i].posY, sizeof(uint8_t), 1, f);
    }
    fwrite(&numDownStrs, sizeof(uint16_t), 1, f);
    for(int i = 0; i < dungeon.numDownwardsStairs; ++i) {
        fwrite(&dungeon.downwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.downwardStairs[i].posY, sizeof(uint8_t), 1, f);
    }
    fclose(f);
    free(path);
    return;
}

/*
 Prints the dungeon to the terminal
 */
void printDungeon(void)
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
            int randomHardness = rand() % 205 + 50; // max hardness is 255 - 1
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

        if (dungeon.map[x][y].ch == ROCK_CELL.ch && dungeon.map[x][y].hardness <= 254 && dungeon.map[x][y].hardness >= 50)
        {
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
