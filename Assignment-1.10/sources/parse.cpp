//
//  parse.cpp
//  Dungeon_Game_327
//
//  Created by Eitan Tuchin on 2/19/25.
//

#include "../headers/parse.hpp"
#include "../headers/dungeon_game.hpp"

#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <fstream>
#include <cstring>
#include <iostream>
#include <regex>
#include <sstream>
#include <set>

using namespace std;

bool isValidColor(const string& color) {
    static const set<string> validColors = {"RED", "GREEN", "BLUE", "CYAN", "YELLOW", "MAGENTA", "WHITE", "BLACK"};
    stringstream ss(color);
    string token;
    while (ss >> token) {
        if (validColors.find(token) == validColors.end()) {
            return false;
        }
    }
    return true;
}

bool isValidAbility(const string& ability) {
    static const set<string> validAbilities = {"UNIQ", "ERRATIC", "DESTROY", "TUNNEL", "BOSS", "TELE", "SMART", "PICKUP", "PASS"};
    stringstream ss(ability);
    string token;
    while (ss >> token) {
        if (validAbilities.find(token) == validAbilities.end()) {
            return false;
        }
    }
    return true;
}

bool isValidDiceFormat(const string& value) {
    return regex_match(value, regex(R"(^\d+\+\d+d\d+$)"));
}

bool isInteger(const string& str) {
    return regex_match(str, regex(R"(^\d+$)"));
}

bool isValidType(const string& type) {
    static const set<string> validTypes = {"WEAPON", "ARMOR", "BOOTS", "HELMET", "OFFHAND", "CLOAK", "RING", "GLOVES", "LIGHT", "AMULET", "RANGED", "SCROLL", "BOOK", "FLASK", "GOLD", "AMMUNITION", "FOOD", "WAND", "CONTAINER", "STACK"};
    stringstream ss(type);
    string token;
    while (ss >> token) {
        if (validTypes.find(token) == validTypes.end()) {
            return false;
        }
    }
    return true;
}

void printMonsterDesciption(monsterDesc_t desc) {
  cout << desc.NAME << '\n';
  cout << desc.DESC;
  cout << desc.SYMB << '\n';
  cout << desc.COLOR << '\n';
  cout << desc.SPEED << '\n';
  cout << desc.ABIL << '\n';
  cout << desc.HP << '\n';
  cout << desc.DAM << '\n';
  cout << desc.RRTY << '\n';
  cout << '\n';
}

