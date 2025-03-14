#include "dungeon_UI.h"
#include "priority_queue.h"

// our dungeon
dungeon_t dungeon;
int roomCounter = 0;
priority_queue_t *event_queue;
bool gameOver = false;
message_t gameMessage = {"", 0, false};
message_t directionMessage = {"", 0, true};
char *dirNames[] = {
    "NORTH", "NORTH-EAST", "EAST", "SOUTH-EAST", "SOUTH", "SOUTH-WEST", "WEST", "NORTH-WEST"
};
bool playerToMove = false;
int monsterListScrollOffset = 0;

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
    dungeon.numRooms = MIN_NUM_ROOMS;
    dungeon.rooms = (room_t *)malloc(sizeof(room_t) * dungeon.numRooms);
    dungeon.upwardStairs = (stair_t *)malloc(sizeof(stair_t) * 3);
    dungeon.downwardStairs = (stair_t *)malloc(sizeof(stair_t) * 3);
    dungeon.pc.speed = PC_SPEED;
    dungeon.pc.previousCharacter = '.'; // PC always starts on floor
    dungeon.monsters = (monster_t *)malloc(sizeof(monster_t) * MAX_NUM_MONSTERS);
    dungeon.modeType = PLAYER_CONTROL;
     
    srand((unsigned int) time(NULL));
    // Initialize ncurses
    initscr(); // Start ncurses mode
    cbreak();  // Disable line buffering
    noecho();  // Don't display typed characters
    keypad(stdscr, TRUE); // Enable special keys (like arrow keys)
    curs_set(0);  // Hide cursor
    nodelay(stdscr, TRUE);
    
    // make a dungeon but no saving or loading
    if (argv[1] == NULL) {
        dungeon.numMonsters = rand() % (10 - MIN_NUM_MONSTERS + 1) + MIN_NUM_MONSTERS;
        generateDungeon();
    }
    else if(strcmp(argv[1], "--nummon") == 0)
    {
        dungeon.numMonsters = atoi(argv[2]);
        generateDungeon();
    }
    else {
        printf("Unsupported command configuration: Please use either --nummon or no switch.\n");
        return 0;
    }

    calculateDistances(0);  // Non-tunneling
    calculateDistances(1);  // Tunneling

    // Initialize event queue
    event_queue = pq_create(MAX_NUM_MONSTERS * 10); // Adequate capacity

    // schedule initial PC event
    scheduleEvent(EVENT_PC, -1, 0);
    
    processEvents();
    
    // Schedule initial monster events
    for (int i = 0; i < dungeon.numMonsters; i++) {
        scheduleEvent(EVENT_MONSTER, i, 0);
    }
  
    // Main game loop
    gameOver = false;
    while (!gameOver) {
        checkKeyInput();
        if (dungeon.modeType == MONSTER_LIST) {
            displayMonsterList();
        }
        else if (dungeon.modeType == PLAYER_CONTROL) {
            //clear();
            printDungeon(0, 0);
            refresh();
        }
        checkGameConditions();
        if (!playerToMove) processEvents();
        checkGameConditions();
    }

    // Cleanup
    endwin();
    pq_destroy(event_queue);
    return 0;
}

char* getMonsterPositionString(int monsterIndex) {
    monster_t monster = dungeon.monsters[monsterIndex];
    int deltaX = monster.posX - dungeon.pc.posX;
    int deltaY = monster.posY - dungeon.pc.posY;

    char* directionX = (deltaX > 0) ? "East" : "West";
    char* directionY = (deltaY > 0) ? "South" : "North";

    int absDeltaX = abs(deltaX);
    int absDeltaY = abs(deltaY);

    char* positionString = malloc(50 * sizeof(char)); // Allocate enough space for the string

    if (absDeltaX == 0 && absDeltaY == 0) {
        snprintf(positionString, 50, "next to you");
    }
    else if (absDeltaX == 0) {
        snprintf(positionString, 50, "%d %s", absDeltaY, directionY);
    }
    else if (absDeltaY == 0) {
        snprintf(positionString, 50, "%d %s", absDeltaX, directionX);
    }
    else {
        snprintf(positionString, 50, "%d %s and %d %s", absDeltaX, directionX, absDeltaY, directionY);
    }

    return positionString;
}

void displayMonsterList(void) {
    //clear();

    int screenHeight, screenWidth;
    getmaxyx(stdscr, screenHeight, screenWidth); // Get the size of the terminal window

    mvprintw(0, 0, "Monster List:");
    mvprintw(1, 0, "Symbol | Position");

    int maxVisibleMonsters = screenHeight - 3; // Subtract 3 for header and instructions

    // Ensure the scroll offset is within valid bounds
    if (monsterListScrollOffset < 0) {
        monsterListScrollOffset = 0;
    }
    int maxScrollOffset = dungeon.numMonsters - maxVisibleMonsters;
    if (monsterListScrollOffset > maxScrollOffset) {
        monsterListScrollOffset = maxScrollOffset;
    }
    if (maxScrollOffset < 0) {
        monsterListScrollOffset = 0; // If the list fits entirely on the screen, reset the offset
    }

    for (int i = 0; i < maxVisibleMonsters && i + monsterListScrollOffset < dungeon.numMonsters; ++i) {
        int monsterIndex = i + monsterListScrollOffset;
        if (dungeon.monsters[monsterIndex].alive) {
            char* positionString = getMonsterPositionString(monsterIndex);
            mvprintw(i + 2, 0, "%c      | %s", dungeon.monsters[monsterIndex].MONSTER_CELL.ch, positionString);
            free(positionString);
        }
        else {
            mvprintw(i + 2, 0, "%c      | %s", dungeon.monsters[monsterIndex].MONSTER_CELL.ch, "Killed");
        }
    }

    mvprintw(screenHeight - 1, 0, "Press ESC to return to the game. Use UP/DOWN arrows to scroll.");

    refresh();
}

