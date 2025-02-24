#include "../headers/monster.hpp"
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
