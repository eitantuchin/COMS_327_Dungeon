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
    vector<Item> inventory;
    string NAME;
    string DESC;
    vector<short> COLOR;
    string DAM;
    char SYMB;
    vector<string> ABIL;
    u_int16_t HP;
    u_int8_t RRTY;

public:
    Monster() // default
    : Character(0, 0, cell_t(), 0, cell_t()), lastSeenPCX(-1), lastSeenPCY(-1), alive(false), inventory(), NAME(""), DESC(""), COLOR(), DAM(""), SYMB(' '), ABIL(), HP(0), RRTY(0){}
    // Constructor
    Monster(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, int lastSeenX, int lastSeenY, bool isAlive, vector<Item> inventory, string NAME, string DESC, vector<short> COLOR, string DAM, char SYMB, vector<string> ABIL, u_int16_t HP, u_int8_t RRTY);
       

    // Getters
    int getLastSeenPCX() const;
    int getLastSeenPCY() const;
    bool isAlive() const;
    vector<Item>& getInventory();
    string getName() const;
    string getDescription() const;
    vector<short> getColor() const;
    string getDamage() const;
    char getSymbol() const;
    vector<string> getAbilities() const;
    u_int16_t getHealth() const;
    u_int8_t getRarity() const;

    // Setters
    void setLastSeenPCX(int x);
    void setLastSeenPCY(int y);
    void setAlive(bool isAlive);
    void setName(const string& name);
    void setDescription(const string& desc);
    void setColor(const vector<short>& color);
    void setDamage(const string& dam);
    void setSymbol(char symb);
    void setAbilities(const vector<string>& abil);
    void setHealth(u_int16_t hp);
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
