#include "../headers/dungeon.hpp"
#include "../headers/dungeon_game.hpp"
#include "../headers/monster.hpp"
#include "../headers/pc.hpp"
#include "../headers/character.hpp"
#include "../headers/priority_queue.h"
#include "../headers/parse.hpp"
#include "../headers/item.hpp"
#include <array>
#include <string>
#include <cstring>
#include <ncurses.h>
#include <sstream>
#include <cmath>
#include <memory>

using namespace std;

// Global variables
Dungeon dungeon;
my_priority_queue event_queue;
bool gameOver = false;
bool fogOfWar = true;
bool playerToMove = true;
message_t gameMessage = {"", 0, false};
message_t directionMessage = {"", 0, true};
string dirNames[8] = {
    "NORTH", "NORTH-EAST", "EAST", "SOUTH-EAST", "SOUTH", "SOUTH-WEST", "WEST", "NORTH-WEST"
};
int monsterListScrollOffset = 0;
cell_t targetingPointerPreviousCell = {' ', 0};
const cell_t IMMUTABLE_ROCK_CELL = {' ', 255};
const cell_t ROCK_CELL = {' ', 1};
const cell_t ROOM_CELL = {'.', 0};
const cell_t CORRIDOR_CELL = {'#', 0};
const cell_t UPWARD_STAIRS_CELL = {'<', 0};
const cell_t DOWNWARD_STAIRS_CELL = {'>', 0};
const cell_t PLAYER_CELL = {'@', 0};
const cell_t POINTER_CELL = {'*', 0};

/*
 Allows the user to choose between saving, loading, and creating dungeons
 */
int main(int argc, char *argv[])
{
    // Initialize dungeon
    dungeon.setNumRooms(MIN_NUM_ROOMS);
    dungeon.setRooms(vector<room_t>(dungeon.getNumRooms()));
    dungeon.setUpwardStairs(vector<stair_t>(3));
    dungeon.setDownwardStairs(vector<stair_t>(3));
    dungeon.setModeType(PLAYER_CONTROL);
    dungeon.getPC().setSpeed(PC_SPEED);
    srand((unsigned int) time(NULL));
    
    // Make a dungeon but no saving or loading
    if (argv[1] == NULL) {
        int numMonsters = rand() % (10 - MIN_NUM_MONSTERS + 1) + MIN_NUM_MONSTERS;
        dungeon.setNumMonsters(numMonsters);
        dungeon.setMonsters(vector<Monster>(dungeon.getNumMonsters()));
        generateDungeon();
    }
    else if (strcmp(argv[1], "--nummon") == 0 && argv[2] != NULL) {
        int numMonsters = atoi(argv[2]);
        dungeon.setNumMonsters(numMonsters);
        dungeon.setMonsters(vector<Monster>(dungeon.getNumMonsters()));
        generateDungeon();
    }
    else if (strcmp(argv[1], "--nummon") == 0 && argv[2] == NULL) {
        printf("When using --nummon switch you must provide an integer amount for monsters like --nummon x, where x is an integer.\n");
        return 0;
    }
    else if (strcmp(argv[1], "--parse") == 0 && strcmp(argv[2], "-m") == 0) {
        readMonsters(true);
        return 0;
    }
    else if (strcmp(argv[1], "--parse") == 0 && strcmp(argv[2], "-o") == 0) {
        readObjects(true);
        return 0;
    }
    else if (strcmp(argv[1], "--parse") == 0 && argv[2] == NULL) {
        printf("When using --parse switch you must provide chars o or m, o for objects descriptions and m for monster descriptions. Usage is like --parse -o or --parse -m.\n");
        return 0;
    }
    
    else {
        printf("Unsupported command configuration: Please use either --nummon, --parse {-o or -m}, or no switch.\n");
        return 0;
    }
    
    // Initialize ncurses
    initscr(); // Start ncurses mode
    cbreak();  // Disable line buffering
    noecho();  // Don't display typed characters
    keypad(stdscr, TRUE); // Enable special keys (like arrow keys)
    curs_set(0);  // Hide cursor
    nodelay(stdscr, TRUE);
    start_color();
    
    // Define color pairs (foreground, background)
    init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    // If foreground is BLACK, set it to WHITE for visibility
    init_pair(COLOR_BLACK, COLOR_WHITE, COLOR_BLACK);

    calculateDistances(0);  // Non-tunneling
    calculateDistances(1);  // Tunneling
    calculateDistances(2);  // Non-tunneling pass

    // Initialize event queue
    event_queue = my_priority_queue();

    char message[100];
    snprintf(message, sizeof(message), "Your turn to move!");
    displayMessage(message);
    
    // Schedule initial PC event
    scheduleEvent(EVENT_PC, -1, 0);

    // Schedule initial monster events
    for (int i = 0; i < dungeon.getNumMonsters(); i++) {
        scheduleEvent(EVENT_MONSTER, i, 0);
    }

    // Main game loop
    gameOver = false;
    while (!gameOver) {
        checkKeyInput();
        if (dungeon.getModeType() == MONSTER_LIST) {
            displayMonsterList();
        }
        else if (dungeon.getModeType() == PLAYER_CONTROL || dungeon.getModeType() == PLAYER_TELEPORT) {
            printDungeon();
            refresh();
        }
        checkGameConditions();
        if (!playerToMove) processEvents();
        checkGameConditions();
        updateFogMap();
    }

    // Cleanup
    
    endwin();
    event_queue.clear();
    return 0;
}

