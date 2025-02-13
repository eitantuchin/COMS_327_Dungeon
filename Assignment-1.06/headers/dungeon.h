#ifndef DUNGEON_H
#define DUNGEON_H

#include "character.h"
#include "dungeon_game.h"
#include "monster.h"
#include "pc.h"
#include <vector>

class Dungeon {
private:
    cell_t map[DUNGEON_HEIGHT][DUNGEON_WIDTH]; // Dungeon map grid
    std::vector<room_t> rooms;                 // List of rooms in the dungeon
    uint16_t numRooms;
    PC pc;                                     // Player character
    std::vector<stair_t> upwardStairs;         // List of upward stairs
    std::vector<stair_t> downwardStairs;       // List of downward stairs
    uint16_t numUpwardsStairs;                 // Number of upward stairs
    uint16_t numDownwardsStairs;               // Number of downward stairs
    int nonTunnelingMap[DUNGEON_HEIGHT][DUNGEON_WIDTH]; // Distance map for non-tunneling monsters
    int tunnelingMap[DUNGEON_HEIGHT][DUNGEON_WIDTH];    // Distance map for tunneling monsters
    std::vector<Monster> monsters;             // List of monsters in the dungeon
    uint16_t numMonsters;                      // Number of monsters
    mode_type_t modeType;                      // Current mode of the dungeon (e.g., player control, monster list)

public:
    // Constructor
    Dungeon();

    // Getters
    cell_t (&getMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    std::vector<room_t>& getRooms() const;
    uint16_t getNumRooms() const;
    PC& getPC() ;
    std::vector<stair_t>& getUpwardStairs() const;
    std::vector<stair_t>& getDownwardStairs() const;
    uint16_t getNumUpwardsStairs() const;
    uint16_t getNumDownwardsStairs() const;
    int (&getNonTunnelingMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    int (&getTunnelingMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH];
    std::vector<Monster>& getMonsters() const;
    uint16_t getNumMonsters() const;
    mode_type_t getModeType() const;

    // Setters
    void setMap(const cell_t newMap[DUNGEON_HEIGHT][DUNGEON_WIDTH]);
    void setRooms(const std::vector<room_t>& newRooms);
    void setNumRooms(uint16_t num);
    void setPC(const PC& newPC);
    void setUpwardStairs(const std::vector<stair_t>& newStairs);
    void setDownwardStairs(const std::vector<stair_t>& newStairs);
    void setNumUpwardsStairs(uint16_t num);
    void setNumDownwardsStairs(uint16_t num);
    void setNonTunnelingMap(const int newMap[DUNGEON_HEIGHT][DUNGEON_WIDTH]);
    void setTunnelingMap(const int newMap[DUNGEON_HEIGHT][DUNGEON_WIDTH]);
    void setMonsters(const std::vector<Monster>& newMonsters);
    void setNumMonsters(uint16_t num);
    void setModeType(mode_type_t newModeType);

    // Additional methods
    void addRoom(const room_t& room);
    void addUpwardStair(const stair_t& stair);
    void addDownwardStair(const stair_t& stair);
    void addMonster(const Monster& monster);
    void removeMonster(size_t index);
};

#endif // DUNGEON_H
