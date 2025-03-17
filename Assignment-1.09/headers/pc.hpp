#pragma once

#include "character.hpp"    // contains inherited methods
#include "dungeon_game.hpp" // contains cell_t

using namespace std;


class PC : public Character {
private:
    direction_t currentDirection; // Current direction the PC is facing
    bool fogMap[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    vector<Item> equippedItems;

public:
    // Constructor
    PC(uint8_t x, uint8_t y, cell_t previousCell, uint8_t speed, cell_t cell, uint16_t HP, string DAM, vector<Item> inventory, direction_t direction, vector<Item> equippedItems);

    // Getters
    direction_t getCurrentDirection() const;
    bool (&getFogMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    vector<Item>& getEquippedItems();

    // Setters
    void setCurrentDirection(direction_t direction);
    
    
    // Additional methods specific to PC can be added here
};

void teleportPlayer(bool randomTeleport);
void changeDirection(bool clockwise, bool justChangeText);
void attack(int distance);
void movePlayer(int key);
void pickupItem(void);
void displayInventory(void);
void displayEquipment(void);
void wearItem(char key);
void dropItem(char key);
void expungeItem(char key);
void inspectItem(char key);
