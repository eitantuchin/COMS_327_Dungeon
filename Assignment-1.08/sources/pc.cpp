#include "../headers/pc.hpp"
#include "../headers/dungeon.hpp"
#include "../headers/dungeon_game.hpp"

using namespace std;

// Constructor implementation
PC::PC(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, direction_t direction)
: Character(x, y, previousCell, speed, cell), currentDirection(direction) {}

// Getters
direction_t PC::getCurrentDirection() const {
    return currentDirection;
}

bool (&PC::getFogMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return fogMap;
}

// Setters
void PC::setCurrentDirection(direction_t direction) {
    currentDirection = direction;
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
        dungeon.getMap()[pointerPos.second][pointerPos.first] = targetingPointerPreviousCell;
        snprintf(message, sizeof(message), "You teleported randomly to X-coord: %i and Y-coord: %i!", randX, randY);
        gameMessage = message;
    } else {
        // Only teleport if the target is different
        if (dungeon.getPC().getPosX() != pointerPos.first || dungeon.getPC().getPosY() != pointerPos.second) {
            dungeon.getMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()] = dungeon.getPC().getPreviousCell();
            dungeon.getPC().setPreviousCell(targetingPointerPreviousCell);
            dungeon.getPC().setPosX(pointerPos.first);
            dungeon.getPC().setPosY(pointerPos.second);
            dungeon.getMap()[pointerPos.second][pointerPos.first] = PLAYER_CELL;
            snprintf(message, sizeof(message), "You teleported to X-coord: %i and Y-coord: %i!", pointerPos.first, pointerPos.second);
            gameMessage = message;
        } else {
            // Teleporting to the same spot, don't change previous cell
            dungeon.getMap()[pointerPos.second][pointerPos.first] = targetingPointerPreviousCell;
            snprintf(message, sizeof(message), "You remained at X-coord: %i and Y-coord: %i!", pointerPos.first, pointerPos.second);
            gameMessage = message;
        }
    }
    calculateDistances(0);
    calculateDistances(1);
    calculateDistances(2);
}


void useStairs(int key) {
    if (dungeon.getPC().getPreviousCell().ch == key) {
        if (key == '<') gameMessage = "Moved up a dungeon level!";
        else gameMessage = "Moved down a dungeon level!";
        resetDungeonLevel();
    }
}

void changeDirection(bool clockwise, bool justChangeText) {
    // Define the order of directions (clockwise and counter-clockwise)
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
        directionMessage = message;
    }
    else {
        // Create the message without changing the direction
        snprintf(message, sizeof(message), "Facing: %s", dirNames[currentDirIndex].c_str());
        directionMessage = message;
    }
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
                    // if the monster is unique then it can't be spawned anymore
                    if (contains(monster->getAbilities(), string("UNIQ"))) {
                        monster->setElgibile(false);
                    }
                    vector<Item>& inventory = monster->getInventory();
                    // if the monster has items in its inventory drop them
                    while (!inventory.empty()) {
                        // Move the last item to the floor
                        dungeon.getItemMap()[oldY][oldX].push_back(inventory.back());
                        inventory.pop_back(); // Remove the last item from the inventory
                    }
                    for (Item i: dungeon.getItemMap()[oldY][oldX]) {
                        i.setPosX(oldX);
                        i.setPosY(oldY);
                        i.setPreviousCell(dungeon.getPC().getPreviousCell());
                    }
                    dungeon.getMap()[attackY][attackX] = PLAYER_CELL;
                    vector<Item> items = dungeon.getItemMap()[oldY][oldX];
                    if (!items.empty()) {
                        char symbol = getSymbolFromType(items.back().getType());
                        dungeon.getMap()[oldY][oldX] = cell_t {symbol, -2};
                    }
                    else {
                        dungeon.getMap()[oldY][oldX] = dungeon.getPC().getPreviousCell();
                    }
                    dungeon.getPC().setPosX(attackX);
                    dungeon.getPC().setPosY(attackY);
                    dungeon.getPC().setPreviousCell(monster->getPreviousCell());
                    char message[100];
                    snprintf(message, sizeof(message), "You killed monster %s!", monster->getName().c_str());
                    gameMessage = message;
                    return;
                }
            }
        }
    }
    // Display a message to the player if we get to the end and we didn't attack anything
    gameMessage = "Attacked nothing! Get close to a monster and face them!";
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
        dungeon.getMap()[oldY][oldX] = dungeon.getPC().getPreviousCell();
        dungeon.getPC().setPreviousCell(dungeon.getMap()[newY][newX]);
        dungeon.getPC().setPosX(newX);
        dungeon.getPC().setPosY(newY);
        dungeon.getMap()[newY][newX] = PLAYER_CELL;
        changeDirection(false, true);
        playerToMove = false;
    }
    else {
        gameMessage = "Uh oh! There's rock there!";
    }
    calculateDistances(0); // recalculate pathfinding maps after moving player
    calculateDistances(1);
    calculateDistances(2);
}
