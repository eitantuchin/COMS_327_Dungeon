#ifndef CHARACTER_H
#define CHARACTER_H

#include "dungeon_game.h"

class Character {
protected:
    uint8_t posX;                // X position in the dungeon
    uint8_t posY;                // Y position in the dungeon
    char previousCharacter;      // The character that was in the cell before the character moved there
    uint8_t speed;               // Speed of the character
    cell_t cell;                 // The cell type associated with the character

public:
    // Constructor
    Character(uint8_t x, uint8_t y, char prevChar, uint8_t speed, cell_t cell);

    // Getters
    virtual uint8_t getPosX() const;
    virtual uint8_t getPosY() const;
    virtual char getPreviousCharacter() const;
    virtual uint8_t getSpeed() const;
    virtual cell_t getCell() const;

    // Setters
    virtual void setPosX(uint8_t x);
    virtual void setPosY(uint8_t y);
    virtual void setPreviousCharacter(char prevChar);
    virtual void setSpeed(uint8_t speed);
    virtual void setCell(cell_t cell);

    // Virtual destructor
    virtual ~Character() = default;
};

#endif // CHARACTER_H
