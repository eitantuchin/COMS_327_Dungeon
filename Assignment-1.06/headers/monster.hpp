#pragma once

#include "character.hpp"
#include "dungeon_game.hpp"

using namespace std;

class Monster : public Character {
private:
    uint8_t monsterBits; // Bitfield for monster attributes
    int lastSeenPCX;     // Last known X position of the PC
    int lastSeenPCY;     // Last known Y position of the PC
    bool alive;          // Whether the monster is alive
    

public:
    
    Monster()
            : Character(0, 0, ' ', 0, cell_t()), monsterBits(0), lastSeenPCX(-1), lastSeenPCY(-1), alive(false) {}
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
   
    // Additional methods specific to Monster can be added here
};