void scheduleEvent(event_type_t type, int index, int currentTurn) {
    event_t *newEvent = malloc(sizeof(event_t));
    newEvent->type = type;
    newEvent->index = index;
    if (type == EVENT_PC) {
        newEvent->turn = currentTurn + (1000 / PC_SPEED);
    }
    else {
        newEvent->turn = currentTurn + (1000 / dungeon.monsters[index].speed);
    }
    pq_insert(event_queue, newEvent->index, 0, newEvent->turn);
    free(newEvent);
}

void processEvents(void) {
    char message[100];
        pq_node_t node = pq_extract_min(event_queue); // extract the first value in the Q
        if (node.x == -1) { // EVENT_PC
            playerToMove = true; // this stops the processing of events until the player moves again
            scheduleEvent(EVENT_PC, -1, node.priority);
            snprintf(message, sizeof(message), "Your turn to move!");
            displayMessage(message);
        }
        else if (dungeon.monsters[node.x].alive) {
            moveMonster(node.x);
            usleep(100000);
            scheduleEvent(EVENT_MONSTER, node.x, node.priority); // keep moving the monster
            snprintf(message, sizeof(message), "The monsters are moving...");
            displayMessage(message);
        }
    
}

void displayMessage(const char *message) {
    if (strstr(message, "Facing:")) { // if the string contains "Facing:"
        strncpy(directionMessage.message, message, sizeof(directionMessage.message) - 1); // Copy message safely
        directionMessage.message[sizeof(directionMessage.message) - 1] = '\0'; // Ensure null termination
    }
    else {
        strncpy(gameMessage.message, message, sizeof(gameMessage.message) - 1); // Copy message safely
        gameMessage.message[sizeof(gameMessage.message) - 1] = '\0'; // Ensure null termination
        gameMessage.startTime = time(NULL);
        gameMessage.visible = true;
    }
}

void drawMessage(void) {
    if (gameMessage.visible) {
        move(0, 0);
        clrtoeol();
        mvprintw(0, 0, "%s", gameMessage.message); // Print at the beginning of the line (left-aligned)
    }
    size_t len = strlen(directionMessage.message);
    size_t x = DUNGEON_WIDTH - len - 1; // Calculate starting x for right alignment
    mvprintw(0, (int) x, "%s", directionMessage.message); // Print right-aligned
}

void checkKeyInput(void) {
    int key = getch();
    switch(key) {
        case KEY_UP: // move up
        case '8':
        case 'k':
        case KEY_DOWN: // move down
        case '2':
        case 'j':
        case KEY_LEFT: // move left
        case '4':
        case 'h':
        case KEY_RIGHT: // move right
        case '6':
        case 'l':
        case KEY_HOME: // up-left
        case '7':
        case 'y':
        case KEY_PPAGE: // up-right
        case '9':
        case 'u':
        case KEY_NPAGE: // down-right
        case '3':
        case 'n':
        case KEY_END: // down-left
        case '1':
        case 'b':
        case KEY_B2: // rest
        case '5':
        case ' ':
        case '.':
            if (playerToMove && dungeon.modeType == PLAYER_CONTROL) movePlayer(key);
            if (dungeon.modeType == MONSTER_LIST) {
                if (key == KEY_UP) {
                    clear();
                    monsterListScrollOffset--;
                    displayMonsterList();
                }
                else if (key == KEY_DOWN) {
                    clear();
                    monsterListScrollOffset++;
                    displayMonsterList();
                }
            }
            break;
        case 'q': // quit the game
        case 'Q':
            gameOver = true;
            endwin();
            printf("\nYou quit the game!\n");
            break;
        case 'z': // PC can attack 1 or 2 spaces away
            attack(1); attack(2); break;
        case 'V': // counter-clockwise
            changeDirection(false, false); break;
        case 'v': // clocckwise
            changeDirection(true, false); break;
        case '<':
        case '>':
            useStairs(key); break;
        case 'm':
            if (playerToMove) {
                clear(); dungeon.modeType = MONSTER_LIST; monsterListScrollOffset = 0;
            }
            break;
        case 27: // escape key
            clear(); dungeon.modeType = PLAYER_CONTROL;
            break;
    }
    
}

void useStairs(int key) {
   if (dungeon.pc.previousCharacter == key) {
        resetDungeonLevel();
        char message[100];
        if (key == '<') snprintf(message, sizeof(message), "Moved up a dungeon level!");
        else snprintf(message, sizeof(message), "Moved down a dungeon level!");
        displayMessage(message);
    }
}

void generateDungeon(void) {
    initImmutableRock();
    addRooms();
    addCorridors();
    addStairs();
    initPCPosition();
    initMonsters();
}

