//
//  parse.hpp
//  Dungeon_Game_327
//
//  Created by Eitan Tuchin on 2/19/25.
//

#pragma once

#include "dungeon.hpp"

typedef struct objectDesc {
  string NAME;
  string DESC;
  string TYPE;
  string COLOR;
  string HIT;
  string DAM;
  string DODGE;
  string DEF;
  string WEIGHT;
  string SPEED;
  string ATTR;
  string VAL;
  string ART;
  string RRTY;
} objectDesc_t;

typedef struct monsterDesc {
  string NAME;
  string SYMB;
  string COLOR;
  string DESC;
  string SPEED;
  string DAM;
  string HP;
  string ABIL;
  string RRTY;
} monsterDesc_t;


bool isValidColor(const string& color);
bool isValidAbility(const string& ability);
bool isValidDiceFormat(const string& value);
bool isInteger(const string& str);
bool isValidType(const string& type);
void printMonsterDesciption(monsterDesc_t *description);
void readMonsters(bool printOutput);
bool isMonsterDescriptionValid(monsterDesc_t description);
void printObjectDesciption(objectDesc_t *description);
vector<objectDesc_t> readObjects(bool printOutput);
bool isObjectDescriptionValid(objectDesc_t description);


