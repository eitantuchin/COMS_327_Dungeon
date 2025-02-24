#include "../headers/dungeon.hpp"
#include "../headers/monster.hpp"
#include "../headers/pc.hpp"
#include "../headers/dungeon_game.hpp"
#include "../headers/priority_queue.h"

using namespace std;


// Constructor implementation
Dungeon::Dungeon()
: numUpwardsStairs(0), numDownwardsStairs(0), numRooms(0), numMonsters(0), pc(0, 0, ROOM_CELL, PC_SPEED, PLAYER_CELL, UP) {
}

unordered_map<pair<int, int>, vector<Item>>& Dungeon::getItemLocationsAndPriorities() {
    return itemLocationsAndPriorities;
}

// Getter for map
cell_t (&Dungeon::getMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return map;
}

// Getter for rooms
vector<Item>& Dungeon::getDungeonItems() {
    return dungeonItems;
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

int (&Dungeon::getNonTunnelingPassMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return nonTunnelingPassMap;
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

void Dungeon::setDungeonItems(vector<Item> newItems) {
    dungeonItems = newItems;
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

void initItems(void) {
    vector<Item> items = itemFactory();
    /*if (items.empty()) {
        std::cerr << "Warning: No item templates available." << std::endl;
        dungeon.setDungeonItems(vector<Item>());
        return;
    }
    */
    vector<Item> dungeonItems;
    int numItems = rand() % (MAX_NUM_ITEMS - MIN_NUM_ITEMS + 1) + MIN_NUM_ITEMS;
    int itemsInserted = 0;
    vector<int> indexesOfItemArtifactsInserted;
    while (itemsInserted != numItems) {
        int randIndex = rand() % items.size();
        int randRarity = rand() % 100;
        char symbol = getSymbolFromType(items[randIndex].getType());
        if (items[randIndex].isEligible() && items[randIndex].getRarity() < randRarity) {
            if ((items[randIndex].isArtifact() && !contains(indexesOfItemArtifactsInserted, randIndex)) || !items[randIndex].isArtifact()) {
                if (items[randIndex].isArtifact()) {
                    indexesOfItemArtifactsInserted.push_back(randIndex);
                }
                dungeon.getMap()[items[randIndex].getPosY()][items[randIndex].getPosX()] = cell_t { symbol, -2 }; // -2 hardness is unique to objects
                itemsInserted++;
                dungeonItems.push_back(items[randIndex]);
                
                pair<int, int> itemLocation = {items[randIndex].getPosY(), items[randIndex].getPosX()};
                vector<Item> temp;
                temp.push_back(items[randIndex]);
                dungeon.getItemLocationsAndPriorities().insert(make_pair(itemLocation, temp));
            }
        }
    }
    dungeon.setDungeonItems(dungeonItems);
}

void initMonsters(void) {
    // add monsters to array
    vector<Monster> monsters(dungeon.getNumMonsters());
    for (int i = 0; i < dungeon.getNumMonsters(); ++i) {
        Monster monster;
        bool isErratic = rand() % 2 == 1;
        bool isTunneling = rand() % 2 == 1;
        bool isIntelligent = rand() % 2 == 1;
        bool isTelepathic = rand() % 2 == 1;
        uint8_t monsterBits = 0;
        if (isIntelligent) monsterBits |= (1 << 0); // Set bit 0 for intelligence
        if (isTelepathic) monsterBits |= (1 << 1);  // Set bit 1 for telepathy
        if (isTunneling) monsterBits |= (1 << 2);   // Set bit 2 for tunneling
        if (isErratic) monsterBits |= (1 << 3);     // Set bit 3 for erratic behavior
        monster.setMonsterBits(monsterBits);
        monster.setSpeed(rand() % 16 + 5);
        monster.setCell(cell_t {"0123456789abcdef"[monsterBits], -1}); // monsters have hardness -1 which is unique to them
        monster.setAlive(true);
        monster.setLastSeenPCX(-1); // Initialize last seen position
        monster.setLastSeenPCY(-1);
        
        monsters[i] = monster;
    }
    dungeon.setMonsters(monsters);
    
    // initialize positions of monsters
    
    for (int i = 0; i < dungeon.getNumMonsters(); ++i) {
        bool foundPosition = false;
        while (!foundPosition) {
            int randY = rand() % DUNGEON_HEIGHT - 2;
            int randX = rand() % DUNGEON_WIDTH - 2;
            bool pcFarAwayEnough = checkMonsterPlacementToPC(randX, randY);
            if ((dungeon.getMap()[randY][randX].ch == '.' || dungeon.getMap()[randY][randX].ch == '#') && pcFarAwayEnough) {
                dungeon.getMonsters()[i].setPosX(randX);
                dungeon.getMonsters()[i].setPosY(randY);
                dungeon.getMonsters()[i].setPreviousCell(dungeon.getMap()[randY][randX]);
                dungeon.getMap()[randY][randX] = dungeon.getMonsters()[i].getCell();
                foundPosition = true;
            }
        }
    }
}


/*
 Initializes the PC's position inside the dungeon
 */
void initPCPosition(void)
{
    int randomRoomNum = rand() % dungeon.getNumRooms();
    room_t room = dungeon.getRooms()[randomRoomNum];
    int getX = room.posX;
    int getY = room.posY;
    dungeon.getPC().setPosX(getX + 2);
    dungeon.getPC().setPosY(getY + 2);
    if (dungeon.getMap()[getY + 2][getX + 2].ch != '<' ||
        dungeon.getMap()[getY + 2][getX + 2].ch != '>')
    {
        dungeon.getMap()[getY + 2][getX + 2] = PLAYER_CELL;
    }
    else
    {
        initPCPosition(); // recursively call to get another random position because we cannot have the PC position override a staircase
    }
}

/*
 Adds the staircases to the dungeon and ensures no room contains multiple stairs and that we have at least one type of each staircase
 */
void addStairs(void)
{
    int numStairs = rand() % (MAX_NUM_STAIRS - MIN_NUM_STAIRS + 1) + MIN_NUM_STAIRS;
    vector<int> randomRoomNums(MAX_NUM_STAIRS);
    dungeon.setNumUpwardsStairs(0);
    dungeon.setNumDownwardsStairs(0);
    while (dungeon.getNumUpwardsStairs() + dungeon.getNumDownwardsStairs() != numStairs) {
        // ensure at least one upwards stairs is inside the dungeon
        int randomRoomNum;
        while (true) {
            randomRoomNum = rand() % dungeon.getNumRooms();
            if (contains(randomRoomNums, randomRoomNum)) {
                continue;
            }
            randomRoomNums[dungeon.getNumUpwardsStairs() + dungeon.getNumDownwardsStairs()] = randomRoomNum;
            break;
        }
        room_t room = dungeon.getRooms()[randomRoomNum];
        //printf("Room width is: %i \n", room.width);
        //printf("Room height is: %i \n", room.height);
        int randomX = room.posX + rand() % room.width;
        int randomY = room.posY + rand() % room.height;
        stair_t stair;
        stair.posX = randomX;
        stair.posY = randomY;
        if (dungeon.getNumUpwardsStairs() + dungeon.getNumDownwardsStairs() == 0)
        {
            dungeon.getMap()[randomY][randomX] = UPWARD_STAIRS_CELL;
            dungeon.getUpwardStairs()[dungeon.getNumUpwardsStairs()] = stair;
            dungeon.setNumUpwardsStairs(dungeon.getNumUpwardsStairs() + 1);
        }
        // ensure at least one downwards stairs is inside the dungeon
        else if (dungeon.getNumUpwardsStairs() + dungeon.getNumDownwardsStairs() == 1)
        {
            dungeon.getMap()[randomY][randomX] = DOWNWARD_STAIRS_CELL;
            dungeon.getDownwardStairs()[dungeon.getNumDownwardsStairs()] = stair;
            dungeon.setNumDownwardsStairs(dungeon.getNumDownwardsStairs() + 1);
        }
        else
        {
            int randomStairCaseDirection = rand() % 2 + 1;
            if (randomStairCaseDirection == 1) {
                dungeon.getMap()[randomY][randomX] = UPWARD_STAIRS_CELL;
                dungeon.getUpwardStairs()[dungeon.getNumUpwardsStairs()] = stair;
                dungeon.setNumUpwardsStairs(dungeon.getNumUpwardsStairs() + 1);
            }
            else {
                dungeon.getMap()[randomY][randomX] = DOWNWARD_STAIRS_CELL;
                dungeon.getDownwardStairs()[dungeon.getNumDownwardsStairs()] = stair;
                dungeon.setNumDownwardsStairs(dungeon.getNumDownwardsStairs() + 1);
            }
        }
    }
}

/*
 Initializes all the immutable rock and regular rock
 */
void initImmutableRock(void)
{
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        for (int x = 0; x < DUNGEON_WIDTH; x++)
        {
            // each rock has a different hardness level
            dungeon.getMap()[y][x] = ROCK_CELL;
            int randomHardness = rand() % 254 + 1; // max hardness is 255 - 1
            dungeon.getMap()[y][x].hardness = randomHardness;
        }
    }

    int maxX = DUNGEON_WIDTH - 1;
    int maxY = DUNGEON_HEIGHT - 1;
    for (int y = 0; y < DUNGEON_HEIGHT; y++)
    {
        dungeon.getMap()[y][0] = IMMUTABLE_ROCK_CELL;
        dungeon.getMap()[y][maxX] = IMMUTABLE_ROCK_CELL;
    }
    for (int x = 0; x < DUNGEON_WIDTH; x++)
    {
        dungeon.getMap()[0][x] = IMMUTABLE_ROCK_CELL;
        dungeon.getMap()[maxY][x] = IMMUTABLE_ROCK_CELL;
    }
}

/*
 Adds the corridors to the dungeon using Euclidean distance
 */
void addCorridors(void)
{
    int roomCentroids[dungeon.getRooms().size()][2];
    for (int i = 0; i < dungeon.getRooms().size(); ++i)
    {
        // init room centroids for all rooms
        roomCentroids[i][0] = dungeon.getRooms()[i].posX + dungeon.getRooms()[i].width / 2;
        roomCentroids[i][1] = dungeon.getRooms()[i].posY + dungeon.getRooms()[i].height / 2;
    }

    // find smallest distance
    for (int i = 1; i < dungeon.getRooms().size(); ++i)
    {
        int minDist = DUNGEON_HEIGHT * DUNGEON_WIDTH;
        int closestRoomIndex = 0;
        for (int j = 0; j < i; ++j)
        {
            double dist = sqrt(pow(roomCentroids[i][0] - roomCentroids[j][0], 2) +
                               pow(roomCentroids[i][1] - roomCentroids[j][1], 2));
            if (dist < minDist)
            {
                minDist = dist;
                closestRoomIndex = j;
            }
        }

        int startX = roomCentroids[i][0];
        int startY = roomCentroids[i][1];
        int endX = roomCentroids[closestRoomIndex][0];
        int endY = roomCentroids[closestRoomIndex][1];
        carveCorridor(startX, startY, endX, endY);
    }
}

/*
 Creates a corridor on the dungeon map
 */
void carveCorridor(int startX, int startY, int endX, int endY)
{
    int x = startX;
    int y = startY;
    bool xWentLast = false;

    while (x != endX || y != endY)
    {
        // xWentLast ensures that when we have diagonal movement that we insert a corridor in a cardinal direction
        if (!xWentLast)
        {
            if (x < endX)
                x++;
            else if (x > endX)
                x--;
            xWentLast = true;
        }
        else
        {
            if (y < endY)
                y++;
            else if (y > endY)
                y--;
            xWentLast = false;
        }

        if (dungeon.getMap()[y][x].ch == ROCK_CELL.ch &&
            dungeon.getMap()[y][x].hardness < IMMUTABLE_ROCK_CELL.hardness) {
            dungeon.getMap()[y][x] = CORRIDOR_CELL;
        }
    }
}

/*
 Adds the rooms to the dungeon
 */
void addRooms(void)
{
    dungeon.setNumRooms(rand() % (MAX_NUM_ROOMS - MIN_NUM_ROOMS + 1) + MIN_NUM_ROOMS);
    int counter = 0;

    while (counter != dungeon.getNumRooms())
    {
        bool roomInserted = false;
        while (!roomInserted)
        {
            int randY = rand() % (DUNGEON_HEIGHT - 2) + 1;
            int randX = rand() % (DUNGEON_WIDTH - 2) + 1;
            int randHeight = rand() % (MAX_ROOM_HEIGHT - MIN_ROOM_HEIGHT + 1) + MIN_ROOM_HEIGHT;
            int randWidth = rand() % (MAX_ROOM_WIDTH - MIN_ROOM_WIDTH + 1) + MIN_ROOM_WIDTH;

            if (randY + randHeight <= DUNGEON_HEIGHT - 1 && randX + randWidth <= DUNGEON_WIDTH - 1)
            {
                bool validRoomPositionFound = true;
                // ensure that we don't create the room where immutable rock lies
                for (int y = randY - 2; y < randY + randHeight + 2; ++y)
                {
                    for (int x = randX - 2; x < randX + randWidth + 2; ++x)
                    {
                        if (dungeon.getMap()[y][x].ch != ROCK_CELL.ch || dungeon.getMap()[y][x].hardness == IMMUTABLE_ROCK_CELL.hardness)
                        {
                            validRoomPositionFound = false;
                            break;
                        }
                    }
                    if (!validRoomPositionFound)
                        break;
                }
                // add the room to the dungeon
                if (validRoomPositionFound)
                {
                    dungeon.getRooms()[counter].height = randHeight;
                    dungeon.getRooms()[counter].width = randWidth;
                    dungeon.getRooms()[counter].posX = randX;
                    dungeon.getRooms()[counter].posY = randY;
                    
                    for (int y = randY; y < randY + randHeight; ++y)
                    {
                        for (int x = randX; x < randX + randWidth; ++x)
                        {
                            dungeon.getMap()[y][x] = ROOM_CELL;
                        }
                    }
                    roomInserted = true;
                    counter++;
                }
            }
        }
    }
}

void generateDungeon(void) {
    initImmutableRock();
    addRooms();
    addCorridors();
    addStairs();
    initPCPosition();
    initMonsters();
    initItems();
}

void resetDungeonLevel(void) {
    // Clear dungeon data
    memset(dungeon.getPC().getFogMap(), false, sizeof(dungeon.getPC().getFogMap()));  // Set all values to false
    // Reset counters and global variables
    dungeon.setNumRooms(MIN_NUM_ROOMS);
    dungeon.setRooms(vector<room_t>(dungeon.getNumRooms()));
    dungeon.setUpwardStairs(vector<stair_t>(3));
    dungeon.setDownwardStairs(vector<stair_t>(3));
    dungeon.setNumUpwardsStairs(0);
    dungeon.setNumDownwardsStairs(0);
    dungeon.getPC().setPreviousCell(ROOM_CELL);
    dungeon.getItemLocationsAndPriorities().clear();
    Dungeon();
    // Clear and reset the event queue
    event_queue = my_priority_queue();

    // Generate a new dungeon level
    generateDungeon();

    // Recalculate pathfinding distances for both tunneling and non-tunneling monsters
    calculateDistances(0); // Non-tunneling
    calculateDistances(1); // Tunneling
    calculateDistances(2);

    // Schedule events for the player and monsters
    scheduleEvent(EVENT_PC, -1, 0);
    processEvents();
    for (int i = 0; i < dungeon.getNumMonsters(); i++) {
        scheduleEvent(EVENT_MONSTER, i, 0);
    }

    // Display a message to the player
    char message[100];
    snprintf(message, sizeof(message), "Your turn to move!");
    displayMessage(message);
}