void checkKeyInput(void) {
    int key = getch();
    switch(key) {
        case KEY_UP: case 'k': case KEY_DOWN: // move down
        case '2': case 'j': case KEY_LEFT: // move left
        case '4': case 'h': case KEY_RIGHT: // move right
        case '6': case 'l': case KEY_HOME: // up-left
        case '7': case 'y': case KEY_PPAGE: // up-right
        case '9': case 'u': case KEY_NPAGE: // down-right
        case '3': case 'n': case KEY_END: // down-left
        case '1': case 'b': case KEY_B2: // rest
        case '5': case ' ': case '.':
            if (playerToMove && dungeon.getModeType() == PLAYER_CONTROL) movePlayer(key);
            if (playerToMove && dungeon.getModeType() == PLAYER_TELEPORT)  moveTargetingPointer(key);
            if (dungeon.getModeType() == MONSTER_LIST) {
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
            dungeon.setRooms(vector<room_t>()); // Clear rooms
            dungeon.setUpwardStairs(vector<stair_t>()); // Clear stairs
            dungeon.setDownwardStairs(vector<stair_t>()); // Clear stairs
            dungeon.setMonsters(vector<Monster>()); // Clear monsters
            exit(0);
            break;
        case 'z': // PC can attack 1 space away
            attack(1); break;
        case 'V': // counter-clockwise
            changeDirection(false, false); break;
        case 'v': // clocckwise
            changeDirection(true, false); break;
        case '<':
        case '>':
            useStairs(key); break;
        case 'm':
            if (playerToMove && dungeon.getModeType() == PLAYER_CONTROL) {
                clear(); dungeon.setModeType(MONSTER_LIST); monsterListScrollOffset = 0;
            }
            else {
                char message[100];
                snprintf(message, sizeof(message), "You must be in player control mode to see the monster list!");
                displayMessage(message);
            }
            break;
        case 27: // escape key
            clear(); dungeon.setModeType(PLAYER_CONTROL);
            break;
        case 'f':
            if (dungeon.getModeType() == PLAYER_CONTROL) {
                fogOfWar = !fogOfWar;
            }// fog of war switch
            else {
                char message[100];
                snprintf(message, sizeof(message), "Cannot use f key when not in player control mode!");
                displayMessage(message);
            }
            break;
        case 'g': // goto/teleport mode activated
            if (dungeon.getModeType() == PLAYER_TELEPORT) {
                teleportPlayer(false);
                dungeon.setModeType(PLAYER_CONTROL);
            }
            else if (dungeon.getModeType() == PLAYER_CONTROL){
                dungeon.setModeType(PLAYER_TELEPORT);
                initTargetingPointer();
            }
            else {
                char message[100];
                snprintf(message, sizeof(message), "You must have player control to use the GOTO command!");
                displayMessage(message);
            }
            break;
        case 'r':
            if (dungeon.getModeType() == PLAYER_TELEPORT) {
                teleportPlayer(true);
                dungeon.setModeType(PLAYER_CONTROL);
            }
            break;
    }
}


void initTargetingPointer(void) {
    int pcX = dungeon.getPC().getPosX();
    int pcY = dungeon.getPC().getPosY();
    bool pointerPlaced = false;

    for (int y = pcY - 1; y <= pcY + 1 && !pointerPlaced; ++y) {
        for (int x = pcX - 1; x <= pcX + 1; ++x) {
            if (dungeon.getMap()[y][x].hardness != IMMUTABLE_ROCK_CELL.hardness && x != pcX && y != pcY) {
                targetingPointerPreviousCell = dungeon.getMap()[y][x];
                dungeon.getMap()[y][x] = POINTER_CELL;
                pointerPlaced = true;
                break;
            }
        }
    }
    refresh();
}

pair<int, int> getPointerCellPosition(void) {
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            if (dungeon.getMap()[y][x].ch == POINTER_CELL.ch) {
                return {x, y};
            }
        }
    }
    return {-1, -1};
}

