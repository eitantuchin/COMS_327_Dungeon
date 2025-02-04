//
//  save_and_load.c
//  Dungeon_Game_327
//
//  Created by Eitan Tuchin on 1/30/25.
//

#include "save_and_load.h"
#include "dungeon_path_finding.h"

dungeon_t dungeon2;

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
    //printf("File size: %u bytes\n", fileSize);

    // Read player position
    fread(&dungeon2.pc.posX, sizeof(uint8_t), 1, f);
    fread(&dungeon2.pc.posY, sizeof(uint8_t), 1, f);

    // Ensure player position is within bounds
    if (dungeon2.pc.posX >= DUNGEON_HEIGHT || dungeon2.pc.posY >= DUNGEON_WIDTH) {
        printf("Invalid player position\n");
        printf("Player's X pos: %i\n", dungeon2.pc.posX);
        printf("Player's Y pos: %i\n", dungeon2.pc.posY);
        fclose(f);
        free(path);
        return;
    }
    dungeon2.map[dungeon2.pc.posX][dungeon2.pc.posY] = PLAYER_CELL;

    // Load dungeon map hardness
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            fread(&dungeon2.map[y][x].hardness, sizeof(uint8_t), 1, f);
        }
    }

    // Read number of rooms and validate it
    fread(&dungeon2.numRooms, sizeof(uint16_t), 1, f);
    dungeon2.numRooms = htons(dungeon2.numRooms);
    if (dungeon2.numRooms > MAX_NUM_ROOMS || dungeon2.numRooms < MIN_NUM_ROOMS) {
        printf("Invalid number of rooms: %u\n", dungeon2.numRooms);
        fclose(f);
        free(path);
        return;
    }

    // Allocate memory for rooms
    dungeon2.rooms = (room_t *)malloc(sizeof(room_t) * dungeon2.numRooms);
    if (dungeon2.rooms == NULL) {
        perror("Failed to allocate memory for rooms");
        fclose(f);
        free(path);
        return;
    }

    // Rebuild rooms
    for (int i = 0; i < dungeon2.numRooms; i++) {
        fread(&dungeon2.rooms[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon2.rooms[i].posY, sizeof(uint8_t), 1, f);
        fread(&dungeon2.rooms[i].width, sizeof(uint8_t), 1, f);
        fread(&dungeon2.rooms[i].height, sizeof(uint8_t), 1, f);

        // Validate room positions and sizes
        if (dungeon2.rooms[i].posY >= DUNGEON_WIDTH || dungeon2.rooms[i].posX >= DUNGEON_HEIGHT ||
            dungeon2.rooms[i].posY + dungeon2.rooms[i].width > DUNGEON_WIDTH ||
            dungeon2.rooms[i].posX + dungeon2.rooms[i].height > DUNGEON_HEIGHT) {
            printf("Invalid room position or size for room %d\n", i);
            fclose(f);
            free(path);
            return;
        }

        // Mark room cells on the map
        for (int n = dungeon2.rooms[i].posX; n < dungeon2.rooms[i].posX + dungeon2.rooms[i].height; n++) {
            for (int m = dungeon2.rooms[i].posY; m < dungeon2.rooms[i].posY + dungeon2.rooms[i].width; m++) {
                /*if (n >= DUNGEON_WIDTH || m >= DUNGEON_HEIGHT) {
                    printf("Room exceeds dungeon bounds: Room %d at (%d,%d)\n", i, n, m);
                    fclose(f);
                    free(path);
                    return;
                }
                 */
                if (dungeon2.map[n][m].ch != '@') {
                    dungeon2.map[n][m] = ROOM_CELL;
                }
            }
        }
    }

    // Rebuilding the upstairs and downstairs
    fread(&dungeon2.numUpwardsStairs, sizeof(uint16_t), 1, f);
    dungeon2.numUpwardsStairs = htons(dungeon2.numUpwardsStairs);
    dungeon2.upwardStairs = malloc(sizeof(stair_t) * dungeon2.numUpwardsStairs);

    // Read stair positions and place them in the map
    for (int i = 0; i < dungeon2.numUpwardsStairs; i++) {
        fread(&dungeon2.upwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon2.upwardStairs[i].posY, sizeof(uint8_t), 1, f);
        dungeon2.map[dungeon2.upwardStairs[i].posX][dungeon2.upwardStairs[i].posY] = UPWARD_STAIRS_CELL;
    }
    
    fread(&dungeon2.numDownwardsStairs, sizeof(uint16_t), 1, f);
    dungeon2.numDownwardsStairs = htons(dungeon2.numDownwardsStairs);
    dungeon2.downwardStairs = malloc(sizeof(stair_t) * dungeon2.numDownwardsStairs);
    for (int i = 0; i < dungeon2.numDownwardsStairs; i++) {
        fread(&dungeon2.downwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon2.downwardStairs[i].posY, sizeof(uint8_t), 1, f);
        dungeon2.map[dungeon2.downwardStairs[i].posX][dungeon2.downwardStairs[i].posY] = DOWNWARD_STAIRS_CELL;
    }

    // Rebuild corridors
    for (int i = 0; i < DUNGEON_HEIGHT; i++) {
        for (int j = 0; j < DUNGEON_WIDTH; j++) {
            if (dungeon2.map[i][j].hardness == 0 && dungeon2.map[i][j].ch == ' ') {
                dungeon2.map[i][j] = CORRIDOR_CELL;
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
    
    uint32_t fileSize = 1708 + dungeon2.numRooms * 4 + dungeon2.numUpwardsStairs * 2 + dungeon2.numDownwardsStairs * 2;
    fileSize = htonl(fileSize);
    
    uint16_t numRooms = dungeon2.numRooms;
    numRooms = htons(numRooms);
    
    uint16_t numUpStrs = dungeon2.numUpwardsStairs;
    numUpStrs = htons(numUpStrs);
    uint16_t numDownStrs = dungeon2.numDownwardsStairs;
    numDownStrs = htons(numDownStrs);

    //Opens a path for the file
    char *path = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) +strlen("/.rlg327/dungeon") + 1));
    strcat(path, getenv("HOME"));
    strcat(path, "/.rlg327/dungeon");
    FILE *f = NULL;
    f = fopen(path, "w");
    if (f == NULL) {
        perror("Failed to open file");
        free(path);
        return;
    }

    //Writes type of file, version, size, location of player, and hardness
    fwrite(&FILE_MARKER, 12, 1, f);
    fwrite(&versionNum, sizeof(uint32_t), 1, f);
    fwrite(&fileSize, sizeof(uint32_t), 1, f);

    fwrite(&dungeon2.pc.posX, sizeof(uint8_t), 1, f);
    fwrite(&dungeon2.pc.posY, sizeof(uint8_t), 1, f);
    
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        for (int x = 0; x < DUNGEON_WIDTH; x++)
        {
            fwrite(&dungeon2.map[y][x].hardness, sizeof(uint8_t), 1, f);
        }
    }
    
    //Writes number of rooms, locations and sizes
    fwrite(&numRooms, sizeof(uint16_t), 1, f);
    for(int i = 0; i < dungeon2.numRooms; ++i){
        fwrite(&dungeon2.rooms[i].posX, sizeof(uint8_t), 1, f);
        fwrite(&dungeon2.rooms[i].posY, sizeof(uint8_t), 1, f);
        fwrite(&dungeon2.rooms[i].width, sizeof(uint8_t), 1, f);
        fwrite(&dungeon2.rooms[i].height, sizeof(uint8_t), 1, f);
    }

    // Writes numer of upward and downward stairs
    fwrite(&numUpStrs, sizeof(uint16_t), 1, f);
    for(int i = 0; i < dungeon2.numUpwardsStairs; ++i) {
        fwrite(&dungeon2.upwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fwrite(&dungeon2.upwardStairs[i].posY, sizeof(uint8_t), 1, f);
    }
    fwrite(&numDownStrs, sizeof(uint16_t), 1, f);
    for(int i = 0; i < dungeon2.numDownwardsStairs; ++i) {
        fwrite(&dungeon2.downwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fwrite(&dungeon2.downwardStairs[i].posY, sizeof(uint8_t), 1, f);
    }
    fclose(f);
    free(path);
    return;
}
