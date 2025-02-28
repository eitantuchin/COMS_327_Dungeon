#ifndef PC_HPP
#define PC_HPP

#include "character.hpp"    // contains inherited methods
#include "dungeon_game.hpp" // contains cell_t

using namespace std;


class PC : public Character {
private:
    direction_t currentDirection; // Current direction the PC is facing
    bool fogMap[DUNGEON_HEIGHT][DUNGEON_WIDTH];

public:
    // Constructor
    PC(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, direction_t direction);

    // Getters
    direction_t getCurrentDirection() const;
    bool (&getFogMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];

    // Setters
    void setCurrentDirection(direction_t direction);
    
    
    // Additional methods specific to PC can be added here
};

#endif