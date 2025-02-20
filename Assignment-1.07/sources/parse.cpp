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

using namespace std;

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

void readMonsters(void) {

  char *dir = getenv("HOME");
  char *path = strcat(dir, "/.rlg327/");

  ifstream monsterDescriptions;
  monsterDescriptions.open(strcat(path, "/monster_desc.txt"));
  string title;
  getline(monsterDescriptions, title);

  if(title != "RLG327 MONSTER DESCRIPTION 1") {
    return;
  }

  monsterDesc_t description;
  string curLine;
  bool duplicateFound = false;
    while(getline(monsterDescriptions, curLine)) {
      if(curLine == "BEGIN MONSTER" || curLine == "") {
        continue;
      }
      else if(curLine == "END"){
        if (!duplicateFound && isMonsterDescriptionValid(description)) printMonsterDesciption(description);
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
                          description.DESC = curLine.substr(0, 77) + '\n';
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
                description.COLOR = curLine.substr(5, curLine.length());
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
                description.SPEED = curLine.substr(5, curLine.length());
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
                description.HP = curLine.substr(5, curLine.length());
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
                description.DAM = curLine.substr(5, curLine.length());
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
}

bool isMonsterDescriptionValid(monsterDesc_t description) {
    return !(description.NAME.empty() || description.DESC.empty() ||
             description.ABIL.empty() || description.COLOR.empty() ||
             description.DAM.empty()  || description.HP.empty() ||
             description.RRTY.empty() || description.SPEED.empty() ||
             description.SYMB.empty());
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

void readObjects(void) {

  char *dir = getenv("HOME");
  char *path = strcat(dir, "/.rlg327/");

  ifstream objectDescriptions;
  objectDescriptions.open(strcat(path, "/object_desc.txt"));
  string title;
  getline(objectDescriptions, title);

  if (title != "RLG327 OBJECT DESCRIPTION 1") {
    return;
  }

  objectDesc_t description;
  string curLine;
  bool duplicateFound = false;

    while(getline(objectDescriptions, curLine)) {
      if(curLine == "BEGIN MONSTER" || curLine == "") {
        continue;
      }
      else if(curLine == "END") {
        if (!duplicateFound && isObjectDescriptionValid(description))  printObjectDesciption(description);
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
                          description.DESC = curLine.substr(0, 77) + '\n';
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
                description.COLOR = curLine.substr(5, curLine.length());
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
                description.HIT = curLine.substr(5, curLine.length());
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
                description.DAM = curLine.substr(5, curLine.length());
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
                description.DODGE = curLine.substr(5, curLine.length());
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
                description.DEF = curLine.substr(5, curLine.length());
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
                description.WEIGHT = curLine.substr(5, curLine.length());
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
                description.SPEED = curLine.substr(5, curLine.length());
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
                description.VAL = curLine.substr(5, curLine.length());
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
                description.ART = curLine.substr(5, curLine.length());
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
}

bool isObjectDescriptionValid(objectDesc_t description) {
    return !(description.NAME.empty() || description.DESC.empty() ||
             description.TYPE.empty() || description.COLOR.empty() ||
             description.HIT.empty() || description.DAM.empty() ||
             description.DODGE.empty() || description.DEF.empty() ||
             description.WEIGHT.empty() || description.SPEED.empty() ||
             description.ATTR.empty() || description.VAL.empty() ||
             description.ART.empty() || description.RRTY.empty());
}
