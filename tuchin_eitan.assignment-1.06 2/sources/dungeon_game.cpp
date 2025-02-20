#include "../headers/dungeon.hpp"
#include "../headers/dungeon_game.hpp"
#include "../headers/monster.hpp"
#include "../headers/pc.hpp"
#include "../headers/character.hpp"
#include "../headers/priority_queue.h"
#include <array>
#include <string>
#include <cstring>
#include <ncurses.h>
#include <sstream>
#include <cmath>
#include <memory>

using namespace std;

// Global variables
Dungeon dungeon; // Our dungeon
int roomCounter = 0;
my_priority_queue event_queue;
bool gameOver = false;
bool fogOfWar = true;
message_t gameMessage = {"", 0, false};
message_t directionMessage = {"", 0, true};
string dirNames[8] = {
    "NORTH", "NORTH-EAST", "EAST", "SOUTH-EAST", "SOUTH", "SOUTH-WEST", "WEST", "NORTH-WEST"
};
bool playerToMove = false;
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

    srand((unsigned int) time(NULL));

    // Initialize ncurses
    initscr(); // Start ncurses mode
    cbreak();  // Disable line buffering
    noecho();  // Don't display typed characters
    keypad(stdscr, TRUE); // Enable special keys (like arrow keys)
    curs_set(0);  // Hide cursor
    nodelay(stdscr, TRUE);

    // Make a dungeon but no saving or loading
    if (argv[1] == NULL) {
        int numMonsters = rand() % (10 - MIN_NUM_MONSTERS + 1) + MIN_NUM_MONSTERS;
        dungeon.setNumMonsters(numMonsters);
        dungeon.setMonsters(vector<Monster>(dungeon.getNumMonsters()));
        generateDungeon();
    }
    else if (strcmp(argv[1], "--nummon") == 0) {
        int numMonsters = atoi(argv[2]);
        dungeon.setNumMonsters(numMonsters);
        dungeon.setMonsters(vector<Monster>(dungeon.getNumMonsters()));
        generateDungeon();
    }
    else {
        printf("Unsupported command configuration: Please use either --nummon or no switch.\n");
        return 0;
    }

    calculateDistances(0);  // Non-tunneling
    calculateDistances(1);  // Tunneling

    // Initialize event queue
    event_queue = my_priority_queue();

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

void teleportPlayer(bool randomTeleport) {
    char message[100];
    pair<int, int> pointerPos = getPointerCellPosition();
    int randY = (rand() % (DUNGEON_HEIGHT - 2) + 2) - 1;
    int randX = (rand() % (DUNGEON_WIDTH - 2) + 2) - 1;
    if (randomTeleport && (randY != dungeon.getPC().getPosY() || randX != dungeon.getPC().getPosX())) {
        dungeon.getMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()] = dungeon.getPC().getPreviousCell();
        dungeon.getPC().setPreviousCell(dungeon.getMap()[randY][randX]);
        dungeon.getPC().setPosX(randX);
        dungeon.getPC().setPosY(randY);
        dungeon.getMap()[randY][randX] = PLAYER_CELL;
        // remove pointer from screen because it's not overriden
        dungeon.getMap()[pointerPos.second][pointerPos.first] =  targetingPointerPreviousCell;
        snprintf(message, sizeof(message), "You teleported randomly to X-coord: %i and Y-coord: %i!", randX, randY);
        displayMessage(message);
    }
    else {
        dungeon.getMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()] =  dungeon.getPC().getPreviousCell();
        dungeon.getPC().setPreviousCell(targetingPointerPreviousCell);
        dungeon.getPC().setPosX(pointerPos.first);
        dungeon.getPC().setPosY(pointerPos.second);
        dungeon.getMap()[pointerPos.second][pointerPos.first] = PLAYER_CELL;
        snprintf(message, sizeof(message), "You teleported to X-coord: %i and Y-coord: %i!", pointerPos.first, pointerPos.second);
        displayMessage(message);
    }
    calculateDistances(0);
    calculateDistances(1);
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

    refresh();
}

string getMonsterPositionString(int monsterIndex) {
    // Get the monster and PC from the dungeon
    const Monster& monster = dungeon.getMonsters()[monsterIndex];
    // Calculate deltas
    int deltaX = monster.getPosX() - dungeon.getPC().getPosX();
    int deltaY = monster.getPosY() - dungeon.getPC().getPosY();

    // Determine directions
    string directionX = (deltaX > 0) ? "East" : "West";
    string directionY = (deltaY > 0) ? "South" : "North";

    // Calculate absolute deltas
    int absDeltaX = abs(deltaX);
    int absDeltaY = abs(deltaY);

    // Build the position string
    stringstream ss;

    if (absDeltaX == 0 && absDeltaY == 0) {
        ss << "next to you";
    } else if (absDeltaX == 0) {
        ss << absDeltaY << " " << directionY;
    } else if (absDeltaY == 0) {
        ss << absDeltaX << " " << directionX;
    } else {
        ss << absDeltaX << " " << directionX << " and " << absDeltaY << " " << directionY;
    }

    return ss.str();
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
    int maxScrollOffset = dungeon.getNumMonsters() - maxVisibleMonsters;
    if (monsterListScrollOffset > maxScrollOffset) {
        monsterListScrollOffset = maxScrollOffset;
    }
    if (maxScrollOffset < 0) {
        monsterListScrollOffset = 0; // If the list fits entirely on the screen, reset the offset
    }

    for (int i = 0; i < maxVisibleMonsters && i + monsterListScrollOffset < dungeon.getNumMonsters(); ++i) {
        int monsterIndex = i + monsterListScrollOffset;
        const Monster& monster = dungeon.getMonsters()[monsterIndex];

        if (monster.isAlive()) {
            string positionString = getMonsterPositionString(monsterIndex);
            const char* positionCString = positionString.c_str();
            mvprintw(i + 2, 0, "%c      | %s", monster.getCell().ch, positionCString);
        }
        else {
            mvprintw(i + 2, 0, "%c      | %s", monster.getCell().ch, "Killed");
        }
    }

    mvprintw(screenHeight - 1, 0, "Press ESC to return to the game. Use UP/DOWN arrows to scroll.");

    refresh();
}

