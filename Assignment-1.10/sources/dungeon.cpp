#include "../headers/dungeon.hpp"
#include "../headers/monster.hpp"
#include "../headers/pc.hpp"
#include "../headers/dungeon_game.hpp"
#include "../headers/priority_queue.h"
#include "../headers/item.hpp"
#include <algorithm>
#include <queue>
#include <cstdlib>
#include <ctime>

using namespace std;

void generateLiquids() {
    const int MAX_CELLS = 30;      // Max cells per pool (controls size)
    const float SPREAD_CHANCE = 0.6f; // Probability of spreading to a neighbor

    // Directions for 4-way spread (up, down, left, right)
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    for (int poolType = 0; poolType < 2; ++poolType) { // 0 = water, 1 = lava
        const int NUM_POOLS = 2 + rand() % 4;
        for (int i = 0; i < NUM_POOLS; i++) {
            bool poolPlaced = false;
            while (!poolPlaced) {
                // Random seed position
                int seedX = rand() % (DUNGEON_WIDTH - 2) + 1;
                int seedY = rand() % (DUNGEON_HEIGHT - 2) + 1;

                cell_t& seedCell = dungeon.getMap()[seedY][seedX];
                if (seedCell.ch != ' ' || seedCell.hardness == IMMUTABLE_ROCK_CELL.hardness) {
                    continue;
                }

                // Use a queue for flood-fill-like growth
                queue<pair<int, int>> toVisit;
                vector<vector<bool>> visited(DUNGEON_HEIGHT, vector<bool>(DUNGEON_WIDTH, false));
                toVisit.push({seedX, seedY});
                visited[seedY][seedX] = true;
                int cellsPlaced = 0;

                while (!toVisit.empty() && cellsPlaced < MAX_CELLS) {
                    pair<int, int> current = toVisit.front();
                    int x = current.first;
                    int y = current.second;
                    toVisit.pop();

                    // Place liquid
                    if (dungeon.getMap()[y][x].ch != ' ' || dungeon.getMap()[y][x].hardness == IMMUTABLE_ROCK_CELL.hardness) {
                        continue;
                    }
                    dungeon.getMap()[y][x] = (poolType == 0) ? WATER_CELL : LAVA_CELL;
                    
                    cellsPlaced++;

                    // Spread to neighbors randomly
                    for (int dir = 0; dir < 4; dir++) {
                        int nx = x + dx[dir];
                        int ny = y + dy[dir];

                        // Check bounds
                        if (nx < 1 || nx >= DUNGEON_WIDTH - 1 || ny < 1 || ny >= DUNGEON_HEIGHT - 1) {
                            continue;
                        }

                        // Skip if already visited or invalid terrain
                        if (visited[ny][nx]) {
                            continue;
                        }
                        cell_t& neighbor = dungeon.getMap()[ny][nx];
                        if (neighbor.ch != ' ' || neighbor.hardness == IMMUTABLE_ROCK_CELL.hardness) {
                            continue;
                        }

                        // Randomly decide to spread
                        if ((float)rand() / RAND_MAX < SPREAD_CHANCE) {
                            toVisit.push({nx, ny});
                            visited[ny][nx] = true;
                        }
                    }
                }

                if (cellsPlaced > 10) {
                    poolPlaced = true; // Successfully placed at least one cell
                }
            }
        }
    }
}