vector<monsterDesc_t> readMonsters(bool printOutput) {
    
    string dir = getenv("HOME") ? getenv("HOME") : "";
    if (dir.empty()) {
        cerr << "HOME environment variable not set." << endl;
        return {};
    }
    string path = dir + "/.rlg327/monster_desc.txt";
    ifstream monsterDescriptions(path);
    if (!monsterDescriptions.is_open()) {
        cerr << "Failed to open file: " << path << endl;
        return {};
    }

    string title;
    getline(monsterDescriptions, title);
    if (title != "RLG327 MONSTER DESCRIPTION 1") {
        monsterDescriptions.close();
        return {};
    }

  monsterDesc_t description;
  string curLine;
  bool duplicateFound = false;
    vector<monsterDesc_t> monsters;
    while(getline(monsterDescriptions, curLine)) {
      if(curLine == "BEGIN MONSTER" || curLine == "") {
        continue;
      }
      else if(curLine == "END"){
          if (!duplicateFound && isMonsterDescriptionValid(description) && printOutput) {
              printMonsterDesciption(description);
          }
          else if ((!duplicateFound && isMonsterDescriptionValid(description))) {
              monsters.push_back(description); // add description to array
          }
        
        description = {};
        duplicateFound = false;
      }
      else if(curLine.substr(0, 4) == "NAME") {
        if(description.NAME == ""){
            try {
                description.NAME = curLine.substr(5, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
        
      else if(curLine.substr(0, 4) == "DESC") {
          if (description.DESC == "")  {
              while(getline(monsterDescriptions, curLine)){
                  if(curLine.substr(0, 1) == "."){
                      break;
                  }
                  else{
                      try {
                          description.DESC += curLine.substr(0, 77) + '\n';
                      }
                      catch (const out_of_range& e) {
                          continue;
                      }
                  }
              }
          }
          else {
              duplicateFound = true;
          }
      }
      else if(curLine.substr(0, 5) == "COLOR") {
        if(description.COLOR == ""){
            try {
                description.COLOR = curLine.substr(6, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 5) == "SPEED") {
        if(description.SPEED == ""){
            try {
                description.SPEED = curLine.substr(6, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 4) == "ABIL") {
        if(description.ABIL == ""){
            try {
                description.ABIL = curLine.substr(5, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 2) == "HP") {
        if(description.HP == ""){
            try {
                description.HP = curLine.substr(3, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 3) == "DAM") {
        if(description.DAM == ""){
            try {
                description.DAM = curLine.substr(4, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 4) == "SYMB") {
        if(description.SYMB == ""){
            try {
                description.SYMB = curLine.substr(5, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 4) == "RRTY") {
        if(description.RRTY == ""){
            try {
                description.RRTY = curLine.substr(5, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
    }

    monsterDescriptions.close();
    return monsters;
}

bool isMonsterDescriptionValid(monsterDesc_t description) {
    if (description.NAME.empty() || description.DESC.empty() ||
        description.ABIL.empty() || description.COLOR.empty() ||
        description.DAM.empty()  || description.HP.empty() ||
        description.RRTY.empty() || description.SPEED.empty() ||
        description.SYMB.empty()) {
        return false;
    }

    // Ensure SYMB is only one character
    if (description.SYMB.length() != 1) {
        return false;
    }

    // Validate COLOR values
    if (!isValidColor(description.COLOR)) {
        return false;
    }

    // Validate SPEED, DAM, HP formats
    if (!isValidDiceFormat(description.SPEED) ||
        !isValidDiceFormat(description.DAM) ||
        !isValidDiceFormat(description.HP)) {
        return false;
    }

    // Validate ABILITY values
    if (!isValidAbility(description.ABIL)) {
        return false;
    }

    // Validate RRTY is an integer
    if (!isInteger(description.RRTY)) {
        return false;
    }

    return true;
}


void printObjectDesciption(objectDesc_t description) {
  cout << description.NAME << '\n';
  cout << description.DESC;
  cout << description.TYPE << '\n';
  cout << description.COLOR << '\n';
  cout << "Hit: " << description.HIT << '\n';
  cout << "Damage: " << description.DAM << '\n';
  cout << "Dodge: " << description.DODGE << '\n';
  cout << "Defense: " << description.DEF << '\n';
  cout << "Weight: " << description.WEIGHT << '\n';
  cout << "Speed: " << description.SPEED << '\n';
  cout << "Attribute: " << description.ATTR << '\n';
  cout << "Value: " << description.VAL << '\n';
  cout << "Artifact: " << description.ART << '\n';
  cout << "Rarity: " << description.RRTY << '\n';
  cout << '\n';
}



vector<objectDesc_t> readObjects(bool printOutput) {
    string dir = getenv("HOME") ? getenv("HOME") : "";
    if (dir.empty()) {
        cerr << "HOME environment variable not set." << endl;
        return {};
    }

    // Construct the path using string concatenation
    string path = dir + "/.rlg327/object_desc.txt";
    ifstream objectDescriptions(path);
    if (!objectDescriptions.is_open()) {
        cerr << "Failed to open file: " << path << endl;
        return {};
    }

    // Verify file header
    string title;
    getline(objectDescriptions, title);
    if (title != "RLG327 OBJECT DESCRIPTION 1") {
        objectDescriptions.close();
        return {};
    }
  
  vector<objectDesc_t> objects;
  objectDesc_t description;
  string curLine;
  bool duplicateFound = false;

    while(getline(objectDescriptions, curLine)) {
      if(curLine == "BEGIN MONSTER" || curLine == "") {
        continue;
      }
      else if(curLine == "END") {
          if (!duplicateFound && isObjectDescriptionValid(description) && printOutput)  {
              printObjectDesciption(description);
          }
          else if ((!duplicateFound && isObjectDescriptionValid(description))) {
              objects.push_back(description); // add description to array
          }
        description = {};
        duplicateFound = false;
      }
      else if(curLine.substr(0, 4) == "NAME") {
          if (description.NAME == "") {
              try {
                  description.NAME = curLine.substr(5, curLine.length());
              }
              catch (const out_of_range& e) {
                  continue;
              }
          }
          else {
              duplicateFound = true;
          }
      }
      else if(curLine.substr(0, 4) == "DESC") {
          if (description.DESC == "") {
              while(getline(objectDescriptions, curLine)) {
                  if(curLine.substr(0, 1) == ".") {
                      break;
                  }
                  else{
                      try {
                          description.DESC += curLine.substr(0, 77) + '\n';
                      }
                      catch (const out_of_range& e) {
                          continue;
                      } // can only be 77 chars wide
                  }
              }
          }
          else {
              duplicateFound = true;
          }
      }
      else if(curLine.substr(0, 4) == "TYPE") {
        if(description.TYPE == ""){
            try {
                description.TYPE = curLine.substr(5, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 5) == "COLOR") {
        if(description.COLOR == ""){
            try {
                description.COLOR = curLine.substr(6, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 3) == "HIT") {
        if(description.HIT == ""){
            try {
                description.HIT = curLine.substr(4, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 3) == "DAM") {
        if(description.DAM == ""){
            try {
                description.DAM = curLine.substr(4, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 5) == "DODGE") {
        if(description.DODGE == ""){
            try {
                description.DODGE = curLine.substr(6, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 3) == "DEF") {
        if(description.DEF == ""){
            try {
                description.DEF = curLine.substr(4, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 6) == "WEIGHT") {
        if(description.WEIGHT == ""){
            try {
                description.WEIGHT = curLine.substr(7, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 5) == "SPEED") {
        if(description.SPEED == ""){
            try {
                description.SPEED = curLine.substr(6, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 4) == "ATTR") {
        if(description.ATTR == ""){
            try {
                description.ATTR = curLine.substr(5, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 3) == "VAL") {
        if(description.VAL == ""){
            try {
                description.VAL = curLine.substr(4, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 3) == "ART") {
        if(description.ART == ""){
            try {
                description.ART = curLine.substr(4, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }
      else if(curLine.substr(0, 4) == "RRTY") {
        if(description.RRTY == ""){
            try {
                description.RRTY = curLine.substr(5, curLine.length());
            }
            catch (const out_of_range& e) {
                continue;
            }
        }
        else {
            duplicateFound = true;
        }
      }

    }
    objectDescriptions.close();
    return objects;
}

bool isObjectDescriptionValid(objectDesc_t description) {
   if (description.NAME.empty() || description.DESC.empty() ||
             description.TYPE.empty() || description.COLOR.empty() ||
             description.HIT.empty() || description.DAM.empty() ||
             description.DODGE.empty() || description.DEF.empty() ||
             description.WEIGHT.empty() || description.SPEED.empty() ||
             description.ATTR.empty() || description.VAL.empty() ||
       description.ART.empty() || description.RRTY.empty()) {
       return false;
   }
    

    if (!isValidDiceFormat(description.SPEED) ||
        !isValidDiceFormat(description.DAM) ||
        !isValidDiceFormat(description.WEIGHT) ||
        !isValidDiceFormat(description.HIT) ||
        !isValidDiceFormat(description.ATTR) ||
        !isValidDiceFormat(description.VAL) ||
        !isValidDiceFormat(description.DODGE) ||
        !isValidDiceFormat(description.DEF)) {
        return false;
    }

    if (!isValidType(description.TYPE)) {
        return false;
    }
    
    if (!isValidColor(description.COLOR)) {
        return false;
    }

    // Validate RRTY is an integer
    if (!isInteger(description.RRTY)) {
        return false;
    }
    
    if (!(description.ART == "TRUE" || description.ART == "FALSE")) {
        return false;
    }
    
    return true;
}