void scheduleEvent(event_type_t type, int index, int currentTurn) {
    unique_ptr<event_t> newEvent(new event_t);
    newEvent->type = type;
    newEvent->index = index;
    if (type == EVENT_PC) {
        newEvent->turn = currentTurn + (1000 / PC_SPEED);
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
        usleep(100000); // intervals between when each monster moves
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

void useStairs(int key) {
    if (dungeon.getPC().getPreviousCell().ch == key) {
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
    // Clear dungeon data
    memset(dungeon.getPC().getFogMap(), false, sizeof(dungeon.getPC().getFogMap()));  // Set all values to false
    // Reset counters and global variables
    roomCounter = 0;
    dungeon.setNumRooms(MIN_NUM_ROOMS);
    dungeon.setRooms(vector<room_t>(dungeon.getNumRooms()));
    dungeon.setUpwardStairs(vector<stair_t>(3));
    dungeon.setDownwardStairs(vector<stair_t>(3));
    dungeon.setNumUpwardsStairs(0);
    dungeon.setNumDownwardsStairs(0);
    dungeon.getPC().setPreviousCell(ROOM_CELL);
    Dungeon();
                             
    // Clear and reset the event queue
    event_queue = my_priority_queue();

    // Generate a new dungeon level
    generateDungeon();

    // Recalculate pathfinding distances for both tunneling and non-tunneling monsters
    calculateDistances(0); // Non-tunneling
    calculateDistances(1); // Tunneling

    // Schedule events for the player and monsters
    scheduleEvent(EVENT_PC, -1, 0);
    processEvents();
    for (int i = 0; i < dungeon.getNumMonsters(); i++) {
        scheduleEvent(EVENT_MONSTER, i, 0);
    }

    // Display a message to the player
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

    // Get the PC's current direction
    PC& pc = dungeon.getPC();
    direction_t currentDirection = pc.getCurrentDirection();

    int currentDirIndex = 0;
    // Find the index of the current direction
    for (int i = 0; i < 8; i++) {
        if (currentDirection == directions[i]) {
            currentDirIndex = i;
            break;
        }
    }

    char message[100];
    if (!justChangeText) {
        // Calculate the new direction index
        int newDirIndex = (currentDirIndex + (clockwise ? 1 : -1) + 8) % 8;
        direction_t newDirection = directions[newDirIndex];

        // Update the PC's direction
        pc.setCurrentDirection(newDirection);

        // Create the message
        snprintf(message, sizeof(message), "Facing: %s", dirNames[newDirIndex].c_str());
    }
    else {
        // Create the message without changing the direction
        snprintf(message, sizeof(message), "Facing: %s", dirNames[currentDirIndex].c_str());
    }

    // Display the message
    displayMessage(message);
    refresh();
}


void attack(int distance) {
    // Determine the direction the player is facing and the *adjacent* directions
    int attackX = dungeon.getPC().getPosX();
    int attackY = dungeon.getPC().getPosY();
    int oldX = attackX;
    int oldY = attackY;

    direction_t directionsToCheck[3]; // Array to hold the directions to check

    switch (dungeon.getPC().getCurrentDirection()) {
        case UP:
            directionsToCheck[0] = UP; directionsToCheck[1] = UP_LEFT;
            directionsToCheck[2] = UP_RIGHT; break;
        case DOWN:
            directionsToCheck[0] = DOWN; directionsToCheck[1] = DOWN_LEFT;
            directionsToCheck[2] = DOWN_RIGHT; break;
        case LEFT:
            directionsToCheck[0] = LEFT; directionsToCheck[1] = UP_LEFT;
            directionsToCheck[2] = DOWN_LEFT; break;
        case RIGHT:
            directionsToCheck[0] = RIGHT; directionsToCheck[1] = UP_RIGHT;
            directionsToCheck[2] = DOWN_RIGHT; break;
        case UP_LEFT:
            directionsToCheck[0] = UP_LEFT; directionsToCheck[1] = UP;
            directionsToCheck[2] = LEFT; break;
        case UP_RIGHT:
            directionsToCheck[0] = UP_RIGHT; directionsToCheck[1] = UP;
            directionsToCheck[2] = RIGHT; break;
        case DOWN_LEFT:
            directionsToCheck[0] = DOWN_LEFT; directionsToCheck[1] = DOWN;
            directionsToCheck[2] = LEFT; break;
        case DOWN_RIGHT:
            directionsToCheck[0] = DOWN_RIGHT; directionsToCheck[1] = DOWN;
            directionsToCheck[2] = RIGHT; break;
        default: return; // Invalid direction
    }

    for (int i = 0; i < 3; i++) { // Check all three directions
        attackX = dungeon.getPC().getPosX();
        attackY = dungeon.getPC().getPosY();

        switch (directionsToCheck[i]) {
            case UP:
                attackY -= distance; break;
            case DOWN:
                attackY += distance; break;
            case LEFT:
                attackX -= distance; break;
            case RIGHT:
                attackX += distance; break;
            case UP_LEFT:
                attackY -= distance; attackX -= distance; break;
            case UP_RIGHT:
                attackY -= distance; attackX += distance; break;
            case DOWN_LEFT:
                attackY += distance; attackX -= distance; break;
            case DOWN_RIGHT:
                attackY += distance; attackX += distance; break;
            default: break; // Invalid direction (shouldn't happen)
        }

        // Check if the position is valid and contains a monster
        if (attackY >= 0 && attackY < DUNGEON_HEIGHT && attackX >= 0 && attackX < DUNGEON_WIDTH) {
            for (int j = 0; j < dungeon.getNumMonsters(); j++) {
                Monster *monster = &dungeon.getMonsters()[j];
               
                if (monster->isAlive() && monster->getPosX() == attackX && monster->getPosY() == attackY) {
                   
                    // Monster is killed (same logic as before)
                    monster->setAlive(false);
                    dungeon.getMap()[attackY][attackX] = PLAYER_CELL;
                    dungeon.getMap()[oldY][oldX] = dungeon.getPC().getPreviousCell();
                    dungeon.getPC().setPosX(attackX);
                    dungeon.getPC().setPosY(attackY);
                    dungeon.getPC().setPreviousCell(monster->getPreviousCell());
                    char message[100];
                    snprintf(message, sizeof(message), "You killed monster %c!", monster->getCell().ch);
                    displayMessage(message);
                    return;
                }
            }
        }
    }
    // Display a message to the player if we get to the end and we didn't attack anything
    char message[100];
    snprintf(message, sizeof(message), "Attacked nothing! Get close to a monster and face them!");
    displayMessage(message);
}

void movePlayer(int key) {
    // Check boundaries and valid movement
    int newX = dungeon.getPC().getPosX();
    int newY = dungeon.getPC().getPosY();
    int oldX = newX;
    int oldY = newY;
    switch (key) {
        case KEY_UP: case '8': case 'k':
            newY--; dungeon.getPC().setCurrentDirection(UP);  break;
        case KEY_DOWN: case '2': case 'j':
            newY++; dungeon.getPC().setCurrentDirection(DOWN); break;
        case KEY_LEFT: case '4': case 'h':
            newX--; dungeon.getPC().setCurrentDirection(LEFT); break;
        case KEY_RIGHT: case '6': case 'l':
            newX++; dungeon.getPC().setCurrentDirection(RIGHT); break;
        case KEY_HOME: case '7': case 'y':
            newX--; newY--; dungeon.getPC().setCurrentDirection(UP_LEFT); break;
        case KEY_PPAGE: case '9': case 'u':
            newX++; newY--; dungeon.getPC().setCurrentDirection(UP_RIGHT); break;
        case KEY_NPAGE: case '3': case 'n':
            newX++; newY++; dungeon.getPC().setCurrentDirection(DOWN_RIGHT); break;
        case KEY_END: case '1': case 'b':
            newX--; newY++; dungeon.getPC().setCurrentDirection(DOWN_LEFT); break;
        case KEY_B2: case '5': case ' ': case '.': break;
    }
    
    if (newY >= 0 && newY < DUNGEON_HEIGHT &&
        newX >= 0 && newX < DUNGEON_WIDTH &&
        dungeon.getMap()[newY][newX].hardness <= 0) {
        //gameOver = true;
        switch(dungeon.getPC().getPreviousCell().ch) {
            case '#':
                dungeon.getMap()[oldY][oldX] = CORRIDOR_CELL;
                break;
            case '.':
               dungeon.getMap()[oldY][oldX] = ROOM_CELL;
               break;
            case '<':
               dungeon.getMap()[oldY][oldX] = UPWARD_STAIRS_CELL;
               break;
            case '>':
               dungeon.getMap()[oldY][oldX] = DOWNWARD_STAIRS_CELL;
               break;
            case ' ':
                dungeon.getMap()[oldY][oldX] = targetingPointerPreviousCell;
                break;
        }
        
        //dungeon.getMap()[oldX][oldY] = dungeon.getPC().getPreviousCell();
        dungeon.getPC().setPreviousCell(dungeon.getMap()[newY][newX]);
        dungeon.getPC().setPosX(newX);
        dungeon.getPC().setPosY(newY);
        dungeon.getMap()[newY][newX] = PLAYER_CELL;
        changeDirection(false, true);
        playerToMove = false;
    }
    else {
        char message[100];
        snprintf(message, sizeof(message), "Uh oh! There's rock there!");
        displayMessage(message);
    }
    calculateDistances(0); // recalculate pathfinding maps after moving player
    calculateDistances(1);
}


inline int sign(int x) {
    return (x > 0) - (x < 0);
}

void moveMonster(int index) {
    // Do nothing if monster is not alive.
    if (!dungeon.getMonsters()[index].isAlive())
        return;
        
    Monster *m = &dungeon.getMonsters()[index];
    int oldX = m->getPosX();
    int oldY = m->getPosY();
    int newX = oldX;
    int newY = oldY;

    // For tunneling purposes.
    bool isTunneling = (m->getMonsterBits() & (1 << 2)) != 0;
    
    // Get the monster's symbol.
    char sym = m->getCell().ch;
    
    // For erratic monsters, we give a 50% chance to move randomly.
    bool erraticRandom = false;
    // (Check only for monsters whose symbol indicates erratic behavior.)
    if (sym == '8' || sym == '9' || sym == 'a' || sym == 'b' ||
        sym == 'c' || sym == 'd' || sym == 'e' || sym == 'f') {
        erraticRandom = (rand() % 2 == 0);
    }
    
    switch(sym) {
        // --- Non-erratic monsters ---
        case '0': {
            // Characteristics: non-erratic, non-tunneling, non-telepathy, non-intelligent.
            // Behavior: Move toward the PC ONLY if line-of-sight exists; move in a straight line.
            if (!hasLineOfSight(oldX, oldY, dungeon.getPC().getPosX(), dungeon.getPC().getPosY()))
                return;
            newX = oldX + sign(dungeon.getPC().getPosX() - oldX);
            newY = oldY + sign(dungeon.getPC().getPosY() - oldY);
            break;
        }
        case '1': {
            // Characteristics: non-erratic, non-tunneling, non-telepathy, intelligent.
            // Behavior: If line-of-sight exists, update last seen and use pathfinding (via non-tunneling map)
            // to choose the neighbor with the lowest cost. Otherwise, if a last-seen position exists, use it.
            int targetX, targetY;
            if (hasLineOfSight(oldX, oldY, dungeon.getPC().getPosX(), dungeon.getPC().getPosY())) {
                targetX = dungeon.getPC().getPosX();
                targetY = dungeon.getPC().getPosY();
                m->setLastSeenPCX(targetX);
                m->setLastSeenPCY(targetY);
            } else {
                if (m->getLastSeenPCX() != -1 && m->getLastSeenPCY() != -1) {
                    targetX = m->getLastSeenPCX();
                    targetY = m->getLastSeenPCY();
                } else {
                    return;
                }
            }
            int best = INT_MAX;
            // Use the non-tunneling distance map.
            int (*dist)[DUNGEON_WIDTH] = dungeon.getNonTunnelingMap();
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = oldX + dx;
                    int ny = oldY + dy;
                    if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT)
                        continue;
                    // Only consider passable cells.
                    if (dungeon.getMap()[ny][nx].hardness > 0)
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
            // Characteristics: non-erratic, non-tunneling, telepathy, non-intelligent.
            // Behavior: Always knows PC's position; move straight-line (no LOS check).
            newX = oldX + sign(dungeon.getPC().getPosX() - oldX);
            newY = oldY + sign(dungeon.getPC().getPosY() - oldY);
            break;
        }
        case '3': {
            // Characteristics: non-erratic, non-tunneling, telepathy, intelligent.
            // Behavior: Always knows PC; use non-tunneling pathfinding (choose neighbor with lowest cost);
            // update last seen.
            int best = INT_MAX;
            int (*dist)[DUNGEON_WIDTH] = dungeon.getNonTunnelingMap();
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
            m->setLastSeenPCX(dungeon.getPC().getPosX());
            m->setLastSeenPCY(dungeon.getPC().getPosY());
            break;
        }
        case '4': {
            // Characteristics: non-erratic, tunneling, non-telepathy, non-intelligent.
            // Behavior: Move in a straight line towards PC only if LOS exists.
            if (!hasLineOfSight(oldX, oldY, dungeon.getPC().getPosX(), dungeon.getPC().getPosY()))
                return;
            newX = oldX + sign(dungeon.getPC().getPosX() - oldX);
            newY = oldY + sign(dungeon.getPC().getPosY() - oldY);
            break;
        }
        case '5': {
            // Characteristics: non-erratic, tunneling, non-telepathy, intelligent.
            // Behavior: If LOS exists, update last seen and use tunneling pathfinding; else if last seen exists, use that.
            int targetX, targetY;
            if (hasLineOfSight(oldX, oldY, dungeon.getPC().getPosX(), dungeon.getPC().getPosY())) {
                targetX = dungeon.getPC().getPosX();
                targetY = dungeon.getPC().getPosY();
                m->setLastSeenPCX(targetX);
                m->setLastSeenPCY(targetY);
            } else {
                if (m->getLastSeenPCX() != -1 && m->getLastSeenPCY() != -1) {
                    targetX = m->getLastSeenPCX();
                    targetY = m->getLastSeenPCY();
                } else {
                    return;
                }
            }
            int best = INT_MAX;
            int (*dist)[DUNGEON_WIDTH] = dungeon.getTunnelingMap();
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
            // Characteristics: non-erratic, tunneling, telepathy, non-intelligent.
            // Behavior: Always knows PC; move straight-line.
            newX = oldX + sign(dungeon.getPC().getPosX() - oldX);
            newY = oldY + sign(dungeon.getPC().getPosY() - oldY);
            break;
        }
        case '7': {
            // Characteristics: non-erratic, tunneling, telepathy, intelligent.
            // Behavior: Always knows PC; use tunneling pathfinding; update last seen.
            int best = INT_MAX;
            int (*dist)[DUNGEON_WIDTH] = dungeon.getTunnelingMap();
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
            m->setLastSeenPCX(dungeon.getPC().getPosX());
            m->setLastSeenPCY(dungeon.getPC().getPosY());
            break;
        }
        // --- Erratic monsters (50% chance to move randomly) ---
        case '8': {
            // Characteristics: erratic, non-tunneling, non-telepathy, non-intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                // Otherwise, act like monster '0'
                if (!hasLineOfSight(oldX, oldY, dungeon.getPC().getPosX(), dungeon.getPC().getPosY()))
                    return;
                newX = oldX + sign(dungeon.getPC().getPosX() - oldX);
                newY = oldY + sign(dungeon.getPC().getPosY() - oldY);
            }
            break;
        }
        case '9': {
            // Characteristics: erratic, non-tunneling, non-telepathy, intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                // Otherwise, act like monster '1'
                int targetX, targetY;
                if (hasLineOfSight(oldX, oldY, dungeon.getPC().getPosX(), dungeon.getPC().getPosY())) {
                    targetX = dungeon.getPC().getPosX();
                    targetY = dungeon.getPC().getPosY();
                    m->setLastSeenPCX(targetX);
                    m->setLastSeenPCY(targetY);
                } else {
                    if (m->getLastSeenPCX() != -1 && m->getLastSeenPCY() != -1) {
                        targetX = m->getLastSeenPCX();
                        targetY = m->getLastSeenPCY();
                    } else {
                        return;
                    }
                }
                int best = INT_MAX;
                int (*dist)[DUNGEON_WIDTH] = dungeon.getNonTunnelingMap();
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
            // Characteristics: erratic, non-tunneling, telepathy, non-intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                // Otherwise, act like monster '2'
                newX = oldX + sign(dungeon.getPC().getPosX() - oldX);
                newY = oldY + sign(dungeon.getPC().getPosY() - oldY);
            }
            break;
        }
        case 'b': {
            // Characteristics: erratic, non-tunneling, telepathy, intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                // Otherwise, act like monster '3'
                int best = INT_MAX;
                int (*dist)[DUNGEON_WIDTH] = dungeon.getNonTunnelingMap();
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
                m->setLastSeenPCX(dungeon.getPC().getPosX());
                m->setLastSeenPCY(dungeon.getPC().getPosY());
            }
            break;
        }
        case 'c': {
            // Characteristics: erratic, tunneling, non-telepathy, non-intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                // Otherwise, act like monster '4'
                if (!hasLineOfSight(oldX, oldY, dungeon.getPC().getPosX(), dungeon.getPC().getPosY()))
                    return;
                newX = oldX + sign(dungeon.getPC().getPosX() - oldX);
                newY = oldY + sign(dungeon.getPC().getPosY() - oldY);
            }
            break;
        }
        case 'd': {
            // Characteristics: erratic, tunneling, non-telepathy, intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                // Otherwise, act like monster '5'
                int targetX, targetY;
                if (hasLineOfSight(oldX, oldY, dungeon.getPC().getPosX(), dungeon.getPC().getPosY())) {
                    targetX = dungeon.getPC().getPosX();
                    targetY = dungeon.getPC().getPosY();
                    m->setLastSeenPCX(targetX);
                    m->setLastSeenPCY(targetY);
                } else {
                    if (m->getLastSeenPCX() != -1 && m->getLastSeenPCY() != -1) {
                        targetX = m->getLastSeenPCX();
                        targetY = m->getLastSeenPCY();
                    } else {
                        return;
                    }
                }
                int best = INT_MAX;
                int (*dist)[DUNGEON_WIDTH] = dungeon.getTunnelingMap();
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
            // Characteristics: erratic, tunneling, telepathy, non-intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                // Otherwise, act like monster '6'
                newX = oldX + sign(dungeon.getPC().getPosX() - oldX);
                newY = oldY + sign(dungeon.getPC().getPosY() - oldY);
            }
            break;
        }
        case 'f': {
            // Characteristics: erratic, tunneling, telepathy, intelligent.
            if (erraticRandom) {
                newX = oldX + ((rand() % 3) - 1);
                newY = oldY + ((rand() % 3) - 1);
            } else {
                // Otherwise, act like monster '7'
                int best = INT_MAX;
                int (*dist)[DUNGEON_WIDTH] = dungeon.getTunnelingMap();
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
                m->setLastSeenPCX(dungeon.getPC().getPosX());
                m->setLastSeenPCY(dungeon.getPC().getPosY());
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
        if (dungeon.getMap()[y][x].hardness > 0 && dungeon.getMap()[y][x].hardness < IMMUTABLE_ROCK_CELL.hardness) {
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


void updateMonsterPosition(int index, int oldX, int oldY, int newX, int newY, Monster *m, bool isTunneling) {
    // performing the monster's move
    if (newX >= 1 && newX < DUNGEON_WIDTH - 1 && newY >= 1 && newY < DUNGEON_HEIGHT - 1) {
        bool collision = false;
        int defenderIndex = -1;
        for (int i = 0; i < dungeon.getNumMonsters(); i++) {
           if (i != index &&  // Don't check against itself
               dungeon.getMonsters()[i].isAlive() &&
               dungeon.getMonsters()[i].getPosX() == newX &&
               dungeon.getMonsters()[i].getPosY() == newY) {
               collision = true;
               defenderIndex = i;
               break;
           }
       }
        if (!collision && dungeon.getMap()[newY][newX].hardness == 0) { // no monster fighting and not a rock cell
            dungeon.getMap()[oldY][oldX] = m->getPreviousCell(); // Restore the old cell
            m->setPreviousCell(dungeon.getMap()[newY][newX]);
            m->setPosX(newX);
            m->setPosY(newY);
            dungeon.getMap()[newY][newX] = m->getCell();

        }
        else if (!collision && dungeon.getMap()[newY][newX].hardness > 0 && dungeon.getMap()[newY][newX].hardness < IMMUTABLE_ROCK_CELL.hardness && isTunneling) { // we have met mutable rock and we are a tunneler
            dungeon.getMap()[newY][newX].hardness -= 85; // mining rock
            calculateDistances(1);  // recalculate tunneling distances
            if (dungeon.getMap()[newY][newX].hardness <= 0) {
                dungeon.getMap()[newY][newX] = CORRIDOR_CELL;

                calculateDistances(0); // Recalculate non-tunneling distances
                calculateDistances(1); // Recalculate tunneling distances

                // After turning rock to corridor, the monster should move there
                dungeon.getMap()[oldY][oldX] = m->getPreviousCell();
                m->setPreviousCell(dungeon.getMap()[newY][newX]);
                m->setPosX(newX);
                m->setPosY(newY);
                dungeon.getMap()[newY][newX] = m->getCell();
            }
            
        }
        else if (collision) {
            if (defenderIndex != -1) { // kill the defender
                dungeon.getMonsters()[defenderIndex].setAlive(false);
                dungeon.getMap()[newY][newX] = m->getCell();
                dungeon.getMap()[oldY][oldX] = m->getPreviousCell();
                m->setPosX(newX);
                m->setPosX(newY);
                m->setPreviousCell(dungeon.getMonsters()[defenderIndex].getPreviousCell());
            }
        }
    }
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

void initMonsters(void) {
    // add monsters to array
    vector<Monster> monsters(dungeon.getNumMonsters());
    for (int i = 0; i < dungeon.getNumMonsters(); ++i) {
        Monster monster;
        bool isErratic = rand() % 2 == 1;
        bool isTunneling = rand() % 2 == 1;
        bool isIntelligent = rand() % 2 == 1;
        bool isTelepathic = rand() % 2 == 1;
        uint8_t monsterBits = 0;
        if (isIntelligent) monsterBits |= (1 << 0); // Set bit 0 for intelligence
        if (isTelepathic) monsterBits |= (1 << 1);  // Set bit 1 for telepathy
        if (isTunneling) monsterBits |= (1 << 2);   // Set bit 2 for tunneling
        if (isErratic) monsterBits |= (1 << 3);     // Set bit 3 for erratic behavior
        monster.setMonsterBits(monsterBits);
        monster.setSpeed(rand() % 16 + 5);
        monster.setCell(cell_t {"0123456789abcdef"[monsterBits], -1}); // monsters have hardness -1 which is unique to them
        monster.setAlive(true);
        monster.setLastSeenPCX(-1); // Initialize last seen position
        monster.setLastSeenPCY(-1);
        
        monsters[i] = monster;
    }
    dungeon.setMonsters(monsters);
    
    // initialize positions of monsters
    
    for (int i = 0; i < dungeon.getNumMonsters(); ++i) {
        bool foundPosition = false;
        while (!foundPosition) {
            int randY = rand() % DUNGEON_HEIGHT - 2;
            int randX = rand() % DUNGEON_WIDTH - 2;
            bool pcFarAwayEnough = checkMonsterPlacementToPC(randX, randY);
            if ((dungeon.getMap()[randY][randX].ch == '.' || dungeon.getMap()[randY][randX].ch == '#') && pcFarAwayEnough) {
                dungeon.getMonsters()[i].setPosX(randX);
                dungeon.getMonsters()[i].setPosY(randY);
                dungeon.getMonsters()[i].setPreviousCell(dungeon.getMap()[randY][randX]);
                dungeon.getMap()[randY][randX] = dungeon.getMonsters()[i].getCell();
                foundPosition = true;
            }
        }
    }
}

bool checkMonsterPlacementToPC(int randX, int randY) {
    for (int y = randY - 2; y < randY + 2; ++y) {
        for (int x = randX - 2; x < randX + 2; ++x) {
            if (dungeon.getMap()[y][x].ch == '@') {
                return false;
            }
        }
    }
    return true;
}


void calculateDistances(int tunneling) {
    // using the priority queue and dijikstra's algorithm
    my_priority_queue pq = my_priority_queue();
    int (*dist)[DUNGEON_WIDTH] = tunneling ? dungeon.getTunnelingMap() : dungeon.getNonTunnelingMap();

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
                if (!tunneling && neighborCell.hardness > 0) continue;  // Non-tunneler can't pass through rock
                if (neighborCell.hardness == 255) continue;             // Immutable rock

                // Calculate weight
                int weight = 1;
                if (tunneling && currentCell.hardness > 0) { // tunneler meets rock
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
 Initializes all the immutable rock and regular rock
 */
void initImmutableRock(void)
{
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        for (int x = 0; x < DUNGEON_WIDTH; x++)
        {
            // each rock has a different hardness level
            dungeon.getMap()[y][x] = ROCK_CELL;
            int randomHardness = rand() % 254 + 1; // max hardness is 255 - 1
            dungeon.getMap()[y][x].hardness = randomHardness;
        }
    }

    int maxX = DUNGEON_WIDTH - 1;
    int maxY = DUNGEON_HEIGHT - 1;
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        dungeon.getMap()[y][0] = IMMUTABLE_ROCK_CELL;
        dungeon.getMap()[y][maxX] = IMMUTABLE_ROCK_CELL;
    }
    for (int x = 0; x < DUNGEON_WIDTH; x++)
    {
        dungeon.getMap()[0][x] = IMMUTABLE_ROCK_CELL;
        dungeon.getMap()[maxY][x] = IMMUTABLE_ROCK_CELL;
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
        roomCentroids[i][0] = dungeon.getRooms()[i].posX + dungeon.getRooms()[i].width / 2;
        roomCentroids[i][1] = dungeon.getRooms()[i].posY + dungeon.getRooms()[i].height / 2;
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

        if (dungeon.getMap()[y][x].ch == ROCK_CELL.ch &&
            dungeon.getMap()[y][x].hardness < IMMUTABLE_ROCK_CELL.hardness) {
            dungeon.getMap()[y][x] = CORRIDOR_CELL;
        }
    }
}

/*
 Adds the rooms to the dungeon
 */
void addRooms(void)
{
    dungeon.setNumRooms(rand() % (MAX_NUM_ROOMS - MIN_NUM_ROOMS + 1) + MIN_NUM_ROOMS);
    int counter = 0;

    while (counter != dungeon.getNumRooms())
    {
        bool roomInserted = false;
        while (!roomInserted)
        {
            int randY = rand() % (DUNGEON_HEIGHT - 2) + 1;
            int randX = rand() % (DUNGEON_WIDTH - 2) + 1;
            int randHeight = rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1) + MIN_ROOM_HEIGHT;
            int randWidth = rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1) + MIN_ROOM_WIDTH;

            if (randY + randHeight <= DUNGEON_HEIGHT - 1 && randX + randWidth <= DUNGEON_WIDTH - 1)
            {
                bool validRoomPositionFound = true;
                // ensure that we don't create the room where immutable rock lies
                for (int y = randY - 2; y < randY + randHeight + 2; ++y)
                {
                    for (int x = randX - 2; x < randX + randWidth + 2; ++x)
                    {
                        if (dungeon.getMap()[y][x].ch != ROCK_CELL.ch || dungeon.getMap()[y][x].hardness == IMMUTABLE_ROCK_CELL.hardness)
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
                    dungeon.getRooms()[roomCounter].height = randHeight;
                    dungeon.getRooms()[roomCounter].width = randWidth;
                    dungeon.getRooms()[roomCounter].posX = randX;
                    dungeon.getRooms()[roomCounter].posY = randY;
                    roomCounter++;

                    for (int y = randY; y < randY + randHeight; ++y)
                    {
                        for (int x = randX; x < randX + randWidth; ++x)
                        {
                            dungeon.getMap()[y][x] = ROOM_CELL;
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
    int randomRoomNum = rand() % dungeon.getNumRooms();
    room_t room = dungeon.getRooms()[randomRoomNum];
    int getX = room.posX;
    int getY = room.posY;
    dungeon.getPC().setPosX(getX + 2);
    dungeon.getPC().setPosY(getY + 2);
    if (dungeon.getMap()[getY + 2][getX + 2].ch != '<' ||
        dungeon.getMap()[getY + 2][getX + 2].ch != '>')
    {
        dungeon.getMap()[getY + 2][getX + 2] = PLAYER_CELL;
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
    dungeon.setNumUpwardsStairs(0);
    dungeon.setNumDownwardsStairs(0);
    while (dungeon.getNumUpwardsStairs() + dungeon.getNumDownwardsStairs() != numStairs) {
        // ensure at least one upwards stairs is inside the dungeon
        int randomRoomNum;
        while (true) {
            randomRoomNum = rand() % dungeon.getNumRooms();
            size_t size = sizeof(randomRoomNums) / sizeof(randomRoomNums[0]);
            if (contains(randomRoomNums, size, randomRoomNum)) {
                continue;
            }
            randomRoomNums[dungeon.getNumUpwardsStairs() + dungeon.getNumDownwardsStairs()] = randomRoomNum;
            break;
        }
        room_t room = dungeon.getRooms()[randomRoomNum];
        //printf("Room width is: %i \n", room.width);
        //printf("Room height is: %i \n", room.height);
        int randomX = room.posX + rand() % room.width;
        int randomY = room.posY + rand() % room.height;
        stair_t stair;
        stair.posX = randomX;
        stair.posY = randomY;
        if (dungeon.getNumUpwardsStairs() + dungeon.getNumDownwardsStairs() == 0)
        {
            dungeon.getMap()[randomY][randomX] = UPWARD_STAIRS_CELL;
            dungeon.getUpwardStairs()[dungeon.getNumUpwardsStairs()] = stair;
            dungeon.setNumUpwardsStairs(dungeon.getNumUpwardsStairs() + 1);
        }
        // ensure at least one downwards stairs is inside the dungeon
        else if (dungeon.getNumUpwardsStairs() + dungeon.getNumDownwardsStairs() == 1)
        {
            dungeon.getMap()[randomY][randomX] = DOWNWARD_STAIRS_CELL;
            dungeon.getDownwardStairs()[dungeon.getNumDownwardsStairs()] = stair;
            dungeon.setNumDownwardsStairs(dungeon.getNumDownwardsStairs() + 1);
        }
        else
        {
            int randomStairCaseDirection = rand() % 2 + 1;
            if (randomStairCaseDirection == 1) {
                dungeon.getMap()[randomY][randomX] = UPWARD_STAIRS_CELL;
                dungeon.getUpwardStairs()[dungeon.getNumUpwardsStairs()] = stair;
                dungeon.setNumUpwardsStairs(dungeon.getNumUpwardsStairs() + 1);
            }
            else {
                dungeon.getMap()[randomY][randomX] = DOWNWARD_STAIRS_CELL;
                dungeon.getDownwardStairs()[dungeon.getNumDownwardsStairs()] = stair;
                dungeon.setNumDownwardsStairs(dungeon.getNumDownwardsStairs() + 1);
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
//Loads a file from disk that contains data for the construction of a dungeon
void loadFile(void){
    initImmutableRock();
    
    // Construct file path
    char *path = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + strlen("/.rlg327/%s") + 1));
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
    printf("File size: %u bytes\n", fileSize);

    // Read player position
    fread(&dungeon.getPC().getPosX(), sizeof(uint8_t), 1, f);
    fread(&dungeon.getPC().getPosY(), sizeof(uint8_t), 1, f);

    
    dungeon.map[dungeon.pc.getY()][dungeon.pc.getX()] = PLAYER_CELL;

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
    dungeon.rooms = std::vector<room_t>(dungeon.numRooms);
    if (dungeon.rooms.empty()) {
        perror("Failed to allocate memory for rooms");
        fclose(f);
        free(path);
        return;
    }

    // Rebuild rooms
    for (int i = 0; i < dungeon.numRooms; i++) {
        fread(&dungeon.rooms[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon.rooms[i].posY, sizeof(uint8_t), 1, f);
        fread(&dungeon.rooms[i].height, sizeof(uint8_t), 1, f);
        fread(&dungeon.rooms[i].width, sizeof(uint8_t), 1, f);


        // Mark room cells on the map
        for (int n = dungeon.rooms[i].posX; n < dungeon.rooms[i].posX + dungeon.rooms[i].height; n++) {
            for (int m = dungeon.rooms[i].posY; m < dungeon.rooms[i].posY + dungeon.rooms[i].width; m++) {
                if (dungeon.map[m][n].ch != '@') {
                    dungeon.map[m][n] = ROOM_CELL;
                }
            }
        }
    }

    // Rebuilding the upstairs and downstairs
    fread(&dungeon.numUpwardsStairs, sizeof(uint16_t), 1, f);
    dungeon.numUpwardsStairs = htons(dungeon.numUpwardsStairs);
    dungeon.upwardStairs = std::vector<stair_t>(dungeon.numUpwardsStairs);

    // Read stair positions and place them in the map
    for (int i = 0; i < dungeon.numUpwardsStairs; i++) {
        fread(&dungeon.upwardStairs[i].posX, sizeof(uint8_t), 1, f);
        fread(&dungeon.upwardStairs[i].posY, sizeof(uint8_t), 1, f);
        dungeon.map[dungeon.upwardStairs[i].posY][dungeon.upwardStairs[i].posX] = UPWARD_STAIRS_CELL;
    }
    
    fread(&dungeon.numDownwardsStairs, sizeof(uint16_t), 1, f);
    dungeon.numDownwardsStairs = htons(dungeon.numDownwardsStairs);
    dungeon.downwardStairs = std::vector<stair_t>(dungeon.numDownwardsStairs);
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


 //Saves the current dungeon configuration to disk
 
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
    
    fwrite(&dungeon.pc.getY(), sizeof(uint8_t), 1, f);
    fwrite(&dungeon.pc.getX(), sizeof(uint8_t), 1, f);
    
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
        fwrite(&dungeon.rooms[i].posY, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.rooms[i].posX, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.rooms[i].width, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.rooms[i].height, sizeof(uint8_t), 1, f);
    }
    
    // Writes numer of upward and downward stairs
    fwrite(&numUpStrs, sizeof(uint16_t), 1, f);
    for(int i = 0; i < dungeon.numUpwardsStairs; ++i) {
        fwrite(&dungeon.upwardStairs[i].posY, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.upwardStairs[i].posX, sizeof(uint8_t), 1, f);
    }
    fwrite(&numDownStrs, sizeof(uint16_t), 1, f);
    for(int i = 0; i < dungeon.numDownwardsStairs; ++i) {
        fwrite(&dungeon.downwardStairs[i].posY, sizeof(uint8_t), 1, f);
        fwrite(&dungeon.downwardStairs[i].posX, sizeof(uint8_t), 1, f);
    }
    fclose(f);
    free(path);
    return;
}
*/
