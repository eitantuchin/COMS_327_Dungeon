//
//  item.cpp
//  Dungeon_Game_327
//
//  Created by Eitan Tuchin on 2/20/25.
//

#include "../headers/item.hpp"
#include "../headers/parse.hpp"
#include "../headers/dungeon_game.hpp"
#include "../headers/dungeon.hpp"
#include <memory>
#include <string>
#include <sstream>
#include <ncurses.h>

using namespace std;

// Constructor
Item::Item(uint8_t posX, uint8_t posY, string NAME, string DESC, string TYPE, vector<short> COLOR, string DAM, u_int16_t HIT, u_int16_t DODGE, u_int16_t DEF, u_int16_t WEIGHT, u_int16_t SPEED, u_int16_t ATTR, u_int16_t VAL, bool ART, u_int8_t RRTY, cell_t previousCell, bool  eligibility, bool equipped)
: posX(posX), posY(posY), NAME(NAME), DESC(DESC), TYPE(TYPE), COLOR(COLOR), DAM(DAM), HIT(HIT), DODGE(DODGE), DEF(DEF), WEIGHT(WEIGHT), SPEED(SPEED), ATTR(ATTR), VAL(VAL), ART(ART), RRTY(RRTY), previousCell(previousCell), eligibility(eligibility),  equipped(equipped) {}
/*
void updateMapForItemCells(void) {
    for (int y = 0; y < DUNGEON_HEIGHT; ++y) {
        for (int x = 0; x < DUNGEON_WIDTH; ++x) {
            vector<Item> items = dungeon.getItemMap()[y][x];
            if (!items.empty() && dungeon.getMap()[y][x].hardness != -1) {
                Item i = items.back();
                char symbol = getSymbolFromType(i.getType());
                dungeon.getMap()[y][x] = cell_t { symbol, -2 };
            }
        }
    }
}
 */

void displayItemMenu(void) {
    // Get items at PC's current position
    vector<Item> items = dungeon.getItemMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()];
    uint8_t count = items.size();

    // Screen dimensions
    int screenHeight, screenWidth;
    getmaxyx(stdscr, screenHeight, screenWidth);

    // Center "Item Menu" title
    const char* title = "Item Menu";
    size_t titleLength = strlen(title);
    size_t titleX = (screenWidth - titleLength) / 2;
    if (titleX < 0) titleX = 1;
    mvprintw(0, (int) titleX, "%s", title);

    // Header (Symbol, Name, Rarity, Type, Artifact)
    mvprintw(1, 1, "Symbol | Name                      | Rarity     | Type       | Artifact");
    mvprintw(2, 1, "------ | ------------------------- | ---------- | ---------- | --------");

    // Display items (limited to screen height minus header and instructions)
    int maxVisibleItems = screenHeight - 5; // Title, header, dashed line, instructions, buffer
    if (maxVisibleItems > (int)count) maxVisibleItems = count;
    
    // Ensure the index offset is within valid bounds
    if (selectedItemIndex < 0) {
        selectedItemIndex = 0;
    }
    if (selectedItemIndex > count - 1) {
        selectedItemIndex = count - 1;
    }
    reverse(items.begin(), items.end());
    for (int i = 0; i < maxVisibleItems; ++i) {
        
        const Item& item = items[i];

        // Symbol with item color
        short color = item.getColor()[0]; // Single color per item
        attron(COLOR_PAIR(color));
        mvprintw(i + 3, 1, "%-6s", string(1, getSymbolFromType(item.getType())).c_str());
        attroff(COLOR_PAIR(color));
        mvprintw(i + 3, 7, " | ");

        // Name (highlighted in yellow if selected)
        string name = item.getName().substr(0, 25); // Limit to 25 chars
        if (i == selectedItemIndex) {
            attron(COLOR_PAIR(COLOR_YELLOW));
            mvprintw(i + 3, 10, "%-25s", name.c_str());
            attroff(COLOR_PAIR(COLOR_YELLOW));
        } else {
            mvprintw(i + 3, 10, "%-25s", name.c_str());
        }
        mvprintw(i + 3, 35, " | ");

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
        mvprintw(i + 3, 38, "%-10s", rarityText.c_str());
        attroff(COLOR_PAIR(colorPair));
        mvprintw(i + 3, 48, " | ");

        // Type
        string type = item.getType().substr(0, 10); // Limit to 10 chars
        mvprintw(i + 3, 51, "%-10s", type.c_str());
        mvprintw(i + 3, 61, " | ");

        // Artifact (YES or NO)
        string artifactText = item.isArtifact() ? "YES" : "NO";
        mvprintw(i + 3, 64, "%-8s", artifactText.c_str());
    }

    // Instructions
    mvprintw(screenHeight - 1, 1, "Use UP/DOWN to select, ENTER to pick up, ESC to exit.");

    refresh();
}

int rollDice(const string &diceStr) {
    int base = 0, dice = 0, sides = 0;
    char d; // To hold 'd' character

    stringstream ss(diceStr);
    ss >> base >> d >> dice >> d >> sides;  // Parse the format "base+diceDsides"

    int total = base;
    for (int i = 0; i < dice; i++) {
        total += (rand() % sides) + 1;
    }

    return total;
}

pair<int, int> getItemCoordinates(void) {
    int randY, randX;
    while(true) {
        randY = (rand() % (DUNGEON_HEIGHT - 2) + 2) - 1;
        randX = (rand() % (DUNGEON_WIDTH - 2) + 2) - 1;
        if (dungeon.getMap()[randY][randX].ch == '.') { // if it's floor
            break;
        }
    }
    return { randY, randX };
}

