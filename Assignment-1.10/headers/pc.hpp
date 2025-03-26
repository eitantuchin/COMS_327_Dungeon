#pragma once

#include "character.hpp"    // contains inherited methods
#include "dungeon_game.hpp" // contains cell_t

using namespace std;


class PC : public Character {
private:
    direction_t currentDirection; // Current direction the PC is facing
    bool fogMap[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    vector<Item> equippedItems;
    int damageDealt;
    int damageTaken;
    int distanceTraveled;
    int monstersKilled;
    int floorsVisited;
    int numItemsPickedUp;
    int numHealing;
    string name;
    int numCoins;
    int numSouls;

public:
    // Constructor
    PC(uint8_t x, uint8_t y, cell_t previousCell, int16_t speed, cell_t cell, int32_t HP, string DAM, vector<Item> inventory, bool isInWater, direction_t direction, vector<Item> equippedItems, int damageDealt, int damageTaken, int distanceTraveled, int monstersKilled, int floorsVisited, int numItemsPickedUp, int numHealing, string name, int numCoins, int numSouls);

    // Getters
    direction_t getCurrentDirection() const;
    bool (&getFogMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    vector<Item>& getEquippedItems();
    int getDamageDealt() const;
    int getDamageTaken() const;
    int getDistanceTraveled() const;
    int getMonstersKilled() const;
    int getFloorsVisited() const;
    int getNumItemsPickedUp() const;
    int getNumHealing() const;
    string getName() const;
    int getCoins() const;
    int getSouls() const;
    
    // Setters
    void setCurrentDirection(direction_t direction);
    void setName(string name);
    void setCoins(int coins);
    void setSouls(int souls);
    
    void addDamageDealt(int stat);
    void addDamageTaken(int stat);
    void addDistanceTraveled(int stat);
    void addMonstersKilled(int stat);
    void addFloorsVisited(int stat);
    void addNumItemsPickedUp(int stat);
    void addNumHealing(int stat);
};

void teleportPlayer(bool randomTeleport);
void changeDirection(bool clockwise, bool justChangeText);
void movePlayer(int key);
void pickupItem(void);
void displayInventory(void);
void displayEquipment(void);
void wearItem(char key);
void dropItem(char key);
void expungeItem(char key);
void inspectItem(char key);
void takeOffItem(char key);
void displayStats(void);
void buyItem(void);
void useItem(char key);