void resetDungeonLevel(void) {
    // Reset counters and any global variables
    roomCounter = 0;
    dungeon.numRooms = MIN_NUM_ROOMS;
    dungeon.rooms = (room_t *)malloc(sizeof(room_t) * dungeon.numRooms);
    dungeon.upwardStairs = (stair_t *)malloc(sizeof(stair_t) * 3);
    dungeon.numUpwardsStairs = 0;
    dungeon.numDownwardsStairs = 0;
    dungeon.downwardStairs = (stair_t *)malloc(sizeof(stair_t) * 3);
    dungeon.pc.previousCharacter = '.'; // PC always starts on floor
    dungeon.monsters = (monster_t *)malloc(sizeof(monster_t) * MAX_NUM_MONSTERS);
    
    // (If your dungeon.map is allocated dynamically, you might free it and reallocate it here as well.)
    event_queue = pq_create(MAX_NUM_MONSTERS * 10);
    // Now generate a new dungeon level
    generateDungeon();
    
    // Recalculate pathfinding distances for both tunneling and non-tunneling monsters
    calculateDistances(0);
    calculateDistances(1);
    
    // Schedule events for the player and monsters as needed
    scheduleEvent(EVENT_PC, -1, 0);
    processEvents();
    for (int i = 0; i < dungeon.numMonsters; i++) {
        scheduleEvent(EVENT_MONSTER, i, 0);
    }
    char message[100];
    snprintf(message, sizeof(message), "Your turn to move!");
    displayMessage(message);
}



void changeDirection(bool clockwise, bool justChangeText) {
    // Define the order of directions (clockwise and counter-clockwise)
    clear();
    direction_t directions[] = {
        UP, UP_RIGHT, RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT, LEFT, UP_LEFT
    };

    int currentDirIndex = 0;
    // Find the index of the current direction
    for (int i = 0; i < 8; i++) {
        if (dungeon.pc.currentDirection == directions[i]) {
            currentDirIndex = i;
            break;
        }
    }

    char message[100];
    if (!justChangeText) {
        int newDirIndex = (currentDirIndex + (clockwise ? 1 : -1) + 8) % 8;
        dungeon.pc.currentDirection = directions[newDirIndex];
        snprintf(message, sizeof(message), "Facing: %s", dirNames[newDirIndex]);
    }
    else {
        snprintf(message, sizeof(message), "Facing: %s", dirNames[currentDirIndex]);
    }
    displayMessage(message);
    refresh();
}


void attack(int distance) {
    // Determine the direction the player is facing and the *adjacent* directions
    int attackX = dungeon.pc.posX;
    int attackY = dungeon.pc.posY;
    int oldX = attackX;
    int oldY = attackY;

    direction_t directionsToCheck[3]; // Array to hold the directions to check

    switch (dungeon.pc.currentDirection) {
        case UP:
            directionsToCheck[0] = UP;
            directionsToCheck[1] = UP_LEFT;
            directionsToCheck[2] = UP_RIGHT;
            break;
        case DOWN:
            directionsToCheck[0] = DOWN;
            directionsToCheck[1] = DOWN_LEFT;
            directionsToCheck[2] = DOWN_RIGHT;
            break;
        case LEFT:
            directionsToCheck[0] = LEFT;
            directionsToCheck[1] = UP_LEFT;
            directionsToCheck[2] = DOWN_LEFT;
            break;
        case RIGHT:
            directionsToCheck[0] = RIGHT;
            directionsToCheck[1] = UP_RIGHT;
            directionsToCheck[2] = DOWN_RIGHT;
            break;
        case UP_LEFT:
            directionsToCheck[0] = UP_LEFT;
            directionsToCheck[1] = UP;
            directionsToCheck[2] = LEFT;
            break;
        case UP_RIGHT:
            directionsToCheck[0] = UP_RIGHT;
            directionsToCheck[1] = UP;
            directionsToCheck[2] = RIGHT;
            break;
        case DOWN_LEFT:
            directionsToCheck[0] = DOWN_LEFT;
            directionsToCheck[1] = DOWN;
            directionsToCheck[2] = LEFT;
            break;
        case DOWN_RIGHT:
            directionsToCheck[0] = DOWN_RIGHT;
            directionsToCheck[1] = DOWN;
            directionsToCheck[2] = RIGHT;
            break;
        default:
            return; // Invalid direction
    }

    for (int i = 0; i < 3; i++) { // Check all three directions
        attackX = dungeon.pc.posX; // Reset attack position for each direction
        attackY = dungeon.pc.posY;

        switch (directionsToCheck[i]) {
            case UP:
                attackY -= distance;
                break;
            case DOWN:
                attackY += distance;
                break;
            case LEFT:
                attackX -= distance;
                break;
            case RIGHT:
                attackX += distance;
                break;
            case UP_LEFT:
                attackY -= distance;
                attackX -= distance;
                break;
            case UP_RIGHT:
                attackY -= distance;
                attackX += distance;
                break;
            case DOWN_LEFT:
                attackY += distance;
                attackX -= distance;
                break;
            case DOWN_RIGHT:
                attackY += distance;
                attackX += distance;
                break;
            default:
                break; // Invalid direction (shouldn't happen)
        }

        // Check if the position is valid and contains a monster
        if (attackY >= 0 && attackY < DUNGEON_HEIGHT && attackX >= 0 && attackX < DUNGEON_WIDTH) {
            for (int j = 0; j < dungeon.numMonsters; j++) {
                monster_t *monster = &dungeon.monsters[j];
                if (monster->alive && monster->posX == attackX && monster->posY == attackY) {
                    // Monster is killed (same logic as before)
                    monster->alive = false;
                    dungeon.map[attackY][attackX] = PLAYER_CELL;
                    dungeon.map[oldY][oldX] = (cell_t){dungeon.pc.previousCharacter, 0};
                    dungeon.pc.posX = attackX;
                    dungeon.pc.posY = attackY;
                    dungeon.pc.previousCharacter = monster->previousCharacter;
                    char message[100];
                    snprintf(message, sizeof(message), "You killed monster %c!", monster->MONSTER_CELL.ch);
                    displayMessage(message);
                    return;
                }
            }
        }
    }
}

