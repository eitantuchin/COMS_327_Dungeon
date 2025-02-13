#include "../headers/pc.hpp"
using namespace std;

// Constructor implementation
PC::PC(uint8_t x, uint8_t y, char prevChar, uint8_t speed, cell_t cell, direction_t direction)
    : Character(x, y, prevChar, speed, cell), currentDirection(direction) {}

// Getter for currentDirection
direction_t PC::getCurrentDirection() const {
    return currentDirection;
}

// Setter for currentDirection
void PC::setCurrentDirection(direction_t direction) {
    currentDirection = direction;
}
