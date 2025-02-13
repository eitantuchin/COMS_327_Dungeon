#include "dungeon.h"
#include "monster.h"
#include "pc.h"
#include "dungeon_game.h"

// Constructor implementation
Dungeon::Dungeon()
: numUpwardsStairs(0), numDownwardsStairs(0), numMonsters(0), numRooms(0), pc(0, 0, '.', PC_SPEED, PLAYER_CELL, UP){
    // Initialize the map with default values
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            map[y][x] = ROCK_CELL; // Default to rock cells
        }
    }

    // Initialize distance maps
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            nonTunnelingMap[y][x] = INT_MAX; // Default to "infinity"
            tunnelingMap[y][x] = INT_MAX;    // Default to "infinity"
        }
    }

    // Initialize mode type
    modeType = PLAYER_CONTROL;
}

// Getter for map
const cell_t (&Dungeon::getMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return map;
}

// Getter for rooms
const std::vector<room_t>& Dungeon::getRooms() const {
    return rooms;
}

uint16_t Dungeon::getNumRooms() const {
    return numRooms;
}

// Getter for PC
const PC& Dungeon::getPC()  {
    return pc;
}

// Getter for upward stairs
const std::vector<stair_t>& Dungeon::getUpwardStairs() const {
    return upwardStairs;
}

// Getter for downward stairs
const std::vector<stair_t>& Dungeon::getDownwardStairs() const {
    return downwardStairs;
}

// Getter for numUpwardsStairs
uint16_t Dungeon::getNumUpwardsStairs() const {
    return numUpwardsStairs;
}

// Getter for numDownwardsStairs
uint16_t Dungeon::getNumDownwardsStairs() const {
    return numDownwardsStairs;
}

// Getter for nonTunnelingMap
const int (&Dungeon::getNonTunnelingMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return nonTunnelingMap;
}

// Getter for tunnelingMap
const int (&Dungeon::getTunnelingMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return tunnelingMap;
}

// Getter for monsters
const std::vector<Monster>& Dungeon::getMonsters() const {
    return monsters;
}

// Getter for numMonsters
uint16_t Dungeon::getNumMonsters() const {
    return numMonsters;
}

// Getter for modeType
mode_type_t Dungeon::getModeType() const {
    return modeType;
}

// Setter for map
void Dungeon::setMap(const cell_t newMap[DUNGEON_HEIGHT][DUNGEON_WIDTH]) {
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            map[y][x] = newMap[y][x];
        }
    }
}

// Setter for rooms
void Dungeon::setRooms(const std::vector<room_t>& newRooms) {
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
void Dungeon::setUpwardStairs(const std::vector<stair_t>& newStairs) {
    upwardStairs = newStairs;
    numUpwardsStairs = upwardStairs.size();
}

// Setter for downward stairs
void Dungeon::setDownwardStairs(const std::vector<stair_t>& newStairs) {
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

// Setter for nonTunnelingMap
void Dungeon::setNonTunnelingMap(const int newMap[DUNGEON_HEIGHT][DUNGEON_WIDTH]) {
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            nonTunnelingMap[y][x] = newMap[y][x];
        }
    }
}

// Setter for tunnelingMap
void Dungeon::setTunnelingMap(const int newMap[DUNGEON_HEIGHT][DUNGEON_WIDTH]) {
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            tunnelingMap[y][x] = newMap[y][x];
        }
    }
}

// Setter for monsters
void Dungeon::setMonsters(const std::vector<Monster>& newMonsters) {
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

// Add a room to the dungeon
void Dungeon::addRoom(const room_t& room) {
    rooms.push_back(room);
    numRooms++;
}

// Add an upward stair to the dungeon
void Dungeon::addUpwardStair(const stair_t& stair) {
    upwardStairs.push_back(stair);
    numUpwardsStairs++;
}

// Add a downward stair to the dungeon
void Dungeon::addDownwardStair(const stair_t& stair) {
    downwardStairs.push_back(stair);
    numDownwardsStairs++;
}

// Add a monster to the dungeon
void Dungeon::addMonster(const Monster& monster) {
    monsters.push_back(monster);
    numMonsters++;
}

// Remove a monster from the dungeon
void Dungeon::removeMonster(size_t index) {
    if (index < monsters.size()) {
        monsters.erase(monsters.begin() + index);
        numMonsters--;
    }
}

