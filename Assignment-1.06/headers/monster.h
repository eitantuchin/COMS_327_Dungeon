#ifndef MONSTER_H
#define MONSTER_H

#include "character.h"
#include "dungeon_game.h"

class Monster : public Character {
private:
    uint8_t monsterBits; // Bitfield for monster attributes
    int lastSeenPCX;     // Last known X position of the PC
    int lastSeenPCY;     // Last known Y position of the PC
    bool alive;          // Whether the monster is alive
    

public:
    // Constructor
    Monster(uint8_t x, uint8_t y, char prevChar, uint8_t speed, cell_t cell, uint8_t bits, int lastSeenX, int lastSeenY, bool isAlive);

    // Getters
    uint8_t getMonsterBits() const;
    int getLastSeenPCX() const;
    int getLastSeenPCY() const;
    bool isAlive() const;

    // Setters
    void setMonsterBits(uint8_t bits);
    void setLastSeenPCX(int x);
    void setLastSeenPCY(int y);
    void setAlive(bool isAlive);
    uint8_t getPosX() const;
    uint8_t getPosY() const;
    char getPreviousCharacter() const;
    uint8_t getSpeed() const;
    cell_t getCell() const;

    // Setters
    void setPosX(uint8_t x);
    void setPosY(uint8_t y);
    void setPreviousCharacter(char prevChar);
    void setSpeed(uint8_t speed);
    void setCell(cell_t cell);

    // Additional methods specific to Monster can be added here
};

#endif // MONSTER_H