void moveTargetingPointer(int key) {
    pair<int, int> pointerPos = getPointerCellPosition();
    int newX = pointerPos.first;
    int newY = pointerPos.second;
    int oldX = newX;
    int oldY = newY;
    switch (key) {
        case KEY_UP: case '8': case 'k':
            newY--; break;
        case KEY_DOWN: case '2': case 'j':
            newY++;  break;
        case KEY_LEFT: case '4': case 'h':
            newX--;  break;
        case KEY_RIGHT: case '6': case 'l':
            newX++;  break;
        case KEY_HOME: case '7': case 'y':
            newX--; newY--; break;
        case KEY_PPAGE: case '9': case 'u':
            newX++; newY--; break;
        case KEY_NPAGE: case '3': case 'n':
            newX++; newY++; break;
        case KEY_END: case '1': case 'b':
            newX--; newY++; break;
    }
    // as long as it's not immutable rock we can move the pointer there
    if (dungeon.getMap()[newY][newX].hardness != IMMUTABLE_ROCK_CELL.hardness) {
        dungeon.getMap()[oldY][oldX] = targetingPointerPreviousCell;
        targetingPointerPreviousCell = dungeon.getMap()[newY][newX];
        dungeon.getMap()[newY][newX] = POINTER_CELL;
    }
}


void updateFogMap(void) {
    for (int y = dungeon.getPC().getPosY() - 2; y <= dungeon.getPC().getPosY() + 2; ++y) {
        for (int x = dungeon.getPC().getPosX() - 2; x <= dungeon.getPC().getPosX() + 2; ++x) {
            if (!dungeon.getPC().getFogMap()[y][x]) {
                dungeon.getPC().getFogMap()[y][x] = true;
            }
        }
    }
    refresh();
}

void printDungeon(void) {
    move(1, 0); // Move cursor to the second row (leaving the first for the message)
    for (int y = 1; y < DUNGEON_HEIGHT; ++y) {
        move(y + 1, 0);
        for (int x = 1; x < DUNGEON_WIDTH; ++x) {
            printCharacter(x, y);
        }
    }
    drawMessage();
}

void printCharacter(int x, int y) {
    short color = 0;
    for (auto entry: dungeon.getItemLocationsAndPriorities()) {
        vector<Item> items = entry.second;
        if (entry.first.first == y && entry.first.second == x && dungeon.getMap()[y][x].hardness == -2) {
            color = items[items.size() - 1].getColor()[0]; // last item in array is the topmost item
            break;
        }
    }
    attron(COLOR_PAIR(color));
    if (fogOfWar) {
        if (dungeon.getMap()[y][x].ch == '*')  addch(dungeon.getMap()[y][x].ch);
        else if (x <= dungeon.getPC().getPosX() + 2 && x >= dungeon.getPC().getPosX() - 2 &&
            y <= dungeon.getPC().getPosY() + 2 && y >= dungeon.getPC().getPosY() - 2) {
            addch(dungeon.getMap()[y][x].ch);
        }
        else {
            if (dungeon.getPC().getFogMap()[y][x] && dungeon.getMap()[y][x].hardness == -1) {
                // add previous character since monster but shouldn't be seen
                for (int i = 0; i < dungeon.getMonsters().size(); ++i) {
                    if (dungeon.getMonsters()[i].getPosX() == x && dungeon.getMonsters()[i].getPosY() == y) {
                        addch(dungeon.getMonsters()[i].getPreviousCell().ch);
                    }
                }
            }
            else if (dungeon.getPC().getFogMap()[y][x]) {
                addch(dungeon.getMap()[y][x].ch);
            }
            else {
                addch(' '); // can't be seen
            }
        }
    }
    else {
        addch(dungeon.getMap()[y][x].ch);
    }
    attroff(COLOR_PAIR(color));
    refresh();
}