void movePlayer(int key) {
    // Check boundaries and valid movement
    int newX = dungeon.pc.posX;
    int newY = dungeon.pc.posY;
    int oldX = newX;
    int oldY = newY;
    switch (key) {
        case KEY_UP: case '8': case 'k':
            newY--; dungeon.pc.currentDirection = UP;  break;
        case KEY_DOWN: case '2': case 'j':
            newY++; dungeon.pc.currentDirection = DOWN; break;
        case KEY_LEFT: case '4': case 'h':
            newX--; dungeon.pc.currentDirection = LEFT; break;
        case KEY_RIGHT: case '6': case 'l':
            newX++; dungeon.pc.currentDirection = RIGHT; break;
        case KEY_HOME: case '7': case 'y':
            newX--; newY--; dungeon.pc.currentDirection = UP_LEFT; break;
        case KEY_PPAGE: case '9': case 'u':
            newX++; newY--; dungeon.pc.currentDirection = UP_RIGHT; break;
        case KEY_NPAGE: case '3': case 'n':
            newX++; newY++; dungeon.pc.currentDirection = DOWN_RIGHT; break;
        case KEY_END: case '1': case 'b':
            newX--; newY++; dungeon.pc.currentDirection = DOWN_LEFT; break;
        case KEY_B2: case '5': case ' ': case '.':
            // rest
            break;
    }
    
    if (newY >= 0 && newY < DUNGEON_HEIGHT &&
        newX >= 0 && newX < DUNGEON_WIDTH &&
        dungeon.map[newY][newX].hardness == 0) {
        //gameOver = true;
        switch(dungeon.pc.previousCharacter) {
            case '#':
                dungeon.map[oldY][oldX] = CORRIDOR_CELL;
                break;
            case '.':
               dungeon.map[oldY][oldX] = ROOM_CELL;
               break;
            case '<':
               dungeon.map[oldY][oldX] = UPWARD_STAIRS_CELL;
               break;
            case '>':
               dungeon.map[oldY][oldX] = DOWNWARD_STAIRS_CELL;
               break;
        }
        
        dungeon.pc.previousCharacter = dungeon.map[newY][newX].ch;
        dungeon.pc.posX = newX;
        dungeon.pc.posY = newY;
        dungeon.map[newY][newX] = PLAYER_CELL;
        changeDirection(false, true);
        playerToMove = false;
    }
    else {
        char message[100];
        snprintf(message, sizeof(message), "There's a wall there!");
        displayMessage(message);
    }
    calculateDistances(0); // recalculate pathfinding maps after moving player
    calculateDistances(1);
}

int sign(int x) {
    return (x > 0) - (x < 0);
}

