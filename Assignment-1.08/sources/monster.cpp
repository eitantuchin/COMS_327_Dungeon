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
