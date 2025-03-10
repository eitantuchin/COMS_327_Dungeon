#include "dungeon_monsters.h"
#include "priority_queue.h"

// our dungeon 
dungeon_t dungeon;
int roomCounter = 0;
priority_queue_t *event_queue;
bool gameOver = false;

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
    dungeon.pc.speed = PC_SPEED;
    dungeon.monsters = (monster_t *)malloc(sizeof(monster_t) * MAX_NUM_MONSTERS);
    
    // Initialize ncurses
    initscr(); // Start ncurses mode
    cbreak();  // Disable line buffering
    noecho();  // Don't display typed characters
    keypad(stdscr, TRUE); // Enable special keys (like arrow keys)
    curs_set(0);  // Hide cursor

    
    // make a dungeon but no saving or loading
    if (argv[1] == NULL) {
        dungeon.numMonsters = rand() % (13 - MIN_NUM_MONSTERS + 1) + MIN_NUM_MONSTERS;
        initImmutableRock();
        addRooms();
        addCorridors();
        addStairs(numStairs);
        initPCPosition();
        initMonsters();
    }
    else if(strcmp(argv[1], "--nummon") == 0)
    {
        dungeon.numMonsters = atoi(argv[2]);
        initImmutableRock();
        addRooms();
        addCorridors();
        addStairs(numStairs);
        initPCPosition();
        initMonsters();
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
        printf("Unsupported command configuration: Please use either --nummon, --load, --save, or --load --save.\n");
        return 0;
    }

    
    // After dungeon is loaded/generated:
    calculateDistances(0);  // Non-tunneling
    calculateDistances(1);  // Tunneling

    // Initialize event queue
    event_queue = pq_create(MAX_NUM_MONSTERS * 10); // Adequate capacity

    // Schedule initial monster events
    for (int i = 0; i < dungeon.numMonsters; i++) {
        scheduleEvent(EVENT_MONSTER, i, 0);
    }
    // Initial draw of the dungeon
    clear();
    printDungeon(0, 0);
    refresh();

    // Main game loop
    gameOver = false;
    while (!gameOver && !pq_is_empty(event_queue)) {
        processEvents();
        checkGameConditions();
    }

    // Cleanup
    endwin();
    pq_destroy(event_queue);
    return 0;
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
        clear();
        printDungeon(0, 0);
        refresh();
    }
}

void checkGameConditions(void) {
    // Check PC death
    for (int i = 0; i < dungeon.numMonsters; i++) {
        if (dungeon.monsters[i].alive &&
            dungeon.monsters[i].posY == dungeon.pc.posY &&
            dungeon.monsters[i].posX == dungeon.pc.posX) {
            gameOver = true;
            endwin();
            printf("\nGame Over - You were killed by a monster!\n");
            return;
        }
    }
    
    // Check remaining monsters
    bool monsters_alive = false;
    for (int i = 0; i < dungeon.numMonsters; i++) {
        if (dungeon.monsters[i].alive) {
            monsters_alive = true;
            break;
        }
    }
    
    if (!monsters_alive) {
        gameOver = true;
        endwin();
        printf("\nCongratulations! All monsters have been defeated!\n");
    }
}

void scheduleEvent(event_type_t type, int monsterIndex, int currentTurn) {
    event_t *newEvent = malloc(sizeof(event_t));
    newEvent->type = type;
    newEvent->monsterIndex = monsterIndex;
    newEvent->turn =  currentTurn + 1000 / dungeon.monsters[monsterIndex].speed;
    pq_insert(event_queue, newEvent->monsterIndex,  0, newEvent->turn); // we don't need to give x and y coords but we'll give an X for monsterIndex
}

void processEvents(void) {
    pq_node_t node = pq_extract_min(event_queue); // extract the first value in the Q

    if (dungeon.monsters[node.x].alive) {
        moveMonster(node.x);
        scheduleEvent(EVENT_MONSTER, node.x, node.priority); // keep moving the monster
    }
    // Update display
    usleep(250000);
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
            if (dungeon.map[randY][randX].ch == '.' || dungeon.map[randY][randX].ch == '#') {
                dungeon.monsters[i].posX = randX;
                dungeon.monsters[i].posY = randY;
                dungeon.monsters[i].previousCharacter = dungeon.map[randY][randX].ch;
                dungeon.map[randY][randX] = dungeon.monsters[i].MONSTER_CELL;
                foundPosition = true;
            }
        }
    }
    
}


// Modified printDungeon function using ncurses
void printDungeon(int showDist, int tunneling) {
    for (int y = 0; y < DUNGEON_HEIGHT; ++y) {
        for (int x = 0; x < DUNGEON_WIDTH; ++x) {
            // Draw borders
            if (y == 0 || y == DUNGEON_HEIGHT - 1) {
                mvaddch(y, x, '-');
            }
            else if (x == 0 || x == DUNGEON_WIDTH - 1) {
                mvaddch(y, x, '|');
            }
            // Draw regular cells
            else {
                mvaddch(y, x, dungeon.map[y][x].ch);
            }
        }
    }
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