void moveMonster(int index) {
    // Do nothing if monster is not alive.
    if (!dungeon.monsters[index].alive)
        return;
        
    monster_t *m = &dungeon.monsters[index];
    int oldX = m->posX;
    int oldY = m->posY;
    int newX = oldX;
    int newY = oldY;

    // For tunneling purposes.
    bool isTunneling = (m->monsterBits & (1 << 2)) != 0;
    
    // Get the monster's symbol.
    char sym = m->MONSTER_CELL.ch;
    
    // For erratic monsters, we give a 50% chance to move randomly.
    bool erraticRandom = false;
    if (sym == '8' || sym == '9' || sym == 'a' || sym == 'b' ||
        sym == 'c' || sym == 'd' || sym == 'e' || sym == 'f') {
        erraticRandom = (rand() % 2 == 0);
    }
    
    switch(sym) {
        case '0': {
            // Non-erratic, non-tunneling, non-telepathy, non-intelligent.
            if (!hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY))
                return;
            newX = oldX + sign(dungeon.pc.posX - oldX);
            newY = oldY + sign(dungeon.pc.posY - oldY);
            break;
        }
        case '1': {
            // Non-erratic, non-tunneling, non-telepathy, intelligent.
            int targetX, targetY;
            if (hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY)) {
                targetX = dungeon.pc.posX;
                targetY = dungeon.pc.posY;
                m->lastSeenPCX = targetX;
                m->lastSeenPCY = targetY;
            } else {
                if (m->lastSeenPCX != -1 && m->lastSeenPCY != -1) {
                    targetX = m->lastSeenPCX;
                    targetY = m->lastSeenPCY;
                } else {
                    return;
                }
            }
            int best = INT_MAX;
            int (*dist)[DUNGEON_WIDTH] = dungeon.nonTunnelingMap;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = oldX + dx;
                    int ny = oldY + dy;
                    if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                        continue;
                    if (dungeon.map[ny][nx].hardness > 0)
                        continue;
                    if (dist[ny][nx] < best) {
                        best = dist[ny][nx];
                        newX = nx;
                        newY = ny;
                    }
                }
            }
            break;
        }
        case '2': {
            // Non-erratic, non-tunneling, telepathy, non-intelligent.
            newX = oldX + sign(dungeon.pc.posX - oldX);
            newY = oldY + sign(dungeon.pc.posY - oldY);
            break;
        }
        case '3': {
            // Non-erratic, non-tunneling, telepathy, intelligent.
            int best = INT_MAX;
            int (*dist)[DUNGEON_WIDTH] = dungeon.nonTunnelingMap;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = oldX + dx;
                    int ny = oldY + dy;
                    if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                        continue;
                    if (dist[ny][nx] < best) {
                        best = dist[ny][nx];
                        newX = nx;
                        newY = ny;
                    }
                }
            }
            m->lastSeenPCX = dungeon.pc.posX;
            m->lastSeenPCY = dungeon.pc.posY;
            break;
        }
        case '4': {
            // Non-erratic, tunneling, non-telepathy, non-intelligent.
            if (!hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY))
                return;
            newX = oldX + sign(dungeon.pc.posX - oldX);
            newY = oldY + sign(dungeon.pc.posY - oldY);
            break;
        }
        case '5': {
            // Non-erratic, tunneling, non-telepathy, intelligent.
            int targetX, targetY;
            if (hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY)) {
                targetX = dungeon.pc.posX;
                targetY = dungeon.pc.posY;
                m->lastSeenPCX = targetX;
                m->lastSeenPCY = targetY;
            } else {
                if (m->lastSeenPCX != -1 && m->lastSeenPCY != -1) {
                    targetX = m->lastSeenPCX;
                    targetY = m->lastSeenPCY;
                } else {
                    return;
                }
            }
            int best = INT_MAX;
            int (*dist)[DUNGEON_WIDTH] = dungeon.tunnelingMap;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = oldX + dx;
                    int ny = oldY + dy;
                    if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                        continue;
                    if (dist[ny][nx] < best) {
                        best = dist[ny][nx];
                        newX = nx;
                        newY = ny;
                    }
                }
            }
            break;
        }
        case '6': {
            // Non-erratic, tunneling, telepathy, non-intelligent.
            newX = oldX + sign(dungeon.pc.posX - oldX);
            newY = oldY + sign(dungeon.pc.posY - oldY);
            break;
        }
        case '7': {
            // Non-erratic, tunneling, telepathy, intelligent.
            int best = INT_MAX;
            int (*dist)[DUNGEON_WIDTH] = dungeon.tunnelingMap;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = oldX + dx;
                    int ny = oldY + dy;
                    if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                        continue;
                    if (dist[ny][nx] < best) {
                        best = dist[ny][nx];
                        newX = nx;
                        newY = ny;
                    }
                }
            }
            m->lastSeenPCX = dungeon.pc.posX;
            m->lastSeenPCY = dungeon.pc.posY;
            break;
        }
        case '8': {
            // Erratic, non-tunneling, non-telepathy, non-intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                if (!hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY))
                    return;
                newX = oldX + sign(dungeon.pc.posX - oldX);
                newY = oldY + sign(dungeon.pc.posY - oldY);
            }
            break;
        }
        case '9': {
            // Erratic, non-tunneling, non-telepathy, intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                int targetX, targetY;
                if (hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY)) {
                    targetX = dungeon.pc.posX;
                    targetY = dungeon.pc.posY;
                    m->lastSeenPCX = targetX;
                    m->lastSeenPCY = targetY;
                } else {
                    if (m->lastSeenPCX != -1 && m->lastSeenPCY != -1) {
                        targetX = m->lastSeenPCX;
                        targetY = m->lastSeenPCY;
                    } else {
                        return;
                    }
                }
                int best = INT_MAX;
                int (*dist)[DUNGEON_WIDTH] = dungeon.nonTunnelingMap;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = oldX + dx;
                        int ny = oldY + dy;
                        if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                            continue;
                        if (dist[ny][nx] < best) {
                            best = dist[ny][nx];
                            newX = nx;
                            newY = ny;
                        }
                    }
                }
            }
            break;
        }
        case 'a': {
            // Erratic, non-tunneling, telepathy, non-intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                newX = oldX + sign(dungeon.pc.posX - oldX);
                newY = oldY + sign(dungeon.pc.posY - oldY);
            }
            break;
        }
        case 'b': {
            // Erratic, non-tunneling, telepathy, intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                int best = INT_MAX;
                int (*dist)[DUNGEON_WIDTH] = dungeon.nonTunnelingMap;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = oldX + dx;
                        int ny = oldY + dy;
                        if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                            continue;
                        if (dist[ny][nx] < best) {
                            best = dist[ny][nx];
                            newX = nx;
                            newY = ny;
                        }
                    }
                }
                m->lastSeenPCX = dungeon.pc.posX;
                m->lastSeenPCY = dungeon.pc.posY;
            }
            break;
        }
        case 'c': {
            // Erratic, tunneling, non-telepathy, non-intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                if (!hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY))
                    return;
                newX = oldX + sign(dungeon.pc.posX - oldX);
                newY = oldY + sign(dungeon.pc.posY - oldY);
            }
            break;
        }
        case 'd': {
            // Erratic, tunneling, non-telepathy, intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                int targetX, targetY;
                if (hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY)) {
                    targetX = dungeon.pc.posX;
                    targetY = dungeon.pc.posY;
                    m->lastSeenPCX = targetX;
                    m->lastSeenPCY = targetY;
                } else {
                    if (m->lastSeenPCX != -1 && m->lastSeenPCY != -1) {
                        targetX = m->lastSeenPCX;
                        targetY = m->lastSeenPCY;
                    } else {
                        return;
                    }
                }
                int best = INT_MAX;
                int (*dist)[DUNGEON_WIDTH] = dungeon.tunnelingMap;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = oldX + dx;
                        int ny = oldY + dy;
                        if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                            continue;
                        if (dist[ny][nx] < best) {
                            best = dist[ny][nx];
                            newX = nx;
                            newY = ny;
                        }
                    }
                }
            }
            break;
        }
        case 'e': {
            // Erratic, tunneling, telepathy, non-intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                newX = oldX + sign(dungeon.pc.posX - oldX);
                newY = oldY + sign(dungeon.pc.posY - oldY);
            }
            break;
        }
        case 'f': {
            // Erratic, tunneling, telepathy, intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                int best = INT_MAX;
                int (*dist)[DUNGEON_WIDTH] = dungeon.tunnelingMap;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = oldX + dx;
                        int ny = oldY + dy;
                        if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                            continue;
                        if (dist[ny][nx] < best) {
                            best = dist[ny][nx];
                            newX = nx;
                            newY = ny;
                        }
                    }
                }
                m->lastSeenPCX = dungeon.pc.posX;
                m->lastSeenPCY = dungeon.pc.posY;
            }
            break;
        }
        default:
            return; // If symbol is not recognized, do nothing.
    }

    // Finally, update the monster's position.
    updateMonsterPosition(index, oldX, oldY, newX, newY, m, isTunneling);
}

