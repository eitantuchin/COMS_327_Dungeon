#include "../headers/pc.hpp"
#include "../headers/dungeon.hpp"
#include "../headers/dungeon_game.hpp"

using namespace std;

// Constructor implementation
PC::PC(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, uint16_t HP, string DAM, vector<Item> inventory, direction_t direction, vector<Item> equippedItems)
: Character(x, y, previousCell, speed, cell, HP, DAM, inventory), currentDirection(direction), equippedItems(equippedItems) {}

vector<string> validTypes = {
    "WEAPON", "OFFHAND", "RANGED", "ARMOR", "HELMET",
    "CLOAK", "GLOVES", "BOOTS", "AMULET", "LIGHT", "RING"
};

int getNumRings(void) {
    int count = 0;
    for (Item i: dungeon.getPC().getEquippedItems()) {
        if (i.getType() == "RING") {
            count++;
        }
    }
    return count;
}

void wearItem(char key) {
    vector<Item> inventory = dungeon.getPC().getInventory();
    vector<Item> equippedItems = dungeon.getPC().getEquippedItems();
    int index = (int (key) - '0'); // '0' ASCII is 48 in decimal
    if (index >= inventory.size() || index < 0) {
        // invalid index handling, display message underneath slots
    }
    else {
        Item item = inventory[index];
        if (containsString(validTypes, item.getType())) {
            // swap item if already another of same type equipped, else equip/wear item
            bool skip = item.getType() == "RING" && getNumRings() == 2; // only if the item we want to swap is a ring and theres two of them
            int randInt = rand() % 2;
            bool swapped = false;
            for (size_t i = 0; i < equippedItems.size(); ++i) {
                if (item.getType() == equippedItems[i].getType()) {
                    if (skip && randInt == 0) {
                        skip = false; // if we skip the first one then we won't skip the next one
                        continue; // speical case where there is two rings and the equal random chance of skipping the first ring and swapping out the second happens
                    }
                    else {
                        // swap items
                        Item temp = equippedItems[i];
                        equippedItems[i] = inventory[index];
                        inventory[index] = temp;
                        swapped = true;
                    }
                }
            }
            
            if (!swapped) { // equip item without swapping
                equippedItems.push_back(item); // add item to equipped items
                inventory.erase(inventory.begin() + index); // remove item from inventory
            }
        
            clear();
            choosingCarryItem = NO_SELECT;
            dungeon.setModeType(PLAYER_CONTROL);
            char message[200];
            snprintf(message, sizeof(message), "You equipped yourself with %s %s!", item.getType().c_str(), item.getName().c_str());
            gameMessage = message;
        }
        else {
            // item can't be worn, display message underneath slots
        }
    }
}

void dropItem(char key) {
    
}

void expungeItem(char key){
    
}

void displayEquipment(void) {
    
}

