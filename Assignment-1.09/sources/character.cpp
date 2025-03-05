#include "../headers/character.hpp"

using namespace std;

// Constructor implementation
Character::Character(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, u_int16_t HP, string DAM, vector<Item> inventory)
: posX(x), posY(y), previousCell(previousCell), speed(speed), cell(cell), HP(HP), DAM(DAM), inventory(inventory) {}

// Getter implementations
uint8_t Character::getPosX() const {
    return posX;
}

uint8_t Character::getPosY() const {
    return posY;
}

cell_t Character::getPreviousCell() const {
    return previousCell;
}

uint8_t Character::getSpeed() const {
    return speed;
}

cell_t Character::getCell() const {
    return cell;
}

u_int16_t Character::getHealth() const {
    return HP;
}

string Character::getDamage() const {
    return DAM;
}

// cannot use const because when using the reference value it can be modified
vector<Item>& Character::getInventory() {
    return inventory;
}

// Setter implementations
void Character::setPosX(uint8_t x) {
    posX = x;
}

void Character::setPosY(uint8_t y) {
    posY = y;
}

void Character::setPreviousCell(cell_t prevCell) {
    previousCell = prevCell;
}

void Character::setSpeed(uint8_t speed) {
    this->speed = speed;
}

void Character::setCell(cell_t cell) {
    this->cell = cell;
}

void Character::setHealth(u_int16_t hp) {
    HP = hp;
}

void Character::setDamage(const string& dam) {
    DAM = dam;
}