bool hasLineOfSight(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int x = x1;
    int y = y1;
    int n = 1 + dx + dy;
    int xInc = (x2 > x1) ? 1 : -1;
    int yInc = (y2 > y1) ? 1 : -1;
    int t = dx - dy;

    for (; n > 0; n--) {
        if (dungeon.map[y][x].hardness > 0 && dungeon.map[y][x].hardness < IMMUTABLE_ROCK_CELL.hardness) {
            return false; // Blocked by a wall
        }
        if (t > 0) {
            x += xInc;
            t -= 2 * dy;
        } else {
            y += yInc;
            t += 2 * dx;
        }
    }
    return true; // Line of sight is clear
}


void updateMonsterPosition(int index, int oldX, int oldY, int newX, int newY, monster_t *m, bool isTunneling) {
    // performing the monster's move
    if (newX >= 1 && newX < DUNGEON_WIDTH - 1 && newY >= 1 && newY < DUNGEON_HEIGHT - 1) {
        bool collision = false;
        int defenderIndex = -1;
        for (int i = 0; i < dungeon.numMonsters; i++) {
           if (i != index &&  // Don't check against itself
               dungeon.monsters[i].alive &&
               dungeon.monsters[i].posX == newX &&
               dungeon.monsters[i].posY == newY) {
               collision = true;
               defenderIndex = i;
               break;
           }
       }
        if (!collision && dungeon.map[newY][newX].hardness == 0) { // no monster fighting and not a rock cell
            dungeon.map[oldY][oldX] = (cell_t){m->previousCharacter, 0}; // Restore the old cell
            m->previousCharacter = dungeon.map[newY][newX].ch;
            m->posX = newX;
            m->posY = newY;
            dungeon.map[newY][newX] = m->MONSTER_CELL;

        }
        else if (!collision && dungeon.map[newY][newX].hardness > 0 && dungeon.map[newY][newX].hardness < IMMUTABLE_ROCK_CELL.hardness && isTunneling) { // we have met mutable rock and we are a tunneler
            dungeon.map[newY][newX].hardness -= 85; // mining rock
            calculateDistances(1);  // recalculate tunneling distances
            if (dungeon.map[newY][newX].hardness <= 0) {
                dungeon.map[newY][newX] = CORRIDOR_CELL;

                calculateDistances(0); // Recalculate non-tunneling distances
                calculateDistances(1); // Recalculate tunneling distances

                // After turning rock to corridor, the monster should move there
                dungeon.map[oldY][oldX] = (cell_t){m->previousCharacter, 0};
                m->previousCharacter = dungeon.map[newY][newX].ch;
                m->posX = newX;
                m->posY = newY;
                dungeon.map[newY][newX] = m->MONSTER_CELL;
            }
            
        }
        else if (collision) {
            if (defenderIndex != -1) { // kill the defender
                dungeon.monsters[defenderIndex].alive = false;
                dungeon.map[newY][newX] = m->MONSTER_CELL;
                dungeon.map[oldY][oldX] = (cell_t){m->previousCharacter, 0};
                m->posX = newX;
                m->posY = newY;
                m->previousCharacter = dungeon.monsters[defenderIndex].previousCharacter;
            }
        }
    }
}

void checkGameConditions(void) {
    // Check PC death
    for (int i = 0; i < dungeon.numMonsters; ++i) {
        if (dungeon.monsters[i].alive &&
            dungeon.monsters[i].posY == dungeon.pc.posY &&
            dungeon.monsters[i].posX == dungeon.pc.posX) {
            gameOver = true;
            endwin();
            printf("\nGame Over - You were killed by monster %c!", dungeon.monsters[i].MONSTER_CELL.ch);
            return;
        }
    }
    
    // Check remaining monsters
    bool monstersAlive = false;
    for (int i = 0; i < dungeon.numMonsters; ++i) {
        if (dungeon.monsters[i].alive) {
            monstersAlive = true;
            break;
        }
    }
    
    if (!monstersAlive) {
        gameOver = true;
        endwin();
        printf("\nCongratulations! All monsters have been defeated!\n");
    }
    
    if (gameMessage.visible && difftime(time(NULL), gameMessage.startTime) >= 4) {
        if (strstr(gameMessage.message, "Your turn to move!")) {
            return;
        }
        else if (strstr(gameMessage.message, "Game Over")) {
            gameOver = true;
            endwin();
            printf("\nGame Over - You were killed by a monster!\n");
            return;
        }
        else {
            gameMessage.visible = false;
        }
    }
}

