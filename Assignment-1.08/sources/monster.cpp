#include "../headers/monster.hpp"
#include "../headers/dungeon.hpp"
#include "../headers/dungeon_game.hpp"

using namespace std;

// Constructor implementation
Monster::Monster(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, uint8_t bits, int lastSeenX, int lastSeenY, bool isAlive)
: Character(x, y, previousCell, speed, cell), monsterBits(bits), lastSeenPCX(lastSeenX), lastSeenPCY(lastSeenY), alive(isAlive) {}

// Getter for monsterBits
uint8_t Monster::getMonsterBits() const {
    return monsterBits;
}

// Getter for lastSeenPCX
int Monster::getLastSeenPCX() const {
    return lastSeenPCX;
}

// Getter for lastSeenPCY
int Monster::getLastSeenPCY() const {
    return lastSeenPCY;
}

// Getter for alive
bool Monster::isAlive() const {
    return alive;
}

// Setter for monsterBits
void Monster::setMonsterBits(uint8_t bits) {
    monsterBits = bits;
}

// Setter for lastSeenPCX
void Monster::setLastSeenPCX(int x) {
    lastSeenPCX = x;
}

// Setter for lastSeenPCY
void Monster::setLastSeenPCY(int y) {
    lastSeenPCY = y;
}

// Setter for alive
void Monster::setAlive(bool isAlive) {
    alive = isAlive;
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
        if (!collision && (dungeon.getMap()[newY][newX].hardness == 0 || dungeon.getMap()[newY][newX].hardness <= -2)) { // no monster fighting and not a rock cell
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