vector<short> getColors(const string &colorString) {
    vector<short> colors;
    stringstream ss(colorString);
    string color;

    while (ss >> color) {  // Split by space
        short colorCode = -1;

        if (color == "RED") colorCode = COLOR_RED;
        else if (color == "GREEN") colorCode = COLOR_GREEN;
        else if (color == "BLUE") colorCode = COLOR_BLUE;
        else if (color == "CYAN") colorCode = COLOR_CYAN;
        else if (color == "YELLOW") colorCode = COLOR_YELLOW;
        else if (color == "MAGENTA") colorCode = COLOR_MAGENTA;
        else if (color == "WHITE") colorCode = COLOR_WHITE;
        else if (color == "BLACK") colorCode = COLOR_BLACK;

        // Add valid color to the vector
        if (colorCode != -1) {
            colors.push_back(colorCode);
        }
    }

    return colors;
}

vector<Item> itemFactory() {
    vector<Item> items; // to make dynamic instaces of items
    vector<objectDesc_t> dungeonObjects = readObjects(false); // false because we don't want to print out items
    for (objectDesc_t objectDescription: dungeonObjects) {
        uint16_t hit = rollDice(objectDescription.HIT);
        uint16_t dodge = rollDice(objectDescription.DODGE);
        uint16_t def = rollDice(objectDescription.DEF);
        uint16_t weight = rollDice(objectDescription.WEIGHT);
        uint16_t speed = rollDice(objectDescription.SPEED);
        uint16_t attr = rollDice(objectDescription.ATTR);
        uint16_t val = rollDice(objectDescription.VAL);
        bool art = objectDescription.ART == "TRUE";
        vector<short> colors = getColors(objectDescription.COLOR);
        bool itemInvalid = containsString(invalidItemsAndMonsters, objectDescription.NAME);
        items.push_back(Item(
            -1, // we randomize the coords later
            -1,
            objectDescription.NAME,
            objectDescription.DESC,
            objectDescription.TYPE,
            colors,
            objectDescription.DAM,
            hit,
            dodge,
            def,
            weight,
            speed,
            attr,
            val,
            art,
            stoi(objectDescription.RRTY),
            ROOM_CELL,
            !itemInvalid, // the item is not eligible if the item is invalid
            false // item is not equipped to begin with
        ));
    }
    return items;
}

char getSymbolFromType(string type) {
    if (type == "WEAPON") return '|';
    else if (type == "OFFHAND") return ')';
    else if (type == "RANGED") return '}';
    else if (type == "ARMOR") return '[';
    else if (type == "HELMET") return ']';
    else if (type == "CLOAK") return '(';
    else if (type == "GLOVES") return '{';
    else if (type == "BOOTS") return '\\';
    else if (type == "RING") return '=';
    else if (type == "AMULET") return '"';
    else if (type == "LIGHT") return '_';
    else if (type == "SCROLL") return '~';
    else if (type == "BOOK") return '?';
    else if (type == "FLASK") return '!';
    else if (type == "GOLD") return '$';
    else if (type == "AMMUNITION") return '/';
    else if (type == "FOOD") return ',';
    else if (type == "WAND") return '-';
    else if (type == "CONTAINER") return '%';
    else if (type == "STACK") return '&';
    return '*';
}

// Getters
uint8_t Item::getPosX() const { return posX; }
uint8_t Item::getPosY() const { return posY; }
string Item::getName() const { return NAME; }
string Item::getDescription() const { return DESC; }
string Item::getType() const { return TYPE; }
vector<short> Item::getColor() const { return COLOR; }
string Item::getDamage() const { return DAM; }
u_int16_t Item::getHit() const { return HIT; }
u_int16_t Item::getDodge() const { return DODGE; }
u_int16_t Item::getDefense() const { return DEF; }
u_int16_t Item::getWeight() const { return WEIGHT; }
u_int16_t Item::getSpeed() const { return SPEED; }
u_int16_t Item::getAttribute() const { return ATTR; }
u_int16_t Item::getValue() const { return VAL; }
bool Item::isArtifact() const { return ART; }
u_int8_t Item::getRarity() const { return RRTY; }
cell_t Item::getPreviousCell() const { return previousCell; }
bool Item::isEligible() const { return eligibility; }
bool Item::isEquipped() const { return equipped; }

// Setters
void Item::setPosX(uint8_t x) { posX = x; }
void Item::setPosY(uint8_t y) { posY = y; }
void Item::setName(const string &name) { NAME = name; }
void Item::setDescription(const string &desc) { DESC = desc; }
void Item::setType(const string &type) { TYPE = type; }
void Item::setColor(const vector<short> &color) { COLOR = color; }
void Item::setDamage(const string &damage) { DAM = damage; }
void Item::setHit(u_int16_t hit) { HIT = hit; }
void Item::setDodge(u_int16_t dodge) { DODGE = dodge; }
void Item::setDefense(u_int16_t defense) { DEF = defense; }
void Item::setWeight(u_int16_t weight) { WEIGHT = weight; }
void Item::setSpeed(u_int16_t speed) { SPEED = speed; }
void Item::setAttribute(u_int16_t attribute) { ATTR = attribute; }
void Item::setValue(u_int16_t value) { VAL = value; }
void Item::setArtifact(bool artifact) { ART = artifact; }
void Item::setRarity(u_int8_t rarity) { RRTY = rarity; }
void Item::setPreviousCell(cell_t cell) { previousCell =  cell; }
void Item::setElgibile(bool isEligible) { eligibility = isEligible; }
void Item::setEquipped(bool isEquipped) { equipped = isEquipped; }