void displayInventory(void) {
    // Get the player's inventory and filter for non-equipped items
    vector<Item>& inventory = dungeon.getPC().getInventory();

    // Screen dimensions
    int screenHeight, screenWidth;
    getmaxyx(stdscr, screenHeight, screenWidth);

    // Center "Inventory (Carry Slots)" title
    const char* title = "";
    const char* footer = "";
    switch (choosingCarryItem) {
        case NO_SELECT:
            title = "Inventory (Carry Slots)";
            footer = "Use [i] to exit.";
            break;
        case WEAR_ITEM:
            title = "Wear Item (Carry Slots)";
            footer = "Use [esc] to return and keys [0] - [9] to select an item to wear.";
            break;
        case EXPUNGE_ITEM:
            title = "Expunge Item (Carry Slots)";
            footer = "Use [esc] to return and keys [0] - [9] to select an item to expunge.";
            break;
        case DROP_ITEM:
            title = "Drop Item (Carry Slots)";
            footer = "Use [esc] to return and keys [0] - [9] to select an item to drop.";
            break;
    }
    size_t titleLength = strlen(title);
    size_t titleX = (screenWidth - titleLength) / 2;
    if (titleX < 0) titleX = 1;
    mvprintw(0, (int) titleX, "%s", title);

    // Header (Number, Symbol, Name, Rarity, Type, Artifact)
    mvprintw(1, 1, "Num | Symbol | Name                      | Rarity     | Type       | Artifact");
    mvprintw(2, 1, "--- | ------ | ------------------------- | ---------- | ---------- | --------");

    // Display all 10 slots ([0] to [9])
    int maxVisibleItems = screenHeight - 5; // Title, header, dashed line, instructions, buffer
    if (maxVisibleItems > 10) maxVisibleItems = 10; // Cap at 10 items

    for (int i = 0; i < maxVisibleItems; ++i) {
        // Item number [0] to [9]
        char numStr[5];
        snprintf(numStr, sizeof(numStr), "[%d]", i);
        mvprintw(i + 3, 1, "%-3s", numStr);
        mvprintw(i + 3, 4, " | ");

        if (i < inventory.size()) {
            const Item& item = inventory[i];

            // Symbol with item color
            short color = item.getColor()[0]; // Single color per item
            attron(COLOR_PAIR(color));
            mvprintw(i + 3, 7, "%-6s", string(1, getSymbolFromType(item.getType())).c_str());
            attroff(COLOR_PAIR(color));
            mvprintw(i + 3, 13, " | ");

            // Name
            string name = item.getName().substr(0, 25); // Limit to 25 chars
            mvprintw(i + 3, 16, "%-25s", name.c_str());
            mvprintw(i + 3, 41, " | ");

            // Rarity with color coding
            uint8_t rarity = item.getRarity();
            string rarityText;
            int colorPair;
            if (rarity > 80) {
                rarityText = "LEGENDARY";
                colorPair = COLOR_YELLOW;
            } else if (rarity > 60) {
                rarityText = "EPIC";
                colorPair = COLOR_MAGENTA;
            } else if (rarity > 40) {
                rarityText = "RARE";
                colorPair = COLOR_BLUE;
            } else if (rarity > 20) {
                rarityText = "UNCOMMON";
                colorPair = COLOR_CYAN;
            } else {
                rarityText = "COMMON";
                colorPair = COLOR_WHITE;
            }
            attron(COLOR_PAIR(colorPair));
            mvprintw(i + 3, 44, "%-10s", rarityText.c_str());
            attroff(COLOR_PAIR(colorPair));
            mvprintw(i + 3, 54, " | ");

            // Type
            string type = item.getType().substr(0, 10); // Limit to 10 chars
            mvprintw(i + 3, 57, "%-10s", type.c_str());
            mvprintw(i + 3, 67, " | ");

            // Artifact (YES or NO)
            string artifactText = item.isArtifact() ? "YES" : "NO";
            mvprintw(i + 3, 70, "%-8s", artifactText.c_str());
        } else {
            // Empty slot
            mvprintw(i + 3, 7, "%-6s", "      "); // Blank Symbol
            mvprintw(i + 3, 13, " | ");
            mvprintw(i + 3, 16, "%-25s", "Empty"); // "Empty" in Name
            mvprintw(i + 3, 41, " | ");
            mvprintw(i + 3, 44, "%-10s", ""); // Blank Rarity
            mvprintw(i + 3, 54, " | ");
            mvprintw(i + 3, 57, "%-10s", ""); // Blank Type
            mvprintw(i + 3, 67, " | ");
            mvprintw(i + 3, 70, "%-8s", ""); // Blank Artifact
        }
    }

    // Instructions
    mvprintw(screenHeight - 1, 1, footer);

    refresh();
}

