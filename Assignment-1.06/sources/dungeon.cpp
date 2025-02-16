#include "../headers/dungeon.hpp"
#include "../headers/monster.hpp"
#include "../headers/pc.hpp"
#include "../headers/dungeon_game.hpp"

using namespace std;


// Constructor implementation
Dungeon::Dungeon()
: numUpwardsStairs(0), numDownwardsStairs(0), numRooms(0), numMonsters(0), pc(0, 0, ROOM_CELL, PC_SPEED, PLAYER_CELL, UP){
}

// Getter for map
cell_t (&Dungeon::getMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return map;
}

// Getter for rooms
vector<room_t>& Dungeon::getRooms() {
    return rooms;
}

uint16_t Dungeon::getNumRooms() {
    return numRooms;
}

// Getter for PC
PC& Dungeon::getPC() {
    return pc;
}

// Getter for upward stairs
vector<stair_t>& Dungeon::getUpwardStairs(){
    return upwardStairs;
}

// Getter for downward stairs
vector<stair_t>& Dungeon::getDownwardStairs(){
    return downwardStairs;
}

// Getter for numUpwardsStairs
uint16_t Dungeon::getNumUpwardsStairs()  {
    return numUpwardsStairs;
}

// Getter for numDownwardsStairs
uint16_t Dungeon::getNumDownwardsStairs()  {
    return numDownwardsStairs;
}

// Getter for nonTunnelingMap
int (&Dungeon::getNonTunnelingMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return nonTunnelingMap;
}

// Getter for tunnelingMap
int (&Dungeon::getTunnelingMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return tunnelingMap;
}

// Getter for monsters
vector<Monster>& Dungeon::getMonsters(){
    return monsters;
}

// Getter for numMonsters
uint16_t Dungeon::getNumMonsters() {
    return numMonsters;
}

// Getter for modeType
mode_type_t Dungeon::getModeType() {
    return modeType;
}

// Setter for rooms
void Dungeon::setRooms(const vector<room_t>& newRooms) {
    rooms = newRooms;
}

void Dungeon::setNumRooms(uint16_t num) {
    numRooms = num;
}

// Setter for PC
void Dungeon::setPC(const PC& newPC) {
    pc = newPC;
}

// Setter for upward stairs
void Dungeon::setUpwardStairs(const vector<stair_t>& newStairs) {
    upwardStairs = newStairs;
    numUpwardsStairs = upwardStairs.size();
}

// Setter for downward stairs
void Dungeon::setDownwardStairs(const vector<stair_t>& newStairs) {
    downwardStairs = newStairs;
    numDownwardsStairs = downwardStairs.size();
}

// Setter for numUpwardsStairs
void Dungeon::setNumUpwardsStairs(uint16_t num) {
    numUpwardsStairs = num;
}

// Setter for numDownwardsStairs
void Dungeon::setNumDownwardsStairs(uint16_t num) {
    numDownwardsStairs = num;
}

// Setter for monsters
void Dungeon::setMonsters(const vector<Monster>& newMonsters) {
    monsters = newMonsters;
    numMonsters = monsters.size();
}

// Setter for numMonsters
void Dungeon::setNumMonsters(uint16_t num) {
    numMonsters = num;
}

// Setter for modeType
void Dungeon::setModeType(mode_type_t newModeType) {
    modeType = newModeType;
}


