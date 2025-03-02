#include "../headers/monster.hpp"
#include "../headers/dungeon.hpp"
#include "../headers/dungeon_game.hpp"
#include "../headers/parse.hpp"
#include "../headers/item.hpp"

#include <stdio.h>

using namespace std;

Monster::Monster(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, int lastSeenX, int lastSeenY, bool isAlive, vector<Item> inventory, string NAME, string DESC, vector<short> COLOR, string DAM, char SYMB, vector<string> ABIL, u_int16_t HP, u_int8_t RRTY)
: Character(x, y, previousCell, speed, cell), lastSeenPCX(lastSeenX), lastSeenPCY(lastSeenY), alive(isAlive), inventory(inventory), NAME(NAME), DESC(DESC), COLOR(COLOR), DAM(DAM), SYMB(SYMB), ABIL(ABIL), HP(HP), RRTY(RRTY) {}

pair<int, int> getMonsterCoordinates(void) {
    int randY, randX;
    while(true) {
        randY = (rand() % (DUNGEON_HEIGHT - 2) + 2) - 1;
        randX = (rand() % (DUNGEON_WIDTH - 2) + 2) - 1;
        bool pcFarAwayEnough = checkMonsterPlacementToPC(randX, randY);
        if ((dungeon.getMap()[randY][randX].ch == '.' || dungeon.getMap()[randY][randX].ch == '#') && pcFarAwayEnough) { // if it's floor
            break;
        }
    }
    return { randY, randX };
}

vector<string> getAbils(const string &abilitiesString) {
    vector<string> abilities;
    stringstream ss(abilitiesString);
    string ability;

    while (ss >> ability) {  // Split by space
        abilities.push_back(ability);
    }
    return abilities;
}

vector<Monster> monsterFactory(void) {
    vector<Monster> monsterArray; // to make dynamic instaces of items
    vector<monsterDesc_t> dungeonMonsters = readMonsters(false); // false because we don't want to print out monsters
    for (monsterDesc_t monsterDescription: dungeonMonsters) {
        pair<int, int> coordinates = getMonsterCoordinates();
        cell_t PREVIOUS_CELL;
        if (dungeon.getMap()[coordinates.first][coordinates.second].ch == '#') {
            PREVIOUS_CELL = CORRIDOR_CELL;
        }
        else {
            PREVIOUS_CELL = ROOM_CELL;
        }
        uint16_t speed = rollDice(monsterDescription.SPEED);
        cell_t MONSTER_CELL = { monsterDescription.SYMB[0], -1 };
        vector<Item> inventory;
        vector<short> colors = getColors(monsterDescription.COLOR);
        uint16_t health = rollDice(monsterDescription.HP);
        vector<string> abilities = getAbils(monsterDescription.ABIL);
        bool monsterInvalid = containsString(invalidItemsAndMonsters, monsterDescription.NAME);
        if (!monsterInvalid) {
            monsterArray.push_back(Monster(
               coordinates.second,
               coordinates.first,
               PREVIOUS_CELL,
               speed,
               MONSTER_CELL,
               -1,
               -1,
               !monsterInvalid,
               inventory,
               monsterDescription.NAME,
               monsterDescription.DESC,
               colors,
               monsterDescription.DAM,
               monsterDescription.SYMB[0],
               abilities,
               health,
               stoi(monsterDescription.RRTY)
            ));
        }
    }
    return monsterArray;
}


