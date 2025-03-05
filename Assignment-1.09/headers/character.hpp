#pragma once
#include "dungeon_game.hpp"
#include "item.hpp"

using namespace std;

class Character {
protected:
    uint8_t posX;                // X position in the dungeon
    uint8_t posY;                // Y position in the dungeon
    cell_t previousCell;      // The character that was in the cell before the character moved there
    uint8_t speed;               // Speed of the character
    cell_t cell;                 // The cell type associated with the character
    u_int16_t HP;
    string DAM;
    vector<Item> inventory;

public:
    // Constructor
    Character(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell,  u_int16_t HP,  string DAM,  vector<Item> inventory);
    
    // Getters
    uint8_t getPosX() const;
    uint8_t getPosY() const;
    cell_t getPreviousCell() const;
    uint8_t getSpeed() const;
    cell_t getCell() const;
    u_int16_t getHealth() const;
    string getDamage() const;
    vector<Item>& getInventory();


    // Setters
    void setPosX(uint8_t x);
    void setPosY(uint8_t y);
    void setPreviousCell(cell_t previousCell);
    void setSpeed(uint8_t speed);
    void setCell(cell_t cell);
    void setHealth(u_int16_t hp);
    void setDamage(const string& dam);


    // Virtual destructor
    virtual ~Character() = default;
};
