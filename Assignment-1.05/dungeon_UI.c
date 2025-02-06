#include "dungeon_UI.h"
#include "priority_queue.h"

// our dungeon
dungeon_t dungeon;
int roomCounter = 0;
priority_queue_t *event_queue;
bool gameOver = false;
struct timeval lastMoveTime;
message_t gameMessage = {"", 0, false};
message_t directionMessage = {"", 0, true};
char *dirNames[] = {
    "NORTH", "NORTH-EAST", "EAST", "SOUTH-EAST", "SOUTH", "SOUTH-WEST", "WEST", "NORTH-WEST"
};
bool playerToMove = false;

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
        checkGameConditions();
        if (!playerToMove) processEvents();
        checkGameConditions();
        clear();
        printDungeon(0, 0);
        refresh();
    }

    // Cleanup
    endwin();
    pq_destroy(event_queue);
    return 0;
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
        //case '2':
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
        //case '3':
        case 'n':
        case KEY_END: // down-left
        //case '1':
        case 'b':
        case KEY_B2: // rest
        case '5':
        case ' ':
        case '.':
            if (playerToMove) movePlayer(key);  // Move the player
            break;
        case 'q': // quit the game
        case 'Q':
            gameOver = true;
            endwin();
            printf("\nYou quit the game!\n");
            break;
        case '2': // PC can attack 1 or 2 spaces away
            attack(1); attack(2); break;
        case '1': // counter-clockwise
            changeDirection(false, false); break;
        case '3': // clocckwise
            changeDirection(true, false); break;
        case '<':
        case '>':
            useStairs(key); break;
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
    // Free the memory allocated for the previous level if it was dynamically allocated.
    if (dungeon.rooms) {
        free(dungeon.rooms);
        dungeon.rooms = NULL;
    }
    if (dungeon.monsters) {
        free(dungeon.monsters);
        dungeon.monsters = NULL;
    }
    if (dungeon.upwardStairs) {
        free(dungeon.upwardStairs);
        dungeon.upwardStairs = NULL;
    }
    if (dungeon.downwardStairs) {
        free(dungeon.downwardStairs);
        dungeon.downwardStairs = NULL;
    }
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
    
    pq_destroy(event_queue);
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
                attackX -= distance;
                break;
            case DOWN:
                attackX += distance;
                break;
            case LEFT:
                attackY -= distance;
                break;
            case RIGHT:
                attackY += distance;
                break;
            case UP_LEFT:
                attackX -= distance;
                attackY -= distance;
                break;
            case UP_RIGHT:
                attackX -= distance;
                attackY += distance;
                break;
            case DOWN_LEFT:
                attackX += distance;
                attackY -= distance;
                break;
            case DOWN_RIGHT:
                attackX += distance;
                attackY += distance;
                break;
            default:
                break; // Invalid direction (shouldn't happen)
        }

        // Check if the position is valid and contains a monster
        if (attackX >= 0 && attackX < DUNGEON_HEIGHT && attackY >= 0 && attackY < DUNGEON_WIDTH) {
            for (int j = 0; j < dungeon.numMonsters; j++) {
                monster_t *monster = &dungeon.monsters[j];
                if (monster->alive && monster->posX == attackY && monster->posY == attackX) {
                    // Monster is killed (same logic as before)
                    monster->alive = false;
                    dungeon.map[attackX][attackY] = PLAYER_CELL;
                    dungeon.map[oldX][oldY] = (cell_t){dungeon.pc.previousCharacter, 0};
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
            newX--; dungeon.pc.currentDirection = UP;  break;
        case KEY_DOWN: case '2': case 'j':
            newX++; dungeon.pc.currentDirection = DOWN; break;
        case KEY_LEFT: case '4': case 'h':
            newY--; dungeon.pc.currentDirection = LEFT; break;
        case KEY_RIGHT: case '6': case 'l':
            newY++; dungeon.pc.currentDirection = RIGHT; break;
        case KEY_HOME: case '7': case 'y':
            newY--; newX--; dungeon.pc.currentDirection = UP_LEFT; break;
        case KEY_PPAGE: case '9': case 'u':
            newY++; newX--; dungeon.pc.currentDirection = UP_RIGHT; break;
        case KEY_NPAGE: case '3': case 'n':
            newY++; newX++; dungeon.pc.currentDirection = DOWN_RIGHT; break;
        case KEY_END: case '1': case 'b':
            newY--; newX++; dungeon.pc.currentDirection = DOWN_LEFT; break;
        case KEY_B2: case '5': case ' ': case '.':
            // rest
            break;
    }
    
    if (newX >= 0 && newX < DUNGEON_HEIGHT &&
        newY >= 0 && newY < DUNGEON_WIDTH &&
        dungeon.map[newX][newY].hardness == 0) {
        //gameOver = true;
        switch(dungeon.pc.previousCharacter) {
            case '#':
                dungeon.map[oldX][oldY] = CORRIDOR_CELL;
                break;
            case '.':
               dungeon.map[oldX][oldY] = ROOM_CELL;
               break;
            case '<':
               dungeon.map[oldX][oldY] = UPWARD_STAIRS_CELL;
               break;
            case '>':
               dungeon.map[oldX][oldY] = DOWNWARD_STAIRS_CELL;
               break;
        }
        
        dungeon.pc.previousCharacter = dungeon.map[newX][newY].ch;
        dungeon.pc.posX = newX;
        dungeon.pc.posY = newY;
        dungeon.map[newX][newY] = PLAYER_CELL;
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

void moveMonster(int index) {
    if (!dungeon.monsters[index].alive) return;
    monster_t *m = &dungeon.monsters[index];
    int oldX = m->posX;
    int oldY = m->posY;
    int newX = oldX;
    int newY = oldY;

    // Get monster attributes from bitfield
    int isIntelligent = m->monsterBits & (1 << 0);
    int isTelepathic = m->monsterBits & (1 << 1);
    int isTunneling = m->monsterBits & (1 << 2);
    int isErratic = m->monsterBits & (1 << 3);

    // choose right distance map
    int (*dist)[DUNGEON_WIDTH] = isTunneling ? dungeon.tunnelingMap : dungeon.nonTunnelingMap;

    bool randomErratic = rand() % 2 == 1;
    // first we handle erraticness
    if (isErratic && randomErratic) {// or 50% chance to move randomly if they are erratic
        bool foundOpenCell = false;
        while (!foundOpenCell) {
            int randDY = rand() % 3 - 1; // either -1, 0, or 1 direction change to accomodate all 8 directions
            int randDX = rand() % 3 - 1;
            newY += randDY;
            newX += randDX;
            if (isTunneling) foundOpenCell = true;
            else {
                if (dungeon.map[newY][newX].ch != ' ') { // we must put the non tunneling erratic monster randomly on non-rock
                    foundOpenCell = true;
                }
            }
        }
    }
    else {
        // Intelligent/Unintelligent Movement
        if (isIntelligent) {
            // Intelligent Movement
            int targetX, targetY;

            if (isTelepathic) {
                // Telepathic: Always know PC's position
                targetX = dungeon.pc.posX;
                targetY = dungeon.pc.posY;
            } else {
                // Non-Telepathic: Check for line of sight
                if (hasLineOfSight(m->posX, m->posY, dungeon.pc.posX, dungeon.pc.posY)) {
                    targetX = dungeon.pc.posX;
                    targetY = dungeon.pc.posY;
                    m->lastSeenPCX = targetX; // Update last seen position
                    m->lastSeenPCY = targetY;
                } else {
                    // No line of sight: Move towards last seen position (if any)
                    if (m->lastSeenPCX != -1 && m->lastSeenPCY != -1) { // Check if the last seen position is valid
                        targetX = m->lastSeenPCX;
                        targetY = m->lastSeenPCY;
                    } else {
                        return; // Don't update the monster position
                    }
                }
            }

            // Pathfinding (using the appropriate distance map)
            int best_dist = INT_MAX;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = m->posX + dx;
                    int ny = m->posY + dy;
                    if (nx >= 1 && nx < DUNGEON_WIDTH - 1 && ny >= 1 && ny < DUNGEON_HEIGHT - 1) {
                        if (dist[ny][nx] < best_dist) {
                            best_dist = dist[ny][nx];
                            newX = nx;
                            newY = ny;
                        }
                    }
                }
            }

        }
        else {
            int targetX = 0, targetY = 0;
            bool canMove = false;

            // Telepathic monsters always know PC location
            if (isTelepathic) {
                targetX = dungeon.pc.posX;
                targetY = dungeon.pc.posY;
                canMove = true;
            }
            
            // Non-telepathic check line of sight
            else if (hasLineOfSight(oldX, oldY, dungeon.pc.posX, dungeon.pc.posY)) {
                targetX = dungeon.pc.posX;
                targetY = dungeon.pc.posY;
                canMove = true;
            }

            if (canMove) {
                // Straight-line movement logic
                int dx = targetX - oldX;
                int dy = targetY - oldY;

                if (abs(dx) > abs(dy)) {
                    newX += (dx > 0) ? 1 : -1;
                } else {
                    newY += (dy > 0) ? 1 : -1;
                }
            } else {
                return;
            }
        }
    }
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
        if (dungeon.map[y][x].hardness > 0 && dungeon.map[y][x].hardness < 255) {
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
        else if (!collision && dungeon.map[newY][newX].hardness > 0 && dungeon.map[newY][newX].hardness < 255 && isTunneling) { // we have met mutable rock and we are a tunneler
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
    for (int i = 0; i < dungeon.numMonsters; i++) {
        if (dungeon.monsters[i].alive &&
            dungeon.monsters[i].posY == dungeon.pc.posX &&
            dungeon.monsters[i].posX == dungeon.pc.posY) {
            gameOver = true;
            endwin();
            printf("Game Over - You were killed by monster %c!", dungeon.monsters[i].MONSTER_CELL.ch);
            return;
        }
    }
    
    // Check remaining monsters
    bool monstersAlive = false;
    for (int i = 0; i < dungeon.numMonsters; i++) {
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
    for (int y = 1; y < DUNGEON_HEIGHT; ++y) { // Start from y = 1 to leave space for message
        move(y + 1, 0); // Adjust row position for the message line
        for (int x = 1; x < DUNGEON_WIDTH -1; ++x) {
            addch(dungeon.map[y][x].ch);
        }
    }
    drawMessage();
    refresh();
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
                        if (dungeon.map[i][j].ch != ROCK_CELL.ch || dungeon.map[i][j].hardness == 255)
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