void displayMonsterList(void) {
    int screenHeight, screenWidth;
    getmaxyx(stdscr, screenHeight, screenWidth); // Get the size of the terminal window
    
    // Center "Monster List" title
    const char* title = "Monster List";
    size_t titleLength = strlen(title);
    size_t titleX = (screenWidth - titleLength) / 2; // Center horizontally
    if (titleX < 0) titleX = 1; // Ensure it doesn't go off-screen on very narrow terminals
    mvprintw(0, (int) titleX, "%s", title);
    
    mvprintw(1, 1, "Symbol  | Name                      | Rarity     | Position");
    
    // Dashed line under the header
    mvprintw(2, 1, "------- | ------------------------- | ---------- | ---------------------------");
    
    int maxVisibleMonsters = screenHeight - 5; // Subtract 5 for header, dashed line, and instructions
    
    // Ensure the scroll offset is within valid bounds
    if (monsterListScrollOffset < 0) {
        monsterListScrollOffset = 0;
    }
    int maxScrollOffset = dungeon.getNumMonsters() - maxVisibleMonsters;
    if (maxScrollOffset < 0) {
        maxScrollOffset = 0;
    }
    if (monsterListScrollOffset > maxScrollOffset) {
        monsterListScrollOffset = maxScrollOffset;
    }
    // Static variables for color cycling
    static struct timespec lastUpdate = {0, 0};
    static int colorIndex = 0;
    
    // Get current time
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (lastUpdate.tv_sec == 0) { // First run
        lastUpdate = now;
    }

    // Check if 0.5 seconds have passed
    double elapsed = (now.tv_sec - lastUpdate.tv_sec) +
                     (now.tv_nsec - lastUpdate.tv_nsec) / 1e9;
    if (elapsed >= 0.5) { // 0.5-second delay
        colorIndex++; // Move to next color
        lastUpdate = now;
    }
    
    for (int i = 0; i < maxVisibleMonsters && i + monsterListScrollOffset < dungeon.getNumMonsters(); ++i) {
        int monsterIndex = i + monsterListScrollOffset;
        const Monster& monster = dungeon.getMonsters()[monsterIndex];
        vector<short> colors = monster.getColor();
        short color = colors[colorIndex % colors.size()];
        attron(COLOR_PAIR(color));
        mvprintw(i + 3, 1, "%-7s", string(1, monster.getSymbol()).c_str());
        attroff(COLOR_PAIR(color));
        mvprintw(i + 3, 8, " | ");
        
        // Name (shifted right, still 25 chars)
        string name = monster.getName().substr(0, 25);
        mvprintw(i + 3, 11, "%-25s", name.c_str());
        mvprintw(i + 3, 36, " | ");
        
        // Rarity with color coding (shifted right, 10 chars)
        uint8_t rarity = monster.getRarity();
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
        mvprintw(i + 3, 39, "%-10s", rarityText.c_str());
        attroff(COLOR_PAIR(colorPair));
        mvprintw(i + 3, 49, " | ");
        
        // Position (shifted right, 27 chars to fit 80-column terminal)
        string positionString = monster.isAlive() ? getMonsterPositionString(monsterIndex) : "Killed";
        positionString = positionString.substr(0, 27);
        mvprintw(i + 3, 52, "%-27s", positionString.c_str());
    }
    
    // Instructions at the bottom, shifted right
    mvprintw(screenHeight - 1, 1, "Press ESC to return to the game. Use UP/DOWN arrows to scroll.");
    
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
    
    // Get monster abilities
    vector<string> abilities = m->getAbilities();
    bool isErratic = containsString(abilities, string("ERRATIC"));
    bool isTunneling = containsString(abilities, string("TUNNEL"));
    bool isTelepathic = containsString(abilities, string("TELE"));
    bool isIntelligent = containsString(abilities, string("SMART"));
    bool canPass = containsString(abilities, string("PASS"));
    
    // Determine target position
    int targetX = dungeon.getPC().getPosX();
    int targetY = dungeon.getPC().getPosY();
    
    // Handle pass-wall and tunneling monsters
    int (*dist)[DUNGEON_WIDTH];
    if (isTunneling) {
        dist = dungeon.getTunnelingMap();
    }
    else if (canPass) {
        dist = dungeon.getNonTunnelingPassMap();
    }
    else {
        dist = dungeon.getNonTunnelingMap();
    }

    // Handle erratic behavior
    bool erraticRandom = isErratic && (rand() % 2 == 0);
    if (erraticRandom) {
        newX = oldX + ((rand() % 3) - 1);
        newY = oldY + ((rand() % 3) - 1);
    }
    // Not telepatchic nor intelligent
    else if (!isTelepathic && !isIntelligent) {
        // Moves towards PC only if LOS exists, in a straight line
        if (!hasLineOfSight(oldX, oldY, targetX, targetY))
            return;
        newX = oldX + sign(targetX - oldX);
        newY = oldY + sign(targetY - oldY);
    }
    // Not telepathic but is intelligent
    else if (!isTelepathic && isIntelligent) {
        // Moves toward PC on shortest path if LOS exists, remembers last seen position
        if (hasLineOfSight(oldX, oldY, targetX, targetY)) {
            m->setLastSeenPCX(targetX);
            m->setLastSeenPCY(targetY);
        }
        else {
            return;
        }
        int best = INT_MAX;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = oldX + dx;
                int ny = oldY + dy;
                if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT) continue;
                if (!isTunneling && dungeon.getMap()[ny][nx].hardness > 0) {
                    continue;
                }
                if (dist[ny][nx] < best) {
                    best = dist[ny][nx];
                    newX = nx;
                    newY = ny;
                }
            }
        }
    }
    // Is telepathic but not intelligent
    else if (isTelepathic && !isIntelligent) {
        // Passes through rock, 50% random, otherwise always knows PC position, moves in straight line
        newX = oldX + sign(targetX - oldX);
        newY = oldY + sign(targetY - oldY);
        m->setLastSeenPCX(targetX);
        m->setLastSeenPCY(targetY);
    }
    // Is telepathic and is intelligent
    else if (isTelepathic && isIntelligent) {
        // Always knows PC position, moves on shortest path, remembers last seen
        int best = INT_MAX;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = oldX + dx;
                int ny = oldY + dy;
                if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT) continue;
                if (!isTunneling && dungeon.getMap()[ny][nx].hardness > 0) {
                    continue;
                }
                if (dist[ny][nx] < best) {
                    best = dist[ny][nx];
                    newX = nx;
                    newY = ny;
                }
            }
        }
        m->setLastSeenPCX(targetX);
        m->setLastSeenPCY(targetY);
    }
    
    // Update position, tunneling only applies to tunneling monsters
    updateMonsterPosition(index, oldX, oldY, newX, newY, m, isTunneling, canPass);
}


