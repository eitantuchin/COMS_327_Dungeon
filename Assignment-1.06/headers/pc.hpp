#ifndef PC_HPP
#define PC_HPP

#include "character.hpp"    // contains inherited methods
#include "dungeon_game.hpp" // contains cell_t

using namespace std;


class PC : public Character {
private:
    direction_t currentDirection; // Current direction the PC is facing
    

public:
    // Constructor
    PC(uint8_t x, uint8_t y, char prevChar, uint8_t speed, cell_t cell, direction_t direction);

    // Getter for currentDirection
    direction_t getCurrentDirection() const;

    // Setter for currentDirection
    void setCurrentDirection(direction_t direction);
    

    // Additional methods specific to PC can be added here
};

#endif // PC_H
