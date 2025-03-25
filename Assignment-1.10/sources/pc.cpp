#include "../headers/pc.hpp"
#include "../headers/dungeon.hpp"
#include "../headers/dungeon_game.hpp"
#include "../headers/priority_queue.h"

using namespace std;

// Constructor implementation
PC::PC(uint8_t x, uint8_t y, cell_t previousCell, int16_t speed, cell_t cell, int32_t HP, string DAM, vector<Item> inventory, direction_t direction, vector<Item> equippedItems, int damageDealt, int damageTaken, int distanceTraveled, int monstersKilled, int floorsVisited, int numItemsPickedUp, int numHealing, string name, int numCoins)
: Character(x, y, previousCell, speed, cell, HP, DAM, inventory), currentDirection(direction), equippedItems(equippedItems), damageDealt(damageDealt), damageTaken(damageTaken), distanceTraveled(distanceTraveled), monstersKilled(monstersKilled), floorsVisited(floorsVisited), numItemsPickedUp(numItemsPickedUp), numHealing(numHealing), name(name), numCoins(numCoins) {}

void useItem(char key) {
    vector<Item>& inventory = dungeon.getPC().getInventory();
    int index = (int (key) - '0'); // '0' ASCII is 48 in decimal
    if ((size_t) index >= inventory.size() || index < 0) {
        // invalid index handling, display message underneath slots
        clear();
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(16, 15, "%-70s", "Item number selected isn't valid. Please try again.");
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else {
        // use item
        Item item = inventory[index];
        if (item.getType() == "FLASK") { // only flask item types can be used for now
            if (item.getName() == "Potion of Healing") {
                potionInUse = HEALING;
                int healingAmount = rollDice(item.getAttribute());
                dungeon.getPC().setHealth(dungeon.getPC().getHealth() + healingAmount);
                gameMessage = "You drank a potion of healing! It gave you " + to_string(healingAmount) + " more HP!";
                potionInUse = NO_POTION;
            }
            else if (item.getName() == "Potion of Speed") {
                potionInUse = SPEED;
                turnsPassed = 0;
                gameMessage = "You drank a potion of speed! You will be faster temporarily!";
            }
            else if (item.getName() == "Potion of Invisibility") {
                potionInUse = INVISIBILITY;
                turnsPassed = 0;
                gameMessage = "You drank a potion of invisibility! Monsters can't see you temporarily!";
            }
            // remove item from inventory after use
            inventory.erase(inventory.begin() + index);
            clear();
            choosingCarryItem = NO_SELECT;
            dungeon.setModeType(PLAYER_CONTROL);
        }
        else {
            clear();
            attron(COLOR_PAIR(COLOR_RED));
            mvprintw(16, 30, "%-70s", "Item cannot be used.");
            attroff(COLOR_PAIR(COLOR_RED));
            refresh();
        }
    }
}

void buyItem(void) {
    vector<Item>& shopItems = dungeon.getShopItems();
    Item itemBought = shopItems[shopItems.size() - scrollOffset - 1];
    // show coins amount
    if (dungeon.getPC().getInventory().size() >= 10) {
        clear();
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(10, 20, "%-70s", "Not enough space in inventory!");
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else if (dungeon.getPC().getCoins() - itemBought.getValue() < 0) { // not enough coins
        clear();
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(10, 15, "%-70s", "You don't have enough coins to buy this item!");
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else {
        dungeon.getPC().getInventory().push_back(itemBought); // add bought item to inventory
        dungeon.getPC().setCoins(dungeon.getPC().getCoins() - itemBought.getValue());
        string bought = "You bought item " + itemBought.getName() + " for " + to_string(itemBought.getValue()) + " coins!\n\t\t\t (Item should be in your inventory now)";
        clear();
        attron(COLOR_PAIR(COLOR_GREEN));
        mvprintw(10, 20, "%-70s", bought.c_str());
        attroff(COLOR_PAIR(COLOR_GREEN));
        refresh();
    }
}

vector<string> validTypes = {
    "ARMOR", "HELMET", "GLOVES", "CLOAK", "BOOTS",
    "RING", "WEAPON", "OFFHAND", "RANGED", "LIGHT", "AMULET"
};

void displayStats(void) {
    int screenHeight, screenWidth;
    getmaxyx(stdscr, screenHeight, screenWidth);

    // Title (centered like in displaySettings)
    const char* title = "Player Statistics";
    size_t titleLength = strlen(title);
    size_t titleX = (screenWidth - titleLength) / 2;
    if (titleX < 0) titleX = 1;
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(0, (int) titleX, "%s", title);
    attroff(COLOR_PAIR(COLOR_CYAN));

    // Instructions (consistent with displaySettings)
    attron(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(screenHeight - 1, 1, "Use UP/DOWN to scroll, [s] to exit");
    attroff(COLOR_PAIR(COLOR_YELLOW));

    // Prepare stats list
    vector<string> statsList;

    PC& pc = dungeon.getPC();
    pair<int, int> damageRange = getMinAndMaxDamage();

    // Core stats
    statsList.push_back("Name: " + pc.getName());
    statsList.push_back("Health: " + to_string(pc.getHealth()));
    statsList.push_back("Speed: " + to_string(pc.getSpeed()));
    statsList.push_back("Coins collected: " + to_string(pc.getCoins()));
    statsList.push_back("Damage Dealt: " + to_string(pc.getDamageDealt()));
    statsList.push_back("Damage Range: " + to_string(damageRange.first) + " - " + to_string(damageRange.second));
    statsList.push_back("Floors Visited: " + to_string(pc.getFloorsVisited()));
    statsList.push_back("Monsters Killed: " + to_string(pc.getMonstersKilled()));
    statsList.push_back("Equipped Items: " + to_string(pc.getEquippedItems().size()));
    statsList.push_back("Items in Inventory: " + to_string(pc.getInventory().size()));
    statsList.push_back("Items Picked Up: " + to_string(pc.getNumItemsPickedUp()));
    statsList.push_back("Damage Taken: " + to_string(pc.getDamageTaken()));
    statsList.push_back("Healing Received: " + to_string(pc.getNumHealing()));
    statsList.push_back("Distance Traveled (No Teleport): " + to_string(pc.getDistanceTraveled()));
    statsList.push_back("Current Direction: " + directionMessage.substr(8));
    statsList.push_back("Current Floor: " + to_string(pc.getFloorsVisited() + 1));
   

    if (scrollOffset < 0) {
        scrollOffset = 0;
    }
    int maxLines = screenHeight - 3;
    int maxScrollOffset = (int) statsList.size() - maxLines;
    if (scrollOffset > maxScrollOffset && maxScrollOffset >= 0) {
        scrollOffset = maxScrollOffset;
    }
    if (maxScrollOffset < 0) {
        scrollOffset = 0;
    }

    for (int i = 0; i < maxLines && (i + scrollOffset) < (int) statsList.size(); i++) {
        int index = i + scrollOffset;
        //size_t length = strlen(statsList[index].c_str());
        //size_t xPos = (screenWidth - length) / 2;
        mvprintw(i + 1, 1, "%-50s", statsList[index].c_str());
    }
    refresh();
}

vector<Item> arrangeItems(void) {
    vector<Item> equippedItems = dungeon.getPC().getEquippedItems();
    vector<Item> arrangedEquipment = {};
    for (size_t i = 0; i < validTypes.size(); ++i) {
        string currEquipmentType = validTypes[i];
        for (size_t j = 0; j < equippedItems.size(); ++j) {
            if (equippedItems[j].getType() == currEquipmentType) {
                arrangedEquipment.push_back(equippedItems[j]);
            }
        }
    }
    return arrangedEquipment;
}

void takeOffItem(char key) {
    PC& pc = dungeon.getPC();
    vector<Item>& inventory = pc.getInventory();
    vector<Item> equippedItems = arrangeItems();
    int index = (int (key) - 'a'); // 'a' ASCII is 97 in decimal
    if ((size_t) index >= equippedItems.size() || index < 0) {
        // invalid index handling, display message underneath slots
        clear();
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(18, 17, "%-70s", "Item letter selected isn't valid. Please try again.");
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else if (inventory.size() >= 10) { // not enough space in inventory
        clear();
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(18, 17, "%-70s", "No open inventory slot to take off an item.");
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else {
        Item item = equippedItems[index];
        equippedItems.erase(equippedItems.begin() - index); // remove from equipped items
        inventory.push_back(item); // add to inventory
        // subtract speed from PC after taking off item
        pc.setSpeed(pc.getSpeed() - item.getSpeed());
        if (pc.getSpeed() <= 0) pc.setSpeed(1); // lowest speed is 1
        clear();
        choosingEquipmentItem = false;
        dungeon.setModeType(PLAYER_CONTROL);
        char message[200];
        snprintf(message, sizeof(message), "You took off item %s!", item.getName().c_str());
        gameMessage = message;
    }
    dungeon.getPC().getEquippedItems() = equippedItems;
}


void displayEquipment(void) {
    // armor, helmet, gloves, cloak, boots, ring, weapon, offhand, ranged, light, amulet
    vector<Item> equippedItems = arrangeItems(); // sorted into slots
    int screenHeight, screenWidth;
    getmaxyx(stdscr, screenHeight, screenWidth);

    const char* title = "";
    const char* footer = "";
    if (choosingEquipmentItem) {
        title = "Take Off Item (Equipment Slots)";
        footer = "Use [esc] to return and keys [a] - [l] to select an item to take off.";
    }
    else {
        title = "Equipped Items (Equipment Slots)";
        footer = "Use [e] to exit.";
    }
    size_t titleLength = strlen(title);
    size_t titleX = (screenWidth - titleLength) / 2;
    if (titleX < 0) titleX = 1;
    attron(COLOR_PAIR(COLOR_MAGENTA));
    mvprintw(0, (int) titleX, "%s", title);
    attroff(COLOR_PAIR(COLOR_MAGENTA));
    // Header (Number, Symbol, Name, Rarity, Type, Artifact)
    mvprintw(1, 1, "Num | Symbol | Name                      | Rarity     | Type       | Artifact");
    mvprintw(2, 1, "--- | ------ | ------------------------- | ---------- | ---------- | --------");

    int maxVisibleItems = screenHeight - 5; // Title, header, dashed line, instructions, buffer
    if (maxVisibleItems > 12) maxVisibleItems = 12;

    for (int i = 0; i < maxVisibleItems; ++i) {
        char charStr[5];
        snprintf(charStr, sizeof(charStr), "[%c]", (char) (i + 'a'));
        mvprintw(i + 3, 1, "%-3s", charStr);
        mvprintw(i + 3, 4, " | ");

        if ((size_t) i < equippedItems.size()) {
            const Item& item = equippedItems[i];

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
    PC& pc = dungeon.getPC();
    vector<Item>& inventory = pc.getInventory();
    vector<Item>& equippedItems = pc.getEquippedItems();
    int index = (int (key) - '0'); // '0' ASCII is 48 in decimal
    if ((size_t) index >= inventory.size() || index < 0) {
        // invalid index handling, display message underneath slots
        clear();
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(16, 15, "%-70s", "Item number selected isn't valid. Please try again."); // "Empty" in Name
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else {
        Item item = inventory[index];
        if (containsString(validTypes, item.getType())) { // can be equipped
            // swap item if already another of same type equipped, else equip/wear item
            bool skip = item.getType() == "RING" && getNumRings() == 2; // only if the item we want to swap is a ring and theres two of them
            int randInt = rand() % 2;
            bool inserted = false;
            for (size_t i = 0; i < equippedItems.size(); ++i) {
                if (item.getType() == equippedItems[i].getType()) {
                    if (getNumRings() < 2 && item.getType() == "RING") {
                        equippedItems.push_back(item);
                        inventory.erase(inventory.begin() + index);
                        inserted = true;
                        break;
                    }
                    else if (skip && randInt == 0) {
                        skip = false; // if we skip the first one then we won't skip the next one
                        continue; // speical case where there is two rings and the equal random chance of skipping the first ring and swapping out the second happens
                    }
                    else {
                        // swap items
                        Item temp = equippedItems[i];
                        equippedItems[i] = inventory[index];
                        inventory[index] = temp;
                        inserted = true;
                        // subtract bonuses from item taken off
                        pc.setSpeed(pc.getSpeed() - equippedItems[i].getSpeed());
                        // add bonuses from item put on
                        pc.setSpeed(pc.getSpeed() + item.getSpeed());
                        break;
                    }
                }
            }
            
            if (!inserted) { // equip item without swapping
                equippedItems.push_back(item); // add item to equipped items
                inventory.erase(inventory.begin() + index); // remove item from inventory
                // add bonuses from item put on
                pc.setSpeed(pc.getSpeed() + item.getSpeed());
            }
            if (pc.getSpeed() <= 0) pc.setSpeed(1); // lowest speed is 1
                    
            clear();
            choosingCarryItem = NO_SELECT;
            dungeon.setModeType(PLAYER_CONTROL);
            char message[200];
            snprintf(message, sizeof(message), "You equipped yourself with %s %s!", item.getType().c_str(), item.getName().c_str());
            gameMessage = message;
        }
        else {
            // item can't be worn, display message underneath slots
            clear();
            attron(COLOR_PAIR(COLOR_RED));
            mvprintw(16, 15, "%-70s", "Item selected cannot be equipped. Please try again."); // "Empty" in Name
            attroff(COLOR_PAIR(COLOR_RED));
            refresh();
        }
    }
}

void expungeItem(char key) {
    vector<Item>& inventory = dungeon.getPC().getInventory();
    int index = (int (key) - '0'); // '0' ASCII is 48 in decimal
    if ((size_t) index >= inventory.size() || index < 0) {
        // invalid index handling, display message underneath slots
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(16, 15, "%-70s", "Item number selected isn't valid. Please try again.");
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else {
        Item item = inventory[index];
        inventory.erase(inventory.begin() + index); // remove item from inventory and don't add save it anywhere else, effectively reomving it from the game forever
        clear();
        choosingCarryItem = NO_SELECT;
        dungeon.setModeType(PLAYER_CONTROL);
        char message[200];
        snprintf(message, sizeof(message), "You removed item %s from the game!", item.getName().c_str());
        gameMessage = message;
    }
}

void dropItem(char key) {
    vector<Item>& inventory = dungeon.getPC().getInventory();
    int index = (int (key) - '0'); // '0' ASCII is 48 in decimal
    if ((size_t) index >= inventory.size() || index < 0) {
        // invalid index handling, display message underneath slots
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(16, 15, "%-70s", "Item number selected isn't valid. Please try again.");
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else {
        // drop item onto floor
        Item item = inventory[index];
        inventory.erase(inventory.begin() + index);
        char symbol = getSymbolFromType(item.getType());
        dungeon.getPC().setPreviousCell(cell_t { symbol, -2 });
        dungeon.getItemMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()].push_back(item);
        clear();
        choosingCarryItem = NO_SELECT;
        dungeon.setModeType(PLAYER_CONTROL);
        char message[200];
        snprintf(message, sizeof(message), "You dropped item %s onto the floor!", item.getName().c_str());
        gameMessage = message;
    }
    
}

void inspectItem(char key) {
    vector<Item>& inventory = dungeon.getPC().getInventory();
    int index = (int (key) - '0'); // '0' ASCII is 48 in decimal
    if ((size_t) index >= inventory.size() || index < 0) {
        // invalid index handling, display message underneath slots
        clear();
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(16, 15, "%-70s", "Item number selected isn't valid. Please try again.");
        attroff(COLOR_PAIR(COLOR_RED));
        refresh();
    }
    else {
        clear();
        int screenHeight, screenWidth;
        getmaxyx(stdscr, screenHeight, screenWidth);
        Item item = inventory[index];
        char title[50];
        snprintf(title, sizeof(title), "Item Details for [%s]", item.getName().c_str());
        size_t titleLength = strlen(title);
        size_t titleX = (screenWidth - titleLength) / 2;
        size_t titleY = screenHeight / 2 + 1;  // Use screenHeight to center vertically
        if (titleX < 0) titleX = 1;
        if (titleY < 0) titleY = 1;
        attron(COLOR_PAIR(COLOR_GREEN));
        mvprintw((int) titleY, (int)titleX, "%s", title);  // Use titleY instead of hardcoded 13
        attroff(COLOR_PAIR(COLOR_GREEN));

        string desc = item.getDescription();
        size_t charsPerLine = 81;
        stringstream ss2(desc);
        string word;
        vector<string> lines(5);
        int currentLine = 0;

        // Build lines with word wrapping
        while (ss2 >> word && currentLine < 5) {
            string& line = lines[currentLine];
            if (line.empty()) {
                line = word;
            } else if (line.length() + 1 + word.length() <= charsPerLine) {
                line += " " + word; // Add space and word if it fits
            } else {
                currentLine++; // Move to next line
                if (currentLine < 5) {
                    lines[currentLine] = word; // Start new line with the word
                }
            }
        }

        // Print the lines
        for (int i = 0; i < 5; ++i) {
            if (!lines[i].empty()) {
                mvprintw(14 + i, 1, "%-81s", lines[i].c_str());
            }
        }
        
        // Defense points
        string defLabel = "Defense points added: ";
        string defValue = to_string(item.getDefense());
        string defText = defLabel + defValue;
        size_t defX = (screenWidth - defText.length()) / 2;
        if (defX < 0) defX = 1;
        attron(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(19, (int)defX, "%s", defLabel.c_str());
        attroff(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(19, (int)(defX + defLabel.length()), "%s", defValue.c_str());

        // Hitpoints
        string hitLabel = "Hitpoints added: ";
        string hitValue = to_string(item.getHit());
        string hitText = hitLabel + hitValue;
        size_t hitX = (screenWidth - hitText.length()) / 2;
        if (hitX < 0) hitX = 1;
        attron(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(20, (int)hitX, "%s", hitLabel.c_str());
        attroff(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(20, (int)(hitX + hitLabel.length()), "%s", hitValue.c_str());

        // Speed
        string speedLabel = "Speed gained: ";
        string speedValue = to_string(item.getSpeed());
        string speedText = speedLabel + speedValue;
        size_t speedX = (screenWidth - speedText.length()) / 2;
        if (speedX < 0) speedX = 1;
        attron(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(21, (int)speedX, "%s", speedLabel.c_str());
        attroff(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(21, (int)(speedX + speedLabel.length()), "%s", speedValue.c_str());

        string damStr = item.getDamage();
        int base = 0, dice = 0, sides = 0;
        char d; // To hold 'd' character
        stringstream ss(damStr);
        if (damStr.find('+') != string::npos) {
            ss >> base >> d >> dice >> d >> sides; // Parse "base+diced sides"
        } else {
            ss >> dice >> d >> sides; // Parse "diced sides" (no base)
        }
        int minDamage = base + dice; // Min: base + 1 per die
        int maxDamage = base + (dice * sides); // Max: base + max roll per die
        string damRange = to_string(minDamage) + " - " + to_string(maxDamage);

        // Damage (as range)
        string damLabel = "Damage added: ";
        string damText = damLabel + damRange;
        size_t damX = (screenWidth - damText.length()) / 2;
        if (damX < 0) damX = 1;
        attron(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(22, (int)damX, "%s", damLabel.c_str());
        attroff(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(22, (int)(damX + damLabel.length()), "%s", damRange.c_str());

        refresh();
    }
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
            footer = "Use [esc] to return and keys [0] - [9] to select an item to wear/equip.";
            break;
        case EXPUNGE_ITEM:
            title = "Expunge Item (Carry Slots)";
            footer = "Use [esc] to return and keys [0] - [9] to select an item to expunge/remove.";
            break;
        case DROP_ITEM:
            title = "Drop Item (Carry Slots)";
            footer = "Use [esc] to return and keys [0] - [9] to select an item to drop.";
            break;
        case INSPECT_ITEM:
            title = "Inspect Item (Carry Slots)";
            footer = "Use [esc] to return and keys [0] - [9] to select an item to inspect.";
            break;
        case USE_ITEM:
            title = "Use Item (Carry Slots)";
            footer = "Use [esc] to return and keys [0] - [9] to select an item to use.";
            break;
    }
    size_t titleLength = strlen(title);
    size_t titleX = (screenWidth - titleLength) / 2;
    if (titleX < 0) titleX = 1;
    attron(COLOR_PAIR(COLOR_MAGENTA));
    mvprintw(0, (int) titleX, "%s", title);
    attroff(COLOR_PAIR(COLOR_MAGENTA));
    // Header (Number, Symbol, Name, Rarity, Type, Artifact)
    mvprintw(1, 1, "Num | Symbol | Name                      | Rarity     | Type       | Artifact");
    mvprintw(2, 1, "--- | ------ | ------------------------- | ---------- | ---------- | --------");

    // Display all 10 slots ([0] to [9])
    int maxVisibleItems = screenHeight - 5; // Title, header, dashed line, instructions, buffer
    if (maxVisibleItems > 10) maxVisibleItems = 10; // Cap at 10 items

    for (int i = 0; i < maxVisibleItems; ++i) {
        // Item number [0] to [9]
        mvprintw(i + 3, 1, "[%d]", i);
        mvprintw(i + 3, 4, " | ");

        if ((size_t) i < inventory.size()) {
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
    size_t itemIndex = items.size() - scrollOffset - 1;
    if (dungeon.getPC().getInventory().size() < 10) {
        if (items[itemIndex].getType() == "FOOD") {
            int healthRegen = rollDice(items[itemIndex].getAttribute());
            dungeon.getPC().setHealth(dungeon.getPC().getHealth() + healthRegen);
            gameMessage = "You consumed FOOD item " + items[itemIndex].getName() +"! It gave you " + to_string(healthRegen) + " more HP!";
            dungeon.getPC().addNumHealing(healthRegen);
        }
        else if (items[itemIndex].getType() == "GOLD") {
            int numCoins = rollDice(items[itemIndex].getAttribute());
            dungeon.getPC().setCoins(dungeon.getPC().getCoins() + numCoins);
            gameMessage = "A treasure! You found " + to_string(numCoins) + " coins on the dungeon floor!";
        }
        else {
            dungeon.getPC().getInventory().push_back(items[itemIndex]);
            char message[100];
            snprintf(message, sizeof(message), "You picked up %s!", items[itemIndex].getName().c_str());
            gameMessage = message;
        }
        // Remove item from map
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
        dungeon.getPC().addNumItemsPickedUp(1);
        if (items[itemIndex].isArtifact()) { // ineligible since picked up and is artifact
            invalidItemsAndMonsters.push_back(items[itemIndex].getName());
        }
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
        dungeon.getPC().addFloorsVisited(1);
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


void attack(int attackX, int attackY, int oldX, int oldY, Monster *m) {
    bool weaponEquipped = false;
    int totalDamage = 0;
    for (Item i: dungeon.getPC().getEquippedItems()) {
        totalDamage += rollDice(i.getDamage());
        if (i.getType() == "WEAPON") {
            weaponEquipped = true;
        }
    }
    if (!weaponEquipped) { // just bare handed
        totalDamage += rollDice("0+1d4");
    }
    
    dungeon.getPC().addDamageDealt(totalDamage);
    m->setHealth(m->getHealth() - totalDamage);
    if (m->getHealth() > 0) { // still alive
        gameMessage = "You attacked monster " + m->getName() + "!";
    }
    else {
        dungeon.getPC().addMonstersKilled(1);
        m->setAlive(false);
        if (containsString(m->getAbilities(), string("UNIQ"))) {
            invalidItemsAndMonsters.push_back(m->getName());
        }
        vector<Item>& inventory = m->getInventory();
        cell_t monsterOriginalCell = m->getPreviousCell(); // Original cell under monster
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
        snprintf(message, sizeof(message), "You killed monster %s!", m->getName().c_str());
        gameMessage = message;
        calculateDistances(0); // recalculate pathfinding maps after moving player
        calculateDistances(1);
        calculateDistances(2);
    }
    playerToMove = false;
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
    
    for (size_t i = 0; i < dungeon.getMonsters().size(); ++i) {
        if (dungeon.getMonsters()[i].isAlive() && dungeon.getMonsters()[i].getPosX() == newX && dungeon.getMonsters()[i].getPosY() == newY) {
            // attack monster
            attack(newX, newY, oldX, oldY, &dungeon.getMonsters()[i]);
            return;
        }
    }
    
    if (newY >= 0 && newY < DUNGEON_HEIGHT &&
        newX >= 0 && newX < DUNGEON_WIDTH &&
        dungeon.getMap()[newY][newX].hardness <= 0) {
        if (dungeon.getMap()[newY][newX].hardness == -3) {
            gameMessage = "Stop trying to run into the shopkeeper!!!";
        }
        else {
            dungeon.getMap()[oldY][oldX] = dungeon.getPC().getPreviousCell();
            dungeon.getPC().setPreviousCell(dungeon.getMap()[newY][newX]);
            dungeon.getPC().setPosX(newX);
            dungeon.getPC().setPosY(newY);
            dungeon.getMap()[newY][newX] = PLAYER_CELL;
            changeDirection(false, true);
            playerToMove = false;
            dungeon.getPC().addDistanceTraveled(1);
            if (dungeon.getPC().getPreviousCell().ch == '<') {
                gameMessage = "You are on upward stairs! Press [<] to go up a level.";
            }
            else if (dungeon.getPC().getPreviousCell().ch == '>') {
                gameMessage = "You are on downward stairs! Press [>] to go down a level.";
            }
            else if (dungeon.getPC().getPreviousCell().ch == '%') {
                gameMessage = "Press [T] to talk to the shopkeeper and look at what you can buy!";
            }
            else {
                gameMessage = "Adventuring...";
            }
        }
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

int PC::getDamageDealt() const {
    return damageDealt;
}
int PC::getDamageTaken() const {
    return damageTaken;
}
int PC::getDistanceTraveled() const {
    return distanceTraveled;
}
int PC::getMonstersKilled() const {
    return monstersKilled;
}
int PC::getFloorsVisited() const {
    return floorsVisited;
}
int PC::getNumItemsPickedUp() const {
    return numItemsPickedUp;
}
int PC::getNumHealing() const {
    return numHealing;
}

void PC::addDamageDealt(int stat) {
    damageDealt += stat;
}
void PC::addDamageTaken(int stat) {
    damageTaken += stat;
}
void PC::addDistanceTraveled(int stat) {
    distanceTraveled += stat;
}
void PC::addMonstersKilled(int stat) {
    monstersKilled += stat;
}
void PC::addFloorsVisited(int stat) {
    floorsVisited += stat;
}
void PC::addNumItemsPickedUp(int stat) {
    numItemsPickedUp += stat;
}
void PC::addNumHealing(int stat) {
    numHealing += stat;
}
void PC::setName(string givenName) {
    name = givenName;
}
string PC::getName() const {
    return name;
}
void PC::setCoins(int coins) {
    numCoins = coins;
}
int PC::getCoins() const {
    return numCoins;
}
