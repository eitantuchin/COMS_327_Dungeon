#pragma once

#include "character.hpp"
#include "dungeon_game.hpp"
#include "monster.hpp"
#include "pc.hpp"
#include "item.hpp"
#include <vector>
#include <unordered_map>
#include <functional> // For std::hash

using namespace std;

// Specialize std::hash for std::pair<int, int>
namespace std {
    template <>
    struct hash<pair<int, int>> {
        std::size_t operator()(const pair<int, int>& p) const {
            std::size_t h1 = std::hash<int>{}(p.first);
            std::size_t h2 = std::hash<int>{}(p.second);
            return h1 ^ (h2 << 1); // Combine hashes
        }
    };
}

class Dungeon {
private:
    cell_t map[DUNGEON_HEIGHT][DUNGEON_WIDTH]; // Dungeon map grid
    vector<room_t> rooms;                 // List of rooms in the dungeon
    uint16_t numRooms;
    PC pc;                                     // Player character
    vector<stair_t> upwardStairs;         // List of upward stairs
    vector<stair_t> downwardStairs;       // List of downward stairs
    uint16_t numUpwardsStairs;                 // Number of upward stairs
    uint16_t numDownwardsStairs;               // Number of downward stairs
    int nonTunnelingMap[DUNGEON_HEIGHT][DUNGEON_WIDTH]; // Distance map for non-tunneling monsters
    int tunnelingMap[DUNGEON_HEIGHT][DUNGEON_WIDTH];    // Distance map for tunneling monsters
    int nonTunnelingPassMap[DUNGEON_HEIGHT][DUNGEON_WIDTH];    // Distance map for tunneling monsters
    vector<Monster> monsters;             // List of monsters in the dungeon
    uint16_t numMonsters;                      // Number of monsters
    mode_type_t modeType;                      // Current mode of the dungeon (e.g., player control, monster list)
    vector<Item> dungeonItems;
    vector<Item> itemMap[DUNGEON_HEIGHT][DUNGEON_WIDTH];

public:
    // Constructor
    Dungeon();

    // Getters
    cell_t (&getMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    vector<room_t>& getRooms();
    uint16_t getNumRooms();
    PC& getPC();
    vector<stair_t>& getUpwardStairs();
    vector<stair_t>& getDownwardStairs();
    uint16_t getNumUpwardsStairs();
    uint16_t getNumDownwardsStairs();
    int (&getNonTunnelingMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    int (&getTunnelingMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    int (&getNonTunnelingPassMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    vector<Monster>& getMonsters();
    uint16_t getNumMonsters();
    mode_type_t getModeType();
    vector<Item>& getDungeonItems();
    vector<Item> (&getItemMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];

    // Setters
    void setRooms(const vector<room_t>& newRooms);
    void setNumRooms(uint16_t num);
    void setPC(const PC& newPC);
    void setUpwardStairs(const vector<stair_t>& newStairs);
    void setDownwardStairs(const vector<stair_t>& newStairs);
    void setNumUpwardsStairs(uint16_t num);
    void setNumDownwardsStairs(uint16_t num);
    void setMonsters(const vector<Monster>& newMonsters);
    void setNumMonsters(uint16_t num);
    void setModeType(mode_type_t newModeType);
    void setDungeonItems(vector<Item> newItems);
};

void addRooms(void);
void addStairs(void);
void initItems(void);
void addCorridors(void);
void carveCorridor(int startX, int startY, int endX, int endY);
void initImmutableRock(void);
void initPCPosition(void);
void initMonsters(void);
void generateDungeon(void);
void useStairs(int key);
void resetDungeonLevel(void);