void initMonsters(void) {
    // add monsters to array
    monster_t monster;
    for (int i = 0; i < dungeon.numMonsters; ++i) {
        bool isErratic = rand() % 2 == 1;
        bool isTunneling = rand() % 2 == 1;
        bool isIntelligent = rand() % 2 == 1;
        bool isTelepathic = rand() % 2 == 1;
        uint8_t monsterBits = 0;
        if (isIntelligent) monsterBits |= (1 << 0); // Set bit 0 for intelligence
        if (isTelepathic) monsterBits |= (1 << 1);  // Set bit 1 for telepathy
        if (isTunneling) monsterBits |= (1 << 2);   // Set bit 2 for tunneling
        if (isErratic) monsterBits |= (1 << 3);     // Set bit 3 for erratic behavior
        monster.monsterBits = monsterBits;
        monster.speed = rand() % 16 + 5;
        monster.MONSTER_CELL.hardness = 0;
        monster.MONSTER_CELL.ch = "0123456789abcdef"[monsterBits];
        monster.alive = true;
        dungeon.monsters[i].lastSeenPCX = -1; // Initialize last seen position
        dungeon.monsters[i].lastSeenPCY = -1;
        dungeon.monsters[i] = monster;
    }
    
    // initialize positions of monsters
    
    for (int i = 0; i < dungeon.numMonsters; ++i) {
        bool foundPosition = false;
        while (!foundPosition) {
            int randY = rand() % DUNGEON_HEIGHT - 2;
            int randX = rand() % DUNGEON_WIDTH - 2;
            bool pcFarAwayEnough = checkMonsterPlacementToPC(randX, randY);
            if ((dungeon.map[randY][randX].ch == '.' || dungeon.map[randY][randX].ch == '#') && pcFarAwayEnough) {
                dungeon.monsters[i].posX = randX;
                dungeon.monsters[i].posY = randY;
                dungeon.monsters[i].previousCharacter = dungeon.map[randY][randX].ch;
                dungeon.map[randY][randX] = dungeon.monsters[i].MONSTER_CELL;
                foundPosition = true;
            }
        }
    }
    
}

bool checkMonsterPlacementToPC(int randX, int randY) {
    for (int i = randY - 2; i < randY + 2; ++i) {
        for (int j = randX - 2; j < randX + 2; ++j) {
            if (dungeon.map[i][j].ch == '@') {
                return false;
            }
        }
    }
    return true;
}

void printDungeon(int showDist, int tunneling) {
    // Print the top border (now with space for the message)
    move(1, 0); // Move cursor to the second row (leaving the first for the message)
    for (int y = 0; y < DUNGEON_HEIGHT; ++y) { // Start from y = 1 to leave space for message
        move(y + 1, 0); // Adjust row position for the message line
        for (int x = 0; x < DUNGEON_WIDTH -1; ++x) {
            addch(dungeon.map[y][x].ch);
        }
    }
    drawMessage();
}