void pickupItem(void) {
    vector<Item>& items = dungeon.getItemMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()];
    if (dungeon.getPC().getInventory().size() < 10) {
        size_t itemIndex = items.size() - selectedItemIndex - 1;
        dungeon.getPC().getInventory().push_back(items[itemIndex]);
        // Remove item from map
        char message[100];
        snprintf(message, sizeof(message), "You picked up %s!", items[itemIndex].getName().c_str());
        gameMessage = message;
        cell_t prevCell = items[itemIndex].getPreviousCell();
        items.erase(items.begin() + itemIndex);
        if (!items.empty()) {
            char symbol = getSymbolFromType(items[items.size() - 1].getType());
            dungeon.getPC().setPreviousCell(cell_t {symbol, -2});
        }
        else {
            dungeon.getPC().setPreviousCell(prevCell);
        }
        dungeon.getItemMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()] = items;
    }
    else {
        gameMessage = "Your inventory is too full to pickup another item!";
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
    int attackX = dungeon.getPC().getPosX();
    int attackY = dungeon.getPC().getPosY();
    int oldX = attackX;
    int oldY = attackY;

    direction_t directionsToCheck[3];
    switch (dungeon.getPC().getCurrentDirection()) {
        case UP: directionsToCheck[0] = UP; directionsToCheck[1] = UP_LEFT; directionsToCheck[2] = UP_RIGHT; break;
        case DOWN: directionsToCheck[0] = DOWN; directionsToCheck[1] = DOWN_LEFT; directionsToCheck[2] = DOWN_RIGHT; break;
        case LEFT: directionsToCheck[0] = LEFT; directionsToCheck[1] = UP_LEFT; directionsToCheck[2] = DOWN_LEFT; break;
        case RIGHT: directionsToCheck[0] = RIGHT; directionsToCheck[1] = UP_RIGHT; directionsToCheck[2] = DOWN_RIGHT; break;
        case UP_LEFT: directionsToCheck[0] = UP_LEFT; directionsToCheck[1] = UP; directionsToCheck[2] = LEFT; break;
        case UP_RIGHT: directionsToCheck[0] = UP_RIGHT; directionsToCheck[1] = UP; directionsToCheck[2] = RIGHT; break;
        case DOWN_LEFT: directionsToCheck[0] = DOWN_LEFT; directionsToCheck[1] = DOWN; directionsToCheck[2] = LEFT; break;
        case DOWN_RIGHT: directionsToCheck[0] = DOWN_RIGHT; directionsToCheck[1] = DOWN; directionsToCheck[2] = RIGHT; break;
        default: return;
    }

    for (int i = 0; i < 3; i++) {
        attackX = dungeon.getPC().getPosX();
        attackY = dungeon.getPC().getPosY();

        switch (directionsToCheck[i]) {
            case UP: attackY -= distance; break;
            case DOWN: attackY += distance; break;
            case LEFT: attackX -= distance; break;
            case RIGHT: attackX += distance; break;
            case UP_LEFT: attackY -= distance; attackX -= distance; break;
            case UP_RIGHT: attackY -= distance; attackX += distance; break;
            case DOWN_LEFT: attackY += distance; attackX -= distance; break;
            case DOWN_RIGHT: attackY += distance; attackX += distance; break;
            default: break;
        }

        if (attackY >= 0 && attackY < DUNGEON_HEIGHT && attackX >= 0 && attackX < DUNGEON_WIDTH) {
            for (int j = 0; j < dungeon.getNumMonsters(); j++) {
                Monster* monster = &dungeon.getMonsters()[j];
                if (monster->isAlive() && monster->getPosX() == attackX && monster->getPosY() == attackY) {
                    monster->setAlive(false);
                    if (containsString(monster->getAbilities(), string("UNIQ"))) {
                        invalidItemsAndMonsters.push_back(monster->getName());
                    }
                    vector<Item>& inventory = monster->getInventory();
                    cell_t monsterOriginalCell = monster->getPreviousCell(); // Original cell under monster
                    for (Item& item : inventory) {
                        item.setPosX(oldX);
                        item.setPosY(oldY);
                        item.setPreviousCell(monsterOriginalCell); // Set to what was under the monster
                        dungeon.getItemMap()[oldY][oldX].push_back(item);
                    }
                    inventory.clear();
                    dungeon.getMap()[attackY][attackX] = PLAYER_CELL;
                    vector<Item>& items = dungeon.getItemMap()[oldY][oldX];
                    if (!items.empty()) {
                        char symbol = getSymbolFromType(items.back().getType());
                        dungeon.getMap()[oldY][oldX] = cell_t{symbol, -2};
                    } else {
                        dungeon.getMap()[oldY][oldX] = dungeon.getPC().getPreviousCell();
                    }
                    dungeon.getPC().setPosX(attackX);
                    dungeon.getPC().setPosY(attackY);
                    dungeon.getPC().setPreviousCell(monsterOriginalCell);
                    char message[100];
                    snprintf(message, sizeof(message), "You killed monster %s!", monster->getName().c_str());
                    gameMessage = message;
                    return;
                }
            }
        }
    }
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

vector<Item>& PC::getEquippedItems() {
    return equippedItems;
}
