#pragma once
#include <string>
#include <vector>
#include "dungeon_game.hpp"

using namespace std;

class Item {
protected:
    // Instance variables
    uint8_t posX;
    uint8_t posY;
    string NAME;
    string DESC;
    string TYPE;
    vector<short> COLOR;
    string DAM;
    u_int16_t HIT;
    u_int16_t DODGE;
    u_int16_t DEF;
    u_int16_t WEIGHT;
    u_int16_t SPEED;
    u_int16_t ATTR;
    u_int16_t VAL;
    bool ART;
    u_int8_t RRTY;
    cell_t previousCell;
    bool eligibility;

public:
    // Constructor
    Item(uint8_t posX, uint8_t posY, string NAME, string DESC, string TYPE, vector<short> COLOR, string DAM, u_int16_t HIT, u_int16_t DODGE, u_int16_t DEF, u_int16_t WEIGHT, u_int16_t SPEED, u_int16_t ATTR, u_int16_t VAL, bool ART, u_int8_t RRTY, cell_t previousCell, bool eligibility);

    // Getters
    uint8_t getPosX() const;
    uint8_t getPosY() const;
    string getName() const;
    string getDescription() const;
    string getType() const;
    vector<short> getColor() const;
    string getDamage() const;
    u_int16_t getHit() const;
    u_int16_t getDodge() const;
    u_int16_t getDefense() const;
    u_int16_t getWeight() const;
    u_int16_t getSpeed() const;
    u_int16_t getAttribute() const;
    u_int16_t getValue() const;
    bool isArtifact() const;
    u_int8_t getRarity() const;
    cell_t getPreviousCell() const;
    bool isEligible() const;

    // Setters
    void setPosX(uint8_t x);
    void setPosY(uint8_t y);
    void setName(const string &name);
    void setDescription(const string &desc);
    void setType(const string &type);
    void setColor(const vector<short> &color);
    void setDamage(const string &damage);
    void setHit(u_int16_t hit);
    void setDodge(u_int16_t dodge);
    void setDefense(u_int16_t defense);
    void setWeight(u_int16_t weight);
    void setSpeed(u_int16_t speed);
    void setAttribute(u_int16_t attribute);
    void setValue(u_int16_t value);
    void setArtifact(bool artifact);
    void setRarity(u_int8_t rarity);
    void setPreviousCell(cell_t cell);
    void setElgibile(bool eligibility);

    // Virtual destructor
    virtual ~Item() = default;
};


int rollDice(const string &diceStr);
vector<Item> itemFactory();
char getSymbolFromType(string type);
vector<short> getColors(const string &colorString);
void updateMapForItemCells(void);
void displayItemMenu(void);
pair<int, int> getItemCoordinates(void);