/*
Use dijiktra's to calculate path finding
*/
void calculateDistances(int mapNum) {
    priority_queue_t *pq = pq_create(DUNGEON_HEIGHT * DUNGEON_WIDTH);
    int (*dist)[DUNGEON_WIDTH];
    if (mapNum == 0) dist = dungeon.nonTunnelingMap;
    else if (mapNum == 1) dist = dungeon.tunnelingMap;

    // Initialize distances.  initialize to -1 to represent "infinity"
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            dist[y][x] = -1;
        }
    }

    // Start with PC position
    int pc_y = dungeon.pc.posY;
    int pc_x = dungeon.pc.posX;
    dist[pc_y][pc_x] = 0;
    pq_insert(pq, pc_x, pc_y, 0);

    while (!pq_is_empty(pq)) {
        pq_node_t node = pq_extract_min(pq);
        int x = node.x;
        int y = node.y;

        // Current cell
        cell_t currentCell = dungeon.map[y][x]; 

        // Check all 8 neighbors
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                int nx = x + dx;
                int ny = y + dy;

                // Bounds check
                if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT) continue;

                // Check passability
                cell_t neighborCell = dungeon.map[ny][nx];
                if (mapNum == 0 && neighborCell.hardness > 0) continue;
                if (neighborCell.hardness == 255) continue;

                // Calculate weight 
                int weight = 1;
                if (mapNum == 1 && currentCell.hardness > 0) { // Current cell's hardness
                    weight = 1 + (currentCell.hardness / 85);
                }

                // Update distance.
                int new_dist = dist[y][x] + weight;
                if (dist[ny][nx] == -1 || new_dist < dist[ny][nx]) {
                    dist[ny][nx] = new_dist;
                    pq_insert(pq, nx, ny, new_dist);
                }
            }
        }
    }
    pq_destroy(pq);
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
        roomCentroids[i][0] = dungeon.rooms[i].posX + dungeon.rooms[i].width / 2;
        roomCentroids[i][1] = dungeon.rooms[i].posY + dungeon.rooms[i].height / 2;
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

        if (dungeon.map[y][x].ch == ROCK_CELL.ch &&
            dungeon.map[y][x].hardness < 255) {
            dungeon.map[y][x] = CORRIDOR_CELL;
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
            int randX = rand() % (DUNGEON_WIDTH - 2) + 1;
            int randY = rand() % (DUNGEON_HEIGHT - 2) + 1;
            int randHeight = rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1) + MIN_ROOM_HEIGHT;
            int randWidth = rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1) + MIN_ROOM_WIDTH;

            if (randX + randWidth <= DUNGEON_WIDTH - 1 && randY + randHeight <= DUNGEON_HEIGHT - 1)
            {
                bool validRoomPositionFound = true;
                // ensure that we don't create the room where immutable rock lies
                for (int y = randY - 2; y < randY + randHeight + 2; ++y)
                {
                    for (int x = randX - 2; x < randX + randWidth + 2; ++x)
                    {
                        if (dungeon.map[y][x].ch != ROCK_CELL.ch)
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

                    for (int y = randY; y < randY + randHeight; ++y)
                    {
                        for (int x = randX; x < randX + randWidth; ++x)
                        {
                            dungeon.map[y][x] = ROOM_CELL;
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
    if (dungeon.map[dungeon.pc.posY][dungeon.pc.posX].ch != '<' ||
        dungeon.map[dungeon.pc.posY][dungeon.pc.posX].ch != '>')
    {
        dungeon.map[dungeon.pc.posY][dungeon.pc.posX] = PLAYER_CELL;
    }
    else
    {
        initPCPosition(); // recursively call to get another random position because we cannot have the PC position override a staircase
    }
}

/*
 Adds the staircases to the dungeon and ensures no room contains multiple stairs and that we have at least one type of each staircase
 */
void addStairs(void)
{
    int numStairs = rand() % (MAX_NUM_STAIRS - MIN_NUM_STAIRS + 1) + MIN_NUM_STAIRS;
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
        int randomX = room.posX + rand() % room.width;
        int randomY = room.posY + rand() % room.height;
        stair_t stair;
        stair.posX = randomX;
        stair.posY = randomY;
        if (dungeon.numUpwardsStairs + dungeon.numDownwardsStairs == 0)
        {
            dungeon.map[randomY][randomX] = UPWARD_STAIRS_CELL;
            dungeon.upwardStairs[dungeon.numUpwardsStairs] = stair;
            dungeon.numUpwardsStairs++;
        }
        // ensure at least one downwards stairs is inside the dungeon
        else if (dungeon.numUpwardsStairs + dungeon.numDownwardsStairs == 1)
        {
            dungeon.map[randomY][randomX] = DOWNWARD_STAIRS_CELL;
            dungeon.downwardStairs[dungeon.numDownwardsStairs] = stair;
            dungeon.numDownwardsStairs++;
        }
        else
        {
            int randomStairCaseDirection = rand() % 2 + 1;
            if (randomStairCaseDirection == 1) {
                dungeon.map[randomY][randomX] = UPWARD_STAIRS_CELL;
                dungeon.upwardStairs[dungeon.numUpwardsStairs] = stair;
                dungeon.numUpwardsStairs++;
            }
            else {
                dungeon.map[randomY][randomX] = DOWNWARD_STAIRS_CELL;
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
    
    /*
    if (f == NULL) {
        perror("Failed to open file");
        free(path);
        return;
    }
    */

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
    printf("File size: %u bytes\n", fileSize);

    // Read player position
    fread(&dungeon.pc.posX, sizeof(uint8_t), 1, f);
    fread(&dungeon.pc.posY, sizeof(uint8_t), 1, f);

    
    dungeon.map[dungeon.pc.posY][dungeon.pc.posX] = PLAYER_CELL;

    // Load dungeon map hardness
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            fread(&dungeon.map[y][x].hardness, sizeof(uint8_t), 1, f);
        }
    }

    // Read number of rooms and validate it
    fread(&dungeon.numRooms, sizeof(uint16_t), 1, f);
    dungeon.numRooms = htons(dungeon.numRooms);


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


        // Mark room cells on the map
        for (int y = dungeon.rooms[i].posY; y < dungeon.rooms[i].posY + dungeon.rooms[i].height; y++) {
            for (int x = dungeon.rooms[i].posX; x < dungeon.rooms[i].posX + dungeon.rooms[i].width; x++) {
                /*if (n >= DUNGEON_WIDTH || m >= DUNGEON_HEIGHT) {
                    printf("Room exceeds dungeon bounds: Room %d at (%d,%d)\n", i, n, m);
                    fclose(f);
                    free(path);
                    return;
                }
                 */
                if (dungeon.map[y][x].ch != '@') {
                    dungeon.map[y][x] = ROOM_CELL;
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
        dungeon.map[dungeon.upwardStairs[i].posY][dungeon.upwardStairs[i].posX] = UPWARD_STAIRS_CELL;
    }
    
    fread(&dungeon.numDownwardsStairs, sizeof(uint16_t), 1, f);
    dungeon.numDownwardsStairs = htons(dungeon.numDownwardsStairs);
    dungeon.downwardStairs = malloc(sizeof(stair_t) * dungeon.numDownwardsStairs);
    for (int i = 0; i < dungeon.numDownwardsStairs; i++) {
        fread(&dungeon.downwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon.downwardStairs[i].posY, sizeof(uint8_t), 1, f);
        dungeon.map[dungeon.downwardStairs[i].posY][dungeon.downwardStairs[i].posX] = DOWNWARD_STAIRS_CELL;
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