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
            : Character(0, 0, cell_t(), 0, cell_t()), monsterBits(0), lastSeenPCX(-1), lastSeenPCY(-1), alive(false) {}
    // Constructor
    Monster(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, uint8_t bits, int lastSeenX, int lastSeenY, bool isAlive);

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

string getMonsterPositionString(int monsterIndex);
void displayMonsterList(void);
bool checkMonsterPlacementToPC(int randX, int randY);
void updateMonsterPosition(int index, int oldX, int oldY, int newX, int newY, Monster *m, bool isTunneling);
void moveMonster(int index);
bool hasLineOfSight(int x1, int y1, int x2, int y2);
