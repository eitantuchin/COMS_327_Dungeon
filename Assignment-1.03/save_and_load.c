//
//  save_and_load.c
//  Dungeon_Game_327
//
//  Created by Eitan Tuchin on 1/30/25.
//

#include "save_and_load.h"
#include "dungeon_path_finding.h"

dungeon_t dungeon;

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
        perror("Failed to open file");
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
