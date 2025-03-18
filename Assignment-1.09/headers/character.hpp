#pragma once
#include "dungeon_game.hpp"
#include "item.hpp"

using namespace std;

class Character {
protected:
    uint8_t posX;                // X position in the dungeon
    uint8_t posY;                // Y position in the dungeon
    cell_t previousCell;      // The character that was in the cell before the character moved there
    int16_t speed;               // Speed of the character
    cell_t cell;                 // The cell type associated with the character
    int32_t HP;
    string DAM;
    vector<Item> inventory;

public:
    // Constructor
    Character(uint8_t x, uint8_t y, cell_t previousCell, int16_t speed, cell_t cell,  int32_t HP,  string DAM,  vector<Item> inventory);
    
    // Getters
    uint8_t getPosX() const;
    uint8_t getPosY() const;
    cell_t getPreviousCell() const;
    int16_t getSpeed() const;
    cell_t getCell() const;
    int32_t getHealth() const;
    string getDamage() const;
    vector<Item>& getInventory();


    // Setters
    void setPosX(uint8_t x);
    void setPosY(uint8_t y);
    void setPreviousCell(cell_t previousCell);
    void setSpeed(int16_t speed);
    void setCell(cell_t cell);
    void setHealth(int32_t hp);
    void setDamage(const string& dam);


    // Virtual destructor
    virtual ~Character() = default;
};