void scheduleEvent(event_type_t type, int index, int currentTurn) {
    unique_ptr<event_t> newEvent(new event_t);
    newEvent->type = type;
    newEvent->index = index;
    if (type == EVENT_PC) {
        newEvent->turn = currentTurn + (1000 / dungeon.getPC().getSpeed());
    }
    else {
        newEvent->turn = currentTurn + (1000 / dungeon.getMonsters()[index].getSpeed());
    }
    event_queue.insert(newEvent->index, 0, newEvent->turn);
}

void processEvents(void) {
    char message[100];
    pq_node_t node = event_queue.extract_min(); // extract the first value in the Q
    if (node.x == -1) { // EVENT_PC
        playerToMove = true; // this stops the processing of events until the player moves again
        scheduleEvent(EVENT_PC, -1, node.priority);
        snprintf(message, sizeof(message), "Your turn to move!");
        displayMessage(message);
    }
    else if (dungeon.getMonsters()[node.x].isAlive()) {
        moveMonster(node.x);
        usleep(50000); // intervals between when each monster moves
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

void checkGameConditions(void) {
    char message[100];
    // Check if we are standing on top of stairs
    if (dungeon.getPC().getPreviousCell().ch == '<') {
        snprintf(message, sizeof(message), "You are on upward stairs! Press < to go up a level!");
        displayMessage(message);
    }
    else if (dungeon.getPC().getPreviousCell().ch == '>') {
        snprintf(message, sizeof(message), "You are on downward stairs! Press > to go down a level!");
        displayMessage(message);
    }
    // Check PC death
    for (int i = 0; i < dungeon.getNumMonsters(); ++i) {
        if (dungeon.getMonsters()[i].isAlive() &&
            dungeon.getMonsters()[i].getPosY() == dungeon.getPC().getPosY() &&
            dungeon.getMonsters()[i].getPosX() == dungeon.getPC().getPosX()) {
            gameOver = true;
            endwin();
            printf("\nGame Over - You were killed by monster %c!", dungeon.getMonsters()[i].getCell().ch);
            return;
        }
    }
    
    // Check remaining monsters
    bool monstersAlive = false;
    for (int i = 0; i < dungeon.getNumMonsters(); ++i) {
        if (dungeon.getMonsters()[i].isAlive()) {
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

void calculateDistances(int mapNum) {
    // using the priority queue and dijikstra's algorithm
    my_priority_queue pq = my_priority_queue();
    int (*dist)[DUNGEON_WIDTH];
    if (mapNum == 0) dist = dungeon.getNonTunnelingMap();
    else if (mapNum == 1) dist = dungeon.getTunnelingMap();
    else dist = dungeon.getNonTunnelingPassMap();

    // Initialize distances
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            dist[y][x] = INT_MAX;
        }
    }

    // Start with PC position
    int pc_y = dungeon.getPC().getPosY();
    int pc_x = dungeon.getPC().getPosX();
    dist[pc_y][pc_x] = 0;
    pq.insert(pc_x, pc_y, 0);

    while (!pq.is_empty()) {
        pq_node_t node = pq.extract_min();
        int x = node.x;
        int y = node.y;
        
        
        cell_t currentCell = dungeon.getMap()[y][x];

        // Check all 8 neighbors
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                int nx = x + dx;
                int ny = y + dy;

                // Bounds check
                if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT) continue;

                // Check passability
                cell_t neighborCell = dungeon.getMap()[ny][nx];
                if (mapNum == 0 && neighborCell.hardness > 0) continue;  // Non-tunneler non-pass can't pass through rock
                if (neighborCell.hardness == 255) continue;             // Immutable rock

                // Calculate weight
                int weight = 1;
                if (mapNum == 1 && currentCell.hardness > 0) { // tunneler meets rock
                    weight = 1 + (currentCell.hardness / 85);
                }

                // Update distance
                int new_dist = dist[y][x] + weight;
                if (new_dist < dist[ny][nx]) {
                    dist[ny][nx] = new_dist;
                    pq.insert(nx, ny, new_dist);
                }
            }
        }
    }
    pq.clear();
}

/*
 Checks whether an element exists within given array
 */
bool contains(vector<int> array, int value) {
    for (size_t i = 0; i < array.size(); i++) {
        if (array[i] == value) {
            return true; // Value found
        }
    }
    return false; // Value not found
}