void displayShop(Item randItem) {
    vector<Item> shopItems = {};
    shopItems.push_back(randItem);
    // Health potion, invis potion, speed potion
    Item healthPotion = Item(0, 0, "Potion of Healing", "Brewed by adding golden watermelon to suspicious water. Take and become heartened!", "FLASK", {COLOR_MAGENTA}, "0+0d1", 0, 0, 0, 2, 0, "250+6d8", 200, true, 80, SHOP_CELL, true, false);
    Item speedPotion = Item(0, 0, "Potion of Speed", "Brewed by adding sugar to suspicious water. Take and become fast AF!", "FLASK", {COLOR_GREEN}, "0+0d1", 0, 0, 0, 2, rollDice("10+5d4"), "0+0d1", 150, true, 80, SHOP_CELL, true, false);
    Item invisPotion = Item(0, 0, "Potion of Invisibility", "Brewed by adding a fermented spider eye to suspicious water. Take and become invisible!", "FLASK", {COLOR_WHITE}, "0+0d1", 0, 0, 0, 2, 0, "0+0d1", 250, true, 80, SHOP_CELL, true, false);
    shopItems.push_back(healthPotion);
    shopItems.push_back(speedPotion);
    shopItems.push_back(invisPotion);
    dungeon.getShopItems() = shopItems;
    uint8_t count = shopItems.size();

    // Screen dimensions
    int screenHeight, screenWidth;
    getmaxyx(stdscr, screenHeight, screenWidth);

    // ASCII Shopkeeper
    const char* shopkeeper[] = {
        "   ___   ",
        "  /   \\  ",
        "  O   O  ",
        "  _/|_   ",
        " /  |  \\ ",
        "|_______|",
        "|  ***  |"
    };
    const int shopkeeperLines = 7;
    const int shopkeeperWidth = 11; // Widest line length
    int shopkeeperY = 12; // Starting row
    int shopkeeperX = (screenWidth - shopkeeperWidth) / 2; // Centered horizontally

    // Draw shopkeeper in magenta
    attron(COLOR_PAIR(COLOR_MAGENTA));
    for (int i = 0; i < shopkeeperLines; i++) {
        mvprintw(shopkeeperY + i, shopkeeperX, "%s", shopkeeper[i]);
    }
    attroff(COLOR_PAIR(COLOR_MAGENTA));

    // Centered welcome message below shopkeeper
    const char* welcomeMsg = "Welcome to my shop! See anything you want to buy?";
    int welcomeLength = (int) strlen(welcomeMsg);
    int welcomeX = (screenWidth - welcomeLength) / 2;
    mvprintw(shopkeeperY + shopkeeperLines, welcomeX, "%s", welcomeMsg);

    // Center "Shop Menu" title
    const char* title = "Shop Menu";
    size_t titleLength = strlen(title);
    size_t titleX = (screenWidth - titleLength) / 2;
    if (titleX < 0) titleX = 1;
    mvprintw(0, (int) titleX, "%s", title);

    // Header (positioned above shopkeeper)
    int itemsStartX = 1; // Left-aligned, above shopkeeper
    mvprintw(1, itemsStartX, "Symbol | Name                      | Coins      | Type       | Artifact");
    mvprintw(2, itemsStartX, "------ | ------------------------- | ---------- | ---------- | --------");

    // Display items (rows 3-11, before shopkeeper)
    int maxVisibleItems = shopkeeperY - 3; // From row 3 to row 11 (9 rows max)
    if (maxVisibleItems > (int)count) maxVisibleItems = count;
    
    // Ensure scrollOffset is within bounds
    if (scrollOffset < 0) {
        scrollOffset = 0;
    }
    if (scrollOffset > count - 1) {
        scrollOffset = count - 1;
    }
    reverse(shopItems.begin(), shopItems.end());
    for (int i = 0; i < maxVisibleItems; ++i) {
        const Item& item = shopItems[i];

        // Symbol with item color
        short color = item.getColor()[0];
        attron(COLOR_PAIR(color));
        mvprintw(i + 3, itemsStartX, "%-6s", string(1, getSymbolFromType(item.getType())).c_str());
        attroff(COLOR_PAIR(color));
        mvprintw(i + 3, itemsStartX + 6, " | ");

        // Name (highlighted in yellow if selected)
        string name = item.getName().substr(0, 25);
        if (i == scrollOffset) {
            attron(COLOR_PAIR(COLOR_YELLOW));
            mvprintw(i + 3, itemsStartX + 9, "%-25s", name.c_str());
            attroff(COLOR_PAIR(COLOR_YELLOW));
        } else {
            mvprintw(i + 3, itemsStartX + 9, "%-25s", name.c_str());
        }
        mvprintw(i + 3, itemsStartX + 34, " | ");

        // Coins
        attron(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(i + 3, itemsStartX + 37, "%-10d", item.getValue());
        attroff(COLOR_PAIR(COLOR_YELLOW));
        mvprintw(i + 3, itemsStartX + 47, " | ");

        // Type
        string type = item.getType().substr(0, 10);
        mvprintw(i + 3, itemsStartX + 50, "%-10s", type.c_str());
        mvprintw(i + 3, itemsStartX + 60, " | ");

        // Artifact
        string artifactText = item.isArtifact() ? "YES" : "NO";
        mvprintw(i + 3, itemsStartX + 63, "%-8s", artifactText.c_str());
    }

    // Player coins (below shopkeeper and welcome message)
    string userCoins = "Coins available: " + to_string(dungeon.getPC().getCoins());
    size_t stringX = (screenWidth - userCoins.length()) / 2;
    attron(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(shopkeeperY + shopkeeperLines + 1, (int) stringX, "%-70s", userCoins.c_str());
    attroff(COLOR_PAIR(COLOR_YELLOW));
    
    // Instructions (bottom of screen)
    mvprintw(screenHeight - 1, 1, "Use UP/DOWN to select, ENTER to buy item, [T] to exit.");

    refresh();
}


void generateShop(void) {
    vector<Item> itemsGot = itemFactory();
    randShopItem = itemsGot[rand() % itemsGot.size()];
    bool shopPlaced = false;
    while (!shopPlaced) {
        // Randomly pick a potential center for the 3x3 shop
        int centerX = rand() % (DUNGEON_WIDTH - 4) + 2; // Avoid borders
        int centerY = rand() % (DUNGEON_HEIGHT - 4) + 2;
        int topLeftX = centerX - 1;
        int topLeftY = centerY - 1;

        // Check if the 3x3 area is within bounds
        if (topLeftX < 1 || topLeftX + 2 >= DUNGEON_WIDTH - 1 ||
            topLeftY < 1 || topLeftY + 2 >= DUNGEON_HEIGHT - 1) {
            continue;
        }

        bool isValidRock = true;
        for (int y = topLeftY; y <= topLeftY + 2 && isValidRock; y++) {
            for (int x = topLeftX; x <= topLeftX + 2; x++) {
                cell_t& cell = dungeon.getMap()[y][x];
                if (cell.ch != ROCK_CELL.ch || cell.hardness == IMMUTABLE_ROCK_CELL.hardness) {
                    isValidRock = false; // Not mutable rock
                    break;
                }
            }
        }

        if (!isValidRock) {
            continue;
        }

        for (int y = topLeftY; y <= topLeftY + 2; y++) {
            for (int x = topLeftX; x <= topLeftX + 2; x++) {
                if (x == centerX && y == centerY) {
                    dungeon.getMap()[y][x] = {'S', -3}; // shopkeeper cell has -3 hardness
                } else {
                    dungeon.getMap()[y][x] = SHOP_CELL; // Shop floor
                }
            }
        }

        // Find the nearest room and connect with a corridor
        vector<room_t>& rooms = dungeon.getRooms();
        if (rooms.empty()) {
            shopPlaced = true; // No rooms to connect to, shop still placed
            break;
        }

       
        int nearestRoomIdx = -1;
        int minDistance = INT_MAX;
        for (int i = 0; i < rooms.size(); i++) {
            int roomCenterX = rooms[i].posX + rooms[i].width / 2;
            int roomCenterY = rooms[i].posY + rooms[i].height / 2;
            int dist = abs(centerX - roomCenterX) + abs(centerY - roomCenterY); // Manhattan distance
            if (dist < minDistance) {
                minDistance = dist;
                nearestRoomIdx = i;
            }
        }

        if (nearestRoomIdx == -1) {
            shopPlaced = true; //
            break;
        }

        int roomCenterX = rooms[nearestRoomIdx].posX + rooms[nearestRoomIdx].width / 2;
        int roomCenterY = rooms[nearestRoomIdx].posY + rooms[nearestRoomIdx].height / 2;

        int x = centerX;
        int y = centerY;
        while (x != roomCenterX) {
            x += (roomCenterX > x) ? 1 : -1;
            if (dungeon.getMap()[y][x].hardness != IMMUTABLE_ROCK_CELL.hardness &&
                dungeon.getMap()[y][x].ch == ' ') {
                dungeon.getMap()[y][x] = CORRIDOR_CELL; // '#'
            }
        }
        while (y != roomCenterY) {
            y += (roomCenterY > y) ? 1 : -1;
            if (dungeon.getMap()[y][x].hardness != IMMUTABLE_ROCK_CELL.hardness &&
                dungeon.getMap()[y][x].ch == ' ') {
                dungeon.getMap()[y][x] = CORRIDOR_CELL; // '#'
            }
        }
        shopPlaced = true;
    }
}

void initItems(void) {
   // int hardnessLevel = dungeon.getPC().getFloorsVisited() * 10;
    // hardness level must be capped at 80
   // if (hardnessLevel > 80) hardnessLevel = 80;
    vector<Item> items = itemFactory();
    vector<Item> dungeonItems;
    int numItems = rand() % (MAX_NUM_ITEMS - MIN_NUM_ITEMS + 1) + MIN_NUM_ITEMS;
    int itemsInserted = 0;
    vector<int> indexesOfItemArtifactsInserted;
    while (itemsInserted != numItems) {
        pair<int, int> coordinates = getItemCoordinates();
        int randIndex = rand() % items.size();
        int randRarity = rand() % (100);
        if (items[randIndex].isEligible() && items[randIndex].getRarity() < randRarity ) {
            if ((items[randIndex].isArtifact() && !containsInt(indexesOfItemArtifactsInserted, randIndex)) || !items[randIndex].isArtifact()) {
                items[randIndex].setPosX(coordinates.second);
                items[randIndex].setPosY(coordinates.first);
                if (items[randIndex].isArtifact()) {
                    indexesOfItemArtifactsInserted.push_back(randIndex);
                }
                // problem here
                char symbol = getSymbolFromType(items[randIndex].getType());
                dungeon.getMap()[items[randIndex].getPosY()][items[randIndex].getPosX()] = cell_t { symbol, -2 }; // -2 hardness is unique to objects
                itemsInserted++;
                dungeonItems.push_back(items[randIndex]);
                dungeon.getItemMap()[items[randIndex].getPosY()][items[randIndex].getPosX()].push_back(items[randIndex]);
            }
        }
    }
    dungeon.setItems(dungeonItems);
}

void initMonsters(void) {
    // hardness level must be capped at 80
    vector<Monster> monsters = monsterFactory();
    vector<Monster> dungeonMonsters;
    int monstersInserted = 0;
    vector<int> indexesOfUniqueMonstersInserted;
    while (monstersInserted != dungeon.getNumMonsters()) {
        pair<int, int> coordinates = getMonsterCoordinates();
        int randIndex = rand() % monsters.size();
        int randRarity = rand() % (100);
        if (monsters[randIndex].isAlive() && monsters[randIndex].getRarity() < randRarity) {
            // if the monster is unique and hasn't been inserted yet or if the monster isn't unique
            bool isUnique = containsString(monsters[randIndex].getAbilities(), string("UNIQ"));
            if ((isUnique && !containsInt(indexesOfUniqueMonstersInserted, randIndex)) || !isUnique) {
                monsters[randIndex].setPosX(coordinates.second);
                monsters[randIndex].setPosY(coordinates.first);
                if (isUnique) {
                    indexesOfUniqueMonstersInserted.push_back(randIndex);
                }
                dungeon.getMap()[monsters[randIndex].getPosY()][monsters[randIndex].getPosX()] = monsters[randIndex].getCell();
                monstersInserted++;
                dungeonMonsters.push_back(monsters[randIndex]);
            }
        }
    }
    dungeon.setMonsters(dungeonMonsters);
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
            if (containsInt(randomRoomNums, randomRoomNum)) {
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
    int roomCentroids[dungeon.getNumRooms()][2];
    for (int i = 0; i < dungeon.getNumRooms(); ++i)
    {
        // init room centroids for all rooms
        roomCentroids[i][0] = dungeon.getRooms()[i].posX + dungeon.getRooms()[i].width / 2;
        roomCentroids[i][1] = dungeon.getRooms()[i].posY + dungeon.getRooms()[i].height / 2;
    }

    // find smallest distance
    for (int i = 1; i < dungeon.getNumRooms(); ++i)
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
                if (validRoomPositionFound) {
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
    if (dungeon.getPC().getFloorsVisited() % 3 == 0 &&
        dungeon.getPC().getFloorsVisited() != 0) { // a shop appears every 3 floors
        generateShop();
    }
    generateLiquids();
}

void resetDungeonLevel(void) {
    // Clear dungeon data
    wandUsedCoords.clear();
    memset(dungeon.getPC().getFogMap(), false, sizeof(dungeon.getPC().getFogMap()));  // Set all values to false
    memset(dungeon.getItemMap(), false, sizeof(dungeon.getItemMap()));
    // Reset counters and global variables
    dungeon.setNumRooms(MIN_NUM_ROOMS);
    dungeon.setRooms(vector<room_t>(dungeon.getNumRooms()));
    dungeon.setUpwardStairs(vector<stair_t>(3));
    dungeon.setDownwardStairs(vector<stair_t>(3));
    dungeon.setNumUpwardsStairs(0);
    dungeon.setNumDownwardsStairs(0);
    dungeon.getPC().setPreviousCell(ROOM_CELL);
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
    turnMessage = "Your turn to move!";
}

// Constructor implementation
Dungeon::Dungeon()
: numRooms(0), pc(0, 0, ROOM_CELL, PC_SPEED, PLAYER_CELL, 100, "0+1d4", {}, false, UP, {},  0, 0, 0, 0, 0, 0, 0, "", 0, 0), numUpwardsStairs(0), numDownwardsStairs(0), numMonsters(0) {
}

// Getter for map
vector<Item> (&Dungeon::getItemMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return itemMap;
}

// Getter for map
cell_t (&Dungeon::getMap())[DUNGEON_HEIGHT][DUNGEON_WIDTH] {
    return map;
}

// Getter for rooms
vector<Item>& Dungeon::getItems() {
    return items;
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

void Dungeon::setItems(vector<Item> newItems) {
    items = newItems;
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

vector<Item>& Dungeon::getShopItems() {
    return shopItems;
}
