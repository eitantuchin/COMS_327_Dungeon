#pragma once

#include "character.hpp"
#include "dungeon_game.hpp"
#include "item.hpp"
#include <vector>
#include <string>

using namespace std;

class Monster : public Character {
private:
    int lastSeenPCX;     // Last known X position of the PC
    int lastSeenPCY;     // Last known Y position of the PC
    bool alive;          // Whether the monster is alive
    string NAME;
    string DESC;
    vector<short> COLOR;
    char SYMB;
    vector<string> ABIL;
    u_int8_t RRTY;

public:
    Monster() // default
    : Character(0, 0, cell_t(), 0, cell_t(), 0, "", {}, false), lastSeenPCX(-1), lastSeenPCY(-1), alive(false), NAME(""), DESC(""), COLOR(), SYMB(' '), ABIL(), RRTY(0){}
    // Constructor
    Monster(uint8_t x, uint8_t y, cell_t previousCell, int16_t speed, cell_t cell, int32_t HP, string DAM, vector<Item> inventory, bool isInWater, int lastSeenX, int lastSeenY, bool isAlive, string NAME, string DESC, vector<short> COLOR, char SYMB, vector<string> ABIL, u_int8_t RRTY);
    

    // Getters
    int getLastSeenPCX() const;
    int getLastSeenPCY() const;
    bool isAlive() const;
    string getName() const;
    string getDescription() const;
    vector<short> getColor() const;
    char getSymbol() const;
    vector<string> getAbilities() const;
    u_int8_t getRarity() const;
    // Setters
    void setLastSeenPCX(int x);
    void setLastSeenPCY(int y);
    void setAlive(bool isAlive);
    void setName(const string& name);
    void setDescription(const string& desc);
    void setColor(const vector<short>& color);
    void setSymbol(char symb);
    void setAbilities(const vector<string>& abil);
    void setRarity(u_int8_t rrty);

    // Additional methods specific to Monster can be added here
};

string getMonsterPositionString(int monsterIndex);
void displayMonsterList(void);
bool checkMonsterPlacementToPC(int randX, int randY);
void updateMonsterPosition(int index, int oldX, int oldY, int newX, int newY, Monster *m, bool isTunneling, bool canPass);
void moveMonster(int index);
bool hasLineOfSight(int x1, int y1, int x2, int y2);
vector<Monster> monsterFactory(void);
int getMonsterAtPointer(void);
void displayMonsterDetails(int monsterIndex);
pair<int, int> getMonsterCoordinates(void);
