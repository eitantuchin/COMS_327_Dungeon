#include "../headers/character.hpp"

using namespace std;

// Constructor implementation
Character::Character(uint8_t x, uint8_t y, char prevChar, uint8_t speed, cell_t cell)
    : posX(x), posY(y), previousCharacter(prevChar), speed(speed), cell(cell) {}

// Getter implementations
uint8_t Character::getPosX() const {
    return posX;
}

uint8_t Character::getPosY() const {
    return posY;
}

char Character::getPreviousCharacter() const {
    return previousCharacter;
}

uint8_t Character::getSpeed() const {
    return speed;
}

cell_t Character::getCell() const {
    return cell;
}

// Setter implementations
void Character::setPosX(uint8_t x) {
    posX = x;
}

void Character::setPosY(uint8_t y) {
    posY = y;
}

void Character::setPreviousCharacter(char prevChar) {
    previousCharacter = prevChar;
}

void Character::setSpeed(uint8_t speed) {
    this->speed = speed;
}

void Character::setCell(cell_t cell) {
    this->cell = cell;
}
