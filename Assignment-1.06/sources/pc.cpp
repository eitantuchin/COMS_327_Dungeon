#include "../headers/pc.hpp"
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