void updateMonsterPosition(int index, int oldX, int oldY, int newX, int newY, Monster *m, bool isTunneling, bool canPass) {
    // performing the monster's move
    //updateMapForItemCells();

    bool cellSet = false;
    if (newX >= 1 && newX < DUNGEON_WIDTH - 1 && newY >= 1 && newY < DUNGEON_HEIGHT - 1) {
        vector<Item> items = dungeon.getItemMap()[newY][newX];
        if (containsString(m->getAbilities(), string("DESTROY")) || containsString(m->getAbilities(), string("PICKUP"))) { // ability to destroy items
            // destroy the topmost item in the stack
            if (!items.empty()) { // Check if the vector is not empty
                Item i = items.back();
                items.pop_back(); // Remove the last item
                if (containsString(m->getAbilities(), string("PICKUP"))) {
                    m->getInventory().push_back(i); // add the item to the monster's inventory
                }
                if (items.empty()) {
                    dungeon.getMap()[oldY][oldX] = m->getPreviousCell();
                    m->setPreviousCell(i.getPreviousCell());
                    cellSet = true;
                }
            }
        }
        dungeon.getItemMap()[newY][newX] = items; // update item map
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
        if (!collision && (dungeon.getMap()[newY][newX].hardness < IMMUTABLE_ROCK_CELL.hardness && canPass)) {
            dungeon.getMap()[oldY][oldX] = m->getPreviousCell(); // Restore the old cell
            if (!cellSet) {
                dungeon.getMap()[oldY][oldX] = m->getPreviousCell();
                m->setPreviousCell(dungeon.getMap()[newY][newX]);
            }
            m->setPosX(newX);
            m->setPosY(newY);
            dungeon.getMap()[newY][newX] = m->getCell();
        }
        else if (!collision && (dungeon.getMap()[newY][newX].hardness <= 0)) { // no monster fighting and not a rock cell
            // Problem here
            if (!cellSet) {
                dungeon.getMap()[oldY][oldX] = m->getPreviousCell();
                m->setPreviousCell(dungeon.getMap()[newY][newX]);
            }// Restore the old cell
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
                if (!cellSet) {
                    dungeon.getMap()[oldY][oldX] = m->getPreviousCell();
                    m->setPreviousCell(dungeon.getMap()[newY][newX]);
                }
                m->setPosX(newX);
                m->setPosY(newY);
                dungeon.getMap()[newY][newX] = m->getCell();
            }
            
        }
        else if (collision) {
            if (defenderIndex != -1) { // kill the defender
                dungeon.getMonsters()[defenderIndex].setAlive(false);
                // if the defending monster is unique then it can't be spawned anymore
                if (containsString(dungeon.getMonsters()[defenderIndex].getAbilities(), string("UNIQ"))) {
                    invalidItemsAndMonsters.push_back(dungeon.getMonsters()[defenderIndex].getName());
                }
                dungeon.getMap()[newY][newX] = m->getCell();
                dungeon.getMap()[oldY][oldX] = m->getPreviousCell();
                m->setPosX(newX);
                m->setPosY(newY);
                m->setPreviousCell(dungeon.getMonsters()[defenderIndex].getPreviousCell());
                
            }
        }
    }
    //updateMapForItemCells();
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


int Monster::getLastSeenPCX() const {
    return lastSeenPCX;
}

int Monster::getLastSeenPCY() const {
    return lastSeenPCY;
}

bool Monster::isAlive() const {
    return alive;
}

// cannot use const because when using the reference value it can be modified
vector<Item>& Monster::getInventory() {
    return inventory;
}

string Monster::getName() const {
    return NAME;
}

string Monster::getDescription() const {
    return DESC;
}

vector<short> Monster::getColor() const {
    return COLOR;
}

string Monster::getDamage() const {
    return DAM;
}

char Monster::getSymbol() const {
    return SYMB;
}

vector<string> Monster::getAbilities() const {
    return ABIL;
}

u_int16_t Monster::getHealth() const {
    return HP;
}

u_int8_t Monster::getRarity() const {
    return RRTY;
}

void Monster::setLastSeenPCX(int x) {
    lastSeenPCX = x;
}

void Monster::setLastSeenPCY(int y) {
    lastSeenPCY = y;
}

void Monster::setAlive(bool isAlive) {
    alive = isAlive;
}

void Monster::setName(const string& name) {
    NAME = name;
}

void Monster::setDescription(const string& desc) {
    DESC = desc;
}

void Monster::setColor(const vector<short>& color) {
    COLOR = color;
}

void Monster::setDamage(const string& dam) {
    DAM = dam;
}

void Monster::setSymbol(char symb) {
    SYMB = symb;
}

void Monster::setAbilities(const vector<string>& abil) {
    ABIL = abil;
}

void Monster::setHealth(u_int16_t hp) {
    HP = hp;
}

void Monster::setRarity(u_int8_t rrty) {
    RRTY = rrty;
}



