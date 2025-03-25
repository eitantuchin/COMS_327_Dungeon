#include "../headers/dungeon.hpp"
#include "../headers/dungeon_game.hpp"
#include "../headers/monster.hpp"
#include "../headers/pc.hpp"
#include "../headers/character.hpp"
#include "../headers/priority_queue.h"
#include "../headers/parse.hpp"
#include "../headers/item.hpp"
#include <array>
#include <string>
#include <cstring>
#include <ncurses.h>
#include <sstream>
#include <cmath>
#include <memory>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <ctime>

using namespace std;

// Global variables
Dungeon dungeon;
my_priority_queue event_queue;
bool gameOver = false;
bool fogOfWar = true;
bool playerToMove = true;
bool cheatsEnabled = false;
item_select_t choosingCarryItem = NO_SELECT;
bool choosingEquipmentItem = false;
string turnMessage = "";
string gameMessage = "";
string directionMessage = "";
string dirNames[8] = {
    "NORTH", "NORTH-EAST", "EAST", "SOUTH-EAST", "SOUTH", "SOUTH-WEST", "WEST", "NORTH-WEST"
};
int scrollOffset = 0;
cell_t targetingPointerPreviousCell = {' ', 0};
const cell_t IMMUTABLE_ROCK_CELL = {' ', 255};
const cell_t ROCK_CELL = {' ', 1};
const cell_t ROOM_CELL = {'.', 0};
const cell_t CORRIDOR_CELL = {'#', 0};
const cell_t UPWARD_STAIRS_CELL = {'<', 0};
const cell_t DOWNWARD_STAIRS_CELL = {'>', 0};
const cell_t PLAYER_CELL = {'@', 0};
const cell_t POINTER_CELL = {'*', 0};
const cell_t SHOP_CELL = {'%', 0};
vector<string> invalidItemsAndMonsters = {};
int selectedMonsterIndex = -1;
Item randShopItem(0, 0, "", "", "", {}, "", 0, 0, 0, 0, 0, "", 0, false, 0, {}, false, false);
int turnsPassed = 0;
potion_type_t potionInUse = NO_POTION;

/*
 Allows the user to choose between saving, loading, and creating dungeons
 */
int main(int argc, char *argv[])
{
    // Initialize dungeon
    dungeon.setNumRooms(MIN_NUM_ROOMS);
    dungeon.setRooms(vector<room_t>(MAX_NUM_ROOMS));
    dungeon.setUpwardStairs(vector<stair_t>(3));
    dungeon.setDownwardStairs(vector<stair_t>(3));
    dungeon.setModeType(PLAYER_CONTROL);
    dungeon.getPC().setSpeed(PC_SPEED);
    dungeon.getPC().setHealth(PC_HEALTH);
    srand((unsigned int) time(NULL));
    
    // Make a dungeon but no saving or loading
    if (argv[1] == NULL) {
        int numMonsters = rand() % (10 - MIN_NUM_MONSTERS + 1) + MIN_NUM_MONSTERS;
        dungeon.setNumMonsters(numMonsters);
        dungeon.setMonsters(vector<Monster>(dungeon.getNumMonsters()));
        generateDungeon();
    }
    else if (strcmp(argv[1], "--nummon") == 0 && argv[2] != NULL) {
        int numMonsters = atoi(argv[2]);
        dungeon.setNumMonsters(numMonsters);
        dungeon.setMonsters(vector<Monster>(dungeon.getNumMonsters()));
        generateDungeon();
    }
    else if (strcmp(argv[1], "--nummon") == 0 && argv[2] == NULL) {
        printf("When using --nummon switch you must provide an integer amount for monsters like --nummon x, where x is an integer.\n");
        return 0;
    }
    else if (strcmp(argv[1], "--parse") == 0 && strcmp(argv[2], "-m") == 0) {
        readMonsters(true);
        return 0;
    }
    else if (strcmp(argv[1], "--parse") == 0 && strcmp(argv[2], "-o") == 0) {
        readObjects(true);
        return 0;
    }
    else if (strcmp(argv[1], "--parse") == 0 && argv[2] == NULL) {
        printf("When using --parse switch you must provide chars o or m, o for objects descriptions and m for monster descriptions. Usage is like --parse -o or --parse -m.\n");
        return 0;
    }
    
    else {
        printf("Unsupported command configuration: Please use either --nummon, --parse {-o or -m}, or no switch.\n");
        return 0;
    }
    
    printf("Welcome to the Dungeon Game!\n");
    printf("The name of this brave adventurer is: ");
    char playerName[51];
    scanf(" %50[^\n]s", playerName);
    dungeon.getPC().setName(string(playerName));
    printf("Greetings, %s! You are ready to embark on your adventure.\n", playerName);
    printf("Would you like to enable cheats (fog of war, teleport)? (y/n): ");
    char cheatChoice;
    scanf(" %c", &cheatChoice);
    if (cheatChoice == 'y' || cheatChoice == 'Y') {
        cheatsEnabled = true;
        printf("Cheats enabled (no leaderboard for you)! Have mercy on those monsters! \n");
    }
    else {
        cheatsEnabled = false;
        printf("Cheats disabled. Good luck, adventurer!\n");
    }
    printf("Press Enter to start the game...\n");
    getchar();
    getchar(); // Wait for Enter key
    
    // Initialize ncurses
    initscr(); // Start ncurses mode
    cbreak();  // Disable line buffering
    noecho();  // Don't display typed characters
    keypad(stdscr, TRUE);
    curs_set(0);  // Hide cursor
    nodelay(stdscr, TRUE);
    start_color();
    
    // Define color pairs (foreground, background)
    init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    // If foreground is BLACK, set it to WHITE for visibility
    init_pair(COLOR_BLACK, COLOR_WHITE, COLOR_BLACK);

    calculateDistances(0);  // Non-tunneling
    calculateDistances(1);  // Tunneling
    calculateDistances(2);  // Non-tunneling pass

    // Initialize event queue
    event_queue = my_priority_queue();

    turnMessage = "Your turn to move!";
    directionMessage = "Facing: NORTH";
    
    // Schedule initial PC event
    scheduleEvent(EVENT_PC, -1, 0);

    // Schedule initial monster events
    for (int i = 0; i < dungeon.getNumMonsters(); i++) {
        scheduleEvent(EVENT_MONSTER, i, 0);
    }

    // Main game loop
    gameOver = false;
    while (!gameOver) {
        checkKeyInput();
        if (dungeon.getModeType() == MONSTER_LIST) {
            displayMonsterList();
        }
        else if (dungeon.getModeType() == ITEM_MENU) {
            displayItemMenu();
        }
        else if (dungeon.getModeType() == INVENTORY) {
            displayInventory();
        }
        else if (dungeon.getModeType() == EQUIPMENT) {
            displayEquipment();
        }
        else if (dungeon.getModeType() == MONSTER_DETAILS) {
            displayMonsterDetails(selectedMonsterIndex);
        }
        else if (dungeon.getModeType() == STATS) {
            displayStats();
        }
        else if (dungeon.getModeType() == SETTINGS) {
            displaySettings();
        }
        else if (dungeon.getModeType() == SHOP) {
            displayShop(randShopItem);
        }
        else if (dungeon.getModeType() == PLAYER_CONTROL || dungeon.getModeType() == PLAYER_TELEPORT || dungeon.getModeType() == LOOK_AT_MONSTER) {
            printGame(0);
            refresh();
        }
        checkGameConditions();
        if (!playerToMove) processEvents();
        updateFogMap();
    }
    endwin();
    showLeaderboard();
    return 0;
}

bool compareEntries(const leaderboardEntry_t& a, const leaderboardEntry_t& b) {
    return a.score > b.score;
}

void showLeaderboard(void) {
    PC& pc = dungeon.getPC();
    int score = pc.getSpeed() + pc.getDamageDealt() + pc.getFloorsVisited() +
        pc.getNumItemsPickedUp() + pc.getDistanceTraveled() + pc.getMonstersKilled() + pc.getCoins();

    // Get the current time
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_r(&now, timeStr);
    timeStr[24] = '\0';
    string timestamp = string(timeStr);

    // Get the home directory
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        printf("Error: Could not determine HOME directory.\n");
        return;
    }

    // Construct the directory and file path
    string dirPath = string(homeDir) + "/.rlg327";
    string filePath = dirPath + "/leaderboard.txt";

    // Check if directory exists, create it if not
    if (access(dirPath.c_str(), F_OK) != 0) {
        if (mkdir(dirPath.c_str(), 0700) != 0) {
            printf("Error: Could not create directory %s.\n", dirPath.c_str());
            return;
        }
    }

    // Write the player's name, score, and timestamp to the file (name in quotes)
    ofstream outFile(filePath, ios::app);
    if (!outFile.is_open()) {
        printf("Error: Could not open or create %s for writing.\n", filePath.c_str());
        return;
    }
    if (!cheatsEnabled) {
        outFile << "\"" << pc.getName() << "\" " << score << " \"" << timestamp << "\"\n";
        outFile.close();
    }

    // Read the leaderboard file
    vector<leaderboardEntry_t> entries;
    ifstream inFile(filePath);
    if (inFile.is_open()) {
        string line;
        while (getline(inFile, line)) {
            string name, timestamp;
            int fileScore;

            // Use stringstream to parse the line
            stringstream iss(line);
            char quote;

            // Extract quoted name
            iss >> quote;
            if (quote != '"') continue;
            getline(iss, name, '"');

            // Extract score
            iss >> fileScore;

            // Extract quoted timestamp
            iss >> quote;
            if (quote != '"') continue;
            getline(iss, timestamp, '"');

            // Add to entries if parsing succeeded
            if (!name.empty() && iss.good()) {
                entries.push_back({name, fileScore, timestamp});
            }
        }
        inFile.close();
    }
    else {
        if (!cheatsEnabled) entries.push_back({pc.getName(), score, timestamp});
    }

    // Sort entries by score (descending)
    sort(entries.begin(), entries.end(), compareEntries);

    // Display the leaderboard
    printf("\n--- Game Leaderboard ---\n");
    printf("Top 5 Scores:\n");
    int displayCount = min(5, (int)entries.size());
    for (int i = 0; i < displayCount; i++) {
        printf("%d. %s - %d (%s)\n", i + 1, entries[i].name.c_str(), entries[i].score, entries[i].timestamp.c_str());
    }
    printf("-----------------------\n");
    if (cheatsEnabled) printf("\e[1mScore not counted on leaderboard since you enabled cheats.\e[1m\n");
    printf("Your Score: %s - %d (%s)\n", pc.getName().c_str(), score, timestamp.c_str());
    printf("Thanks for playing!\n");
}

void checkKeyInput(void) {
    int key = getch();
    switch (key) {
        case KEY_UP: case 'k': case KEY_DOWN: // move down
        case '2': case 'j': case KEY_LEFT: // move left
        case '4': case 'h': case KEY_RIGHT: // move right
        case '6': case 'l': case KEY_HOME: // up-left
        case '7': case 'y': case KEY_PPAGE: // up-right
        case '9': case 'u': case KEY_NPAGE: // down-right
        case '3': case 'n': case KEY_END: // down-left
        case '1': case 'b': case KEY_B2: // rest
        case '5': case ' ': case '.':
        case '0': case '8': case 'a': case 'c':
            if (choosingCarryItem == WEAR_ITEM) {
                wearItem(key);
            }
            else if (choosingCarryItem == DROP_ITEM) { // for dropping an item
                dropItem(key);
            }
            else if (choosingCarryItem == EXPUNGE_ITEM) { // for expunging an item
                expungeItem(key);
            }
            else if (choosingCarryItem == INSPECT_ITEM) {
                inspectItem(key);
            }
            else if (choosingCarryItem == USE_ITEM) {
                useItem(key);
            }
            else if (choosingEquipmentItem) { // for taking off an item
                takeOffItem(key);
            }
            else if (dungeon.getModeType() == DISTANCE_MAPS && (key == '1' || key == '2' || key == '3')) {
                clear();
                printGame(key);
            }
            else if (playerToMove && dungeon.getModeType() == PLAYER_CONTROL) movePlayer(key);
            else if (playerToMove && (dungeon.getModeType() == PLAYER_TELEPORT || dungeon.getModeType() == LOOK_AT_MONSTER)) {
                moveTargetingPointer(key);
            }
            else if (dungeon.getModeType() == MONSTER_LIST || dungeon.getModeType() == ITEM_MENU || dungeon.getModeType() == SETTINGS || dungeon.getModeType() == STATS || dungeon.getModeType() == SHOP) {
                clear();
                if (key == KEY_UP) scrollOffset--;
                else if (key == KEY_DOWN) scrollOffset++;
            }
            break;
        case 'q': // quit the game
        case 'Q':
            endwin();
            printf("\nYou quit the game!\n");
            printf(" _____                 _ _\n");
            printf("|  __ \\               | | |\n");
            printf("| |  \\/ ___   ___   __| | |__  _   _  ___\n");
            printf("| | __ / _ \\ / _ \\ / _` | '_ \\| | | |/ _ \\\n");
            printf("| |_\\ \\ (_) | (_) | (_| | |_) | |_| |  __/\n");
            printf(" \\____/\\___/ \\___/ \\__,_|_.__/ \\__, |\\___|\n");
            printf("                                __/ |\n");
            printf("                               |___/\n");
            gameOver = true;
            break;
        case 'V': // counter-clockwise
            changeDirection(false, false); break;
        case 'v': // clocckwise
            changeDirection(true, false); break;
        case '<':
        case '>':
            useStairs(key); break;
        case 'm':
            if (playerToMove && dungeon.getModeType() == PLAYER_CONTROL) {
                clear();
                dungeon.setModeType(MONSTER_LIST);
                scrollOffset = 0;
            }
            else {
                gameMessage = "You must be in player control mode to see the monster list!";
            }
            break;
        case 27: // escape key
            if (dungeon.getModeType() == MONSTER_LIST || dungeon.getModeType() == ITEM_MENU || choosingCarryItem != NO_SELECT || choosingEquipmentItem || dungeon.getModeType() == LOOK_AT_MONSTER) {
                clear();
                scrollOffset = 0;
                choosingCarryItem = NO_SELECT;
                choosingEquipmentItem = false;
                if (dungeon.getModeType() == LOOK_AT_MONSTER) {
                    pair<int, int> pointerPos = getPointerCellPosition();
                    if (pointerPos.first != -1 && pointerPos.second != -1) {
                        dungeon.getMap()[pointerPos.second][pointerPos.first] = targetingPointerPreviousCell;
                    }
                    selectedMonsterIndex = -1; // Reset selection
                }
                dungeon.setModeType(PLAYER_CONTROL);
                turnMessage = "Your turn to move!";
            }
            break;
        case 'f':
            if (choosingEquipmentItem) {
                takeOffItem(key);
            }
            else if (dungeon.getModeType() == PLAYER_CONTROL) {
                if (cheatsEnabled) fogOfWar = !fogOfWar; // only can be toggled with cheats
            }// fog of war switch
            else {
                gameMessage = "Cannot use f key when not in player control mode!";

            }
            break;
        case 'g': // goto/teleport mode activated
            if (choosingEquipmentItem) {
                takeOffItem(key);
            }
            else if (dungeon.getModeType() == PLAYER_TELEPORT) {
                teleportPlayer(false);
                dungeon.setModeType(PLAYER_CONTROL);
            }
            else if (dungeon.getModeType() == PLAYER_CONTROL){
                if (cheatsEnabled) {
                    dungeon.setModeType(PLAYER_TELEPORT);
                    initTargetingPointer();
                    gameMessage = "Use arrows to move pointer, [g] to teleport to pointer, [r] to teleport randomly.";
                }
            }
            else {
                gameMessage = "You must have player control to use the GOTO command!";
            }
            break;
        case 'r':
            if (dungeon.getModeType() == PLAYER_TELEPORT) {
                teleportPlayer(true);
                dungeon.setModeType(PLAYER_CONTROL);
            }
            break;
        case 'H': // display hardness map
            clear();
            if (dungeon.getModeType() == HARDNESS_MAP) {
                turnMessage = "Your turn to move!";
                dungeon.setModeType(PLAYER_CONTROL);
            }
            else {
                gameMessage = "Rock hardness map. Use H to return to player control.";
                dungeon.setModeType(HARDNESS_MAP);
                printGame(key);
            }
            break;
        case 'D': // display distance map
            clear();
            if (dungeon.getModeType() == DISTANCE_MAPS) {
                turnMessage = "Your turn to move!";
                dungeon.setModeType(PLAYER_CONTROL);
            }
            else {
                gameMessage = "Use keys [1] Non-Tunneling Map [2] Tunneling Map [3] Pass Wall Map [D] return";
                dungeon.setModeType(DISTANCE_MAPS);
                printGame(key);
            }
            break;
        case ',':
            // Show item menu for the player choose from
            if(!dungeon.getItemMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()].empty() && playerToMove && dungeon.getModeType() == PLAYER_CONTROL) {
                clear();
                dungeon.setModeType(ITEM_MENU);
                scrollOffset = 0;
            }
            else {
                gameMessage = "No items here! You also must be in player control!";
            }
            
            break;
        case 10: // Enter key
            if (dungeon.getModeType() == ITEM_MENU) {
                pickupItem();
                clear();
                dungeon.setModeType(PLAYER_CONTROL);
                scrollOffset = 0; // Reset after pickup
            }
            else if (dungeon.getModeType() == SHOP) {
                buyItem();
            }
            break;
        case 'i':
            if (choosingEquipmentItem) {
                takeOffItem(key);
            }
            else if (dungeon.getModeType() == INVENTORY && choosingCarryItem == NO_SELECT) {
                clear();
                turnMessage = "Your turn to move!";
                dungeon.setModeType(PLAYER_CONTROL);
            }
            else {
                clear();
                dungeon.setModeType(INVENTORY);
            }
            break;
        case 'w':
            if (dungeon.getModeType() == PLAYER_CONTROL && playerToMove) {
                clear();
                choosingCarryItem = WEAR_ITEM;
                dungeon.setModeType(INVENTORY);
            }
            else {
                gameMessage = "Can't wear an item in this mode. Go back to player control.";
            }
            break;
        case 'x':
            if (dungeon.getModeType() == PLAYER_CONTROL && playerToMove) {
                clear();
                choosingCarryItem = EXPUNGE_ITEM;
                dungeon.setModeType(INVENTORY);
            }
            else {
                gameMessage = "Can't expunge an item in this mode. Go back to player control.";
            }
            break;
        case 'd':
            if (choosingEquipmentItem) {
                takeOffItem(key);
            }
            else if (dungeon.getModeType() == PLAYER_CONTROL && playerToMove) {
                clear();
                choosingCarryItem = DROP_ITEM;
                dungeon.setModeType(INVENTORY);
            }
            else {
                gameMessage = "Can't drop an item in this mode. Go back to player control.";
            }
            break;
        case 'I':
            if (dungeon.getModeType() == PLAYER_CONTROL && playerToMove) {
                clear();
                choosingCarryItem = INSPECT_ITEM;
                dungeon.setModeType(INVENTORY);
            }
            else {
                gameMessage = "Can't inspect an item in this mode. Go back to player control.";
            }
            break;
        case 'e':
            if (choosingEquipmentItem) {
                takeOffItem(key);
            }
            else if (dungeon.getModeType() == EQUIPMENT && choosingEquipmentItem == false) {
                clear();
                turnMessage = "Your turn to move!";
                dungeon.setModeType(PLAYER_CONTROL);
            }
            else {
                clear();
                dungeon.setModeType(EQUIPMENT);
            }
            break;
        case 'U':
            if (dungeon.getModeType() == PLAYER_CONTROL && playerToMove) {
                clear();
                choosingCarryItem = USE_ITEM;
                dungeon.setModeType(INVENTORY);
            }
            else {
                gameMessage = "Can't use an item in this mode. Go back to player control.";
            }
            break;
        case 't':
            if (dungeon.getModeType() == MONSTER_DETAILS) {
                clear();
                dungeon.setModeType(LOOK_AT_MONSTER);
            }
            else if (dungeon.getModeType() == LOOK_AT_MONSTER) {
                int monsterIndex = getMonsterAtPointer();
                if (monsterIndex != -1) {
                    selectedMonsterIndex = monsterIndex;
                    clear();
                    dungeon.setModeType(MONSTER_DETAILS);
                }
                else {
                    gameMessage = "No visible monster at the pointer location!";
                    clear();
                    pair<int, int> pointerPos = getPointerCellPosition();
                    if (pointerPos.first != -1 && pointerPos.second != -1) {
                        dungeon.getMap()[pointerPos.second][pointerPos.first] = targetingPointerPreviousCell;
                    }
                    dungeon.setModeType(PLAYER_CONTROL);
                }
            }
            else if (dungeon.getModeType() == PLAYER_CONTROL && playerToMove) {
                clear();
                choosingEquipmentItem = true;
                dungeon.setModeType(EQUIPMENT);
            }
            else {
                gameMessage = "Can't take off an item in this mode. Go back to player control.";
            }
            break;
        case 'L':
            if (dungeon.getModeType() == PLAYER_CONTROL && playerToMove) {
                clear();
                dungeon.setModeType(LOOK_AT_MONSTER);
                initTargetingPointer();
                gameMessage = "Move pointer with movement keys, press 't' to select monster, ESC to abort.";
            }
            else {
                gameMessage = "Can't look at a monster now. Return to player control mode.";
            }
            break;
        case 's':
            if (dungeon.getModeType() == PLAYER_CONTROL && playerToMove) {
                clear();
                dungeon.setModeType(STATS);
                scrollOffset = 0;
            }
            else if (dungeon.getModeType() == STATS) {
                turnMessage = "Your turn to move!";
                dungeon.setModeType(PLAYER_CONTROL);
                scrollOffset = 0;

            }
            else {
                gameMessage = "Please return to player control to look at your stats.";
            }
            break;
        case 'S':
            if (dungeon.getModeType() == SETTINGS) {
                turnMessage = "Your turn to move!";
                dungeon.setModeType(PLAYER_CONTROL);
                scrollOffset = 0;
            }
            else { // we can always go to settings
                clear();
                dungeon.setModeType(SETTINGS);
                scrollOffset = 0;
            }
            break;
        case 'T':            
            if (dungeon.getModeType() == SHOP) {
                clear();
                turnMessage = "Your turn to move!";
                dungeon.setModeType(PLAYER_CONTROL);
                scrollOffset = 0;
            }
            else if (playerToMove && dungeon.getModeType() == PLAYER_CONTROL && dungeon.getPC().getPreviousCell().ch == '%') {
                clear();
                dungeon.setModeType(SHOP);
                scrollOffset = 0;
            }
            else {
                gameMessage = "Not currently next to a shopkeeper!";
            }
            break;
    }
}

vector<string> inputDescriptions = {
    "Movement (Player Control Mode):",
    "  UP, k, 8: Move up",
    "  DOWN, j, 2: Move down",
    "  LEFT, h, 4: Move left",
    "  RIGHT, l, 6: Move right",
    "  HOME, y, 7: Move up-left",
    "  PGUP, u, 9: Move up-right",
    "  END, b, 1: Move down-left",
    "  PGDN, n, 3: Move down-right",
    "  SPACE, ., 5: Rest",
    "Targeting Pointer (Teleport/Look Mode):",
    "  Same as movement keys: Move pointer",
    "Menu Navigation:",
    "  UP: Scroll up (Monster List, Item Menu, Settings)",
    "  DOWN: Scroll down (Monster List, Item Menu, Settings)",
    "Game Actions:",
    "  q, Q: Quit game",
    "  V: Turn counter-clockwise",
    "  v: Turn clockwise",
    "  <: Use upward stairs",
    "  >: Use downward stairs",
    "  m: View monster list",
    "  ESC: Exit current mode (for some)",
    "  f: Toggle fog of war",
    "  g: Enter teleport mode / Teleport to pointer",
    "  r: Random teleport (Teleport mode)",
    "  H: Toggle hardness map",
    "  D: Toggle distance maps",
    "  1, 2, 3: Show distance map (Distance Maps mode)",
    "  ,: Open item menu (if items present)",
    "  Enter: Pick up item (Item Menu), Buy item (Shop)",
    "  i: View inventory",
    "  w: Wear item (from Inventory)",
    "  x: Expunge item (from Inventory)",
    "  d: Drop item (from Inventory)",
    "  I: Inspect item (from Inventory)",
    "  e: View equipment",
    "  t: Take off item (Equipment) / Select monster in lookup mode",
    "  L: Look at monster mode",
    "  s: Toggle stats screen",
    "  S: Toggle settings screen",
    "  U: Use item (from Inventory)",
    "  T: Talk to shopkeeper (when next to shop)",
    "Inventory/Equipment Selection:",
    "  0-9, a-c: Select slot for actions"
};

// Updated displaySettings method
void displaySettings(void) {
    int screenHeight, screenWidth;
    getmaxyx(stdscr, screenHeight, screenWidth);
    
    const char* title = "Settings - Keybindings";
    size_t titleLength = strlen(title);
    size_t titleX = (screenWidth - titleLength) / 2;
    if (titleX < 0) titleX = 1;
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(0, (int) titleX, "%s", title);
    attroff(COLOR_PAIR(COLOR_CYAN));
    // Ensure the scroll offset is within valid bounds
    // Instructions
    attron(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(LINES - 1, 1, "Use UP/DOWN to scroll, [S] to exit");
    attroff(COLOR_PAIR(COLOR_YELLOW));
    
    if (scrollOffset < 0) {
        scrollOffset = 0;
    }
    int maxLines = screenHeight - 3; // Leave space for title and instructions
    int maxScrollOffset = (int) inputDescriptions.size() - maxLines;
    if (scrollOffset > maxScrollOffset) {
        scrollOffset = maxScrollOffset;
    }
    // Display input descriptions with scrolling
    for (int i = 0; i < maxLines && (i + scrollOffset) < (int) inputDescriptions.size(); i++) {
        int index = i + scrollOffset;
        mvprintw(i + 1, 1, "%s", inputDescriptions[index].c_str());
    }
    refresh();
}


void initTargetingPointer(void) {
    int pcX = dungeon.getPC().getPosX();
    int pcY = dungeon.getPC().getPosY();
    bool pointerPlaced = false;

    for (int y = pcY - 1; y <= pcY + 1 && !pointerPlaced; ++y) {
        for (int x = pcX - 1; x <= pcX + 1; ++x) {
            if (dungeon.getMap()[y][x].hardness != IMMUTABLE_ROCK_CELL.hardness && x != pcX && y != pcY) {
                targetingPointerPreviousCell = dungeon.getMap()[y][x];
                dungeon.getMap()[y][x] = POINTER_CELL;
                pointerPlaced = true;
                break;
            }
        }
    }
    refresh();
}

pair<int, int> getPointerCellPosition(void) {
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            if (dungeon.getMap()[y][x].ch == POINTER_CELL.ch) {
                return {x, y};
            }
        }
    }
    return {-1, -1};
}

void moveTargetingPointer(int key) {
    pair<int, int> pointerPos = getPointerCellPosition();
    int newX = pointerPos.first;
    int newY = pointerPos.second;
    int oldX = newX;
    int oldY = newY;
    switch (key) {
        case KEY_UP: case '8': case 'k':
            newY--; break;
        case KEY_DOWN: case '2': case 'j':
            newY++;  break;
        case KEY_LEFT: case '4': case 'h':
            newX--;  break;
        case KEY_RIGHT: case '6': case 'l':
            newX++;  break;
        case KEY_HOME: case '7': case 'y':
            newX--; newY--; break;
        case KEY_PPAGE: case '9': case 'u':
            newX++; newY--; break;
        case KEY_NPAGE: case '3': case 'n':
            newX++; newY++; break;
        case KEY_END: case '1': case 'b':
            newX--; newY++; break;
    }
    // as long as it's not immutable rock we can move the pointer there
    if (dungeon.getMap()[newY][newX].hardness != IMMUTABLE_ROCK_CELL.hardness) {
        dungeon.getMap()[oldY][oldX] = targetingPointerPreviousCell;
        targetingPointerPreviousCell = dungeon.getMap()[newY][newX];
        dungeon.getMap()[newY][newX] = POINTER_CELL;
    }
}


void updateFogMap(void) {
    PC& pc = dungeon.getPC();
    int lightRadius = 2; // if not holding any light then the default visibility dist is 2
    for (Item i: pc.getEquippedItems()) {
        if (i.getType() == "LIGHT") {
            lightRadius = rollDice(i.getAttribute());
        }
    }
    for (int y = pc.getPosY() - lightRadius; y <= pc.getPosY() + lightRadius; ++y) {
        for (int x = pc.getPosX() - lightRadius; x <= pc.getPosX() + lightRadius; ++x) {
            if (!pc.getFogMap()[y][x]) {
                pc.getFogMap()[y][x] = true;
            }
        }
    }
}

void printGame(int value) {
    int (*dist)[DUNGEON_WIDTH] = nullptr;
    if (value == '1') dist = dungeon.getNonTunnelingMap();
    else if (value == '2') dist = dungeon.getTunnelingMap();
    else if (value == '3') dist = dungeon.getNonTunnelingPassMap();
    move(1, 0); // Move cursor to the second row (leaving the first for the message)
    for (int y = 0; y < DUNGEON_HEIGHT; ++y) {
        move(y + 1, 0);
        for (int x = 0; x < DUNGEON_WIDTH; ++x) {
            switch (value) {
                case 0: // print regular dungeon
                    printCharacter(x, y);
                    break;
                case 'H':
                    if (y == dungeon.getPC().getPosY() && x == dungeon.getPC().getPosX()) {
                        addch('@');
                    }
                    else if (dungeon.getMap()[y][x].hardness == IMMUTABLE_ROCK_CELL.hardness) {
                        attron(COLOR_PAIR(COLOR_RED));
                        addch('X');
                        attroff(COLOR_PAIR(COLOR_RED));
                    }
                    else {
                        double hardness = static_cast<double>(dungeon.getMap()[y][x].hardness);
                        double result = ceil(hardness / 85.0);
                        short color = 0;
                        switch ((int) result) {
                            case 1: color = COLOR_YELLOW;    break;
                            case 2: color = COLOR_GREEN;   break;
                            case 3: color = COLOR_MAGENTA; break;
                        }
                        if (result != 0) {
                            attron(COLOR_PAIR(color));
                            addch(static_cast<int>(result) + '0');
                            attroff(COLOR_PAIR(color));
                        }
                        else {
                            addch(' ');
                        }
                       
                    }
                    break;
                case 'D':
                    break;
                case '1':
                case '2':
                case '3':
                    if (y == dungeon.getPC().getPosY() && x == dungeon.getPC().getPosX()) {
                        addch('@');
                    }
                    else if (dist[y][x] == INT_MAX || dist[y][x] ==  -1) {
                        addch(' ');
                    }
                    else {
                        int digit = dist[y][x] % 10;
                        addch(digit + '0');
                    }
                    break;
            }
        }
    }
    drawMessages();
}

void printCharacter(int x, int y) {
    // Static variables for color cycling
    static struct timespec lastUpdate = {0, 0};
    static int colorIndex = 0;
    
    // Get current time
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (lastUpdate.tv_sec == 0) { // First run
        lastUpdate = now;
    }

    // Check if 0.5 seconds have passed
    double elapsed = (now.tv_sec - lastUpdate.tv_sec) +
                     (now.tv_nsec - lastUpdate.tv_nsec) / 1e9;
    if (elapsed >= 0.5) { // 0.5-second delay
        colorIndex++; // Move to next color
        lastUpdate = now;
    }
    
    short color = 0;
    // check for item
    vector<Item> items = dungeon.getItemMap()[y][x];
    if (!items.empty() && dungeon.getMap()[y][x].hardness == -2) {
        color = items[items.size() - 1].getColor()[0];
    }
    // check for monster
    for (Monster m: dungeon.getMonsters()) {
        if (m.getPosY() == y && m.getPosX() == x && m.isAlive() && dungeon.getMap()[y][x].hardness == -1) {
            vector<short> colors = m.getColor();
            color = colors[colorIndex % colors.size()];
            break;
        }
    }
    if (dungeon.getMap()[y][x].ch == 'S' && dungeon.getMap()[y][x].hardness == -3) color = COLOR_MAGENTA; // for the shop
    attron(COLOR_PAIR(color));
    if (fogOfWar) {
        if (dungeon.getMap()[y][x].ch == '*')  addch(dungeon.getMap()[y][x].ch);
        else if (x <= dungeon.getPC().getPosX() + 2 && x >= dungeon.getPC().getPosX() - 2 &&
            y <= dungeon.getPC().getPosY() + 2 && y >= dungeon.getPC().getPosY() - 2) {
            addch(dungeon.getMap()[y][x].ch);
        }
        else {
            if (dungeon.getPC().getFogMap()[y][x] && dungeon.getMap()[y][x].hardness == -1) {
                // add previous character since monster but shouldn't be seen
                for (size_t i = 0; i < dungeon.getMonsters().size(); ++i) {
                    if (dungeon.getMonsters()[i].getPosX() == x && dungeon.getMonsters()[i].getPosY() == y) {
                        attron(COLOR_PAIR(COLOR_WHITE));
                        addch(dungeon.getMonsters()[i].getPreviousCell().ch);
                        attroff(COLOR_PAIR(COLOR_WHITE));

                    }
                }
            }
            else if (dungeon.getPC().getFogMap()[y][x]) {
                addch(dungeon.getMap()[y][x].ch);
            }
            else {
                addch(' '); // can't be seen
            }
        }
    }
    else {
        addch(dungeon.getMap()[y][x].ch);
    }
    attroff(COLOR_PAIR(color));
    refresh();
}


void scheduleEvent(event_type_t type, int index, int currentTurn) {
    unique_ptr<event_t> newEvent(new event_t);
    newEvent->type = type;
    newEvent->index = index;
    if (type == EVENT_PC) {
        int speedBoost = 0;
        if (potionInUse) turnsPassed++;
        if (turnsPassed == 20 && potionInUse == SPEED) {
            turnsPassed = 0;
            potionInUse = NO_POTION; // NO_POTION is
            gameMessage = "Speed potion ended!";
        }
        else if (turnsPassed < 20 && potionInUse == SPEED) {
            speedBoost = rollDice("10+5d4");
        }
        else if (turnsPassed == 35 && potionInUse == INVISIBILITY) {
            turnsPassed = 0;
            potionInUse = NO_POTION;
            gameMessage = "Invisibility potion ended!";
        }
        newEvent->turn = currentTurn + (1000 / (dungeon.getPC().getSpeed() + speedBoost));
    }
    else {
        newEvent->turn = currentTurn + (1000 / dungeon.getMonsters()[index].getSpeed());
    }
    event_queue.insert(newEvent->index, 0, newEvent->turn);
}

void processEvents(void) {
    pq_node_t node = event_queue.extract_min(); // extract the first value in the Q
    if (node.x == -1) { // EVENT_PC
        playerToMove = true; // this stops the processing of events until the player moves again
        scheduleEvent(EVENT_PC, -1, node.priority);
        turnMessage = "Your turn to move!";
        if (!dungeon.getItemMap()[dungeon.getPC().getPosY()][dungeon.getPC().getPosX()].empty()) {
            gameMessage = "There are items here! Press [,] to checkout the items.";
        }
    }
    else if (dungeon.getMonsters()[node.x].isAlive()) {
        moveMonster(node.x);
        scheduleEvent(EVENT_MONSTER, node.x, node.priority); // keep moving the monster
        turnMessage = "The monsters are moving...";
    }
}

void drawMessages(void) {
    move(0, 0);
    clrtoeol();
    // game message
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(0, 1, "%s", gameMessage.c_str());
    attroff(COLOR_PAIR(COLOR_CYAN));
    
    move(22, 0);
    clrtoeol();
    // Health
    attron(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(22, 1, "Health: ");
    attroff(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(22, 9, "%d", dungeon.getPC().getHealth());
    
    // Speed
    attron(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(22, 15, "Speed: ");
    attroff(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(22, 22, "%d", dungeon.getPC().getSpeed());
    
    attron(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(22, 28, "Coins: ");
    attroff(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(22, 35, "%d", dungeon.getPC().getCoins());
    
    switch (potionInUse) {
        case INVISIBILITY:
            attron(COLOR_PAIR(COLOR_BLUE));
            mvprintw(22, 40, "Using INVISIBILITY Pot");
            attroff(COLOR_PAIR(COLOR_BLUE));
            break;
        case SPEED:
            attron(COLOR_PAIR(COLOR_BLUE));
            mvprintw(22, 40, "Using SPEED Pot");
            attroff(COLOR_PAIR(COLOR_BLUE));
            break;
        case HEALING:
            attron(COLOR_PAIR(COLOR_BLUE));
            mvprintw(22, 40, "Using HEALING Pot");
            attroff(COLOR_PAIR(COLOR_BLUE));
            break;
        case NO_POTION:
            attron(COLOR_PAIR(COLOR_RED));
            mvprintw(22, 40, "No potion in use");
            attroff(COLOR_PAIR(COLOR_RED));
            break;
    }
    
    attron(COLOR_PAIR(COLOR_MAGENTA));
    mvprintw(22, 63, "[S] for settings");
    attroff(COLOR_PAIR(COLOR_MAGENTA));
    
    move(23, 0);
    clrtoeol();
    // Turn message
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(23, 1, "%s", turnMessage.c_str());
    attroff(COLOR_PAIR(COLOR_CYAN));
    
    // Coords
    attron(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(23, 35, "Position: ");
    attroff(COLOR_PAIR(COLOR_YELLOW));
    mvprintw(23, 45, "[X: %d, Y: %d]", dungeon.getPC().getPosX(), dungeon.getPC().getPosY());
    
    // Direction message
    size_t len = strlen(directionMessage.c_str());
    size_t x = DUNGEON_WIDTH - len - 1; // Calculate starting x for right alignment
    attron(COLOR_PAIR(COLOR_GREEN));
    mvprintw(23, (int) x, "%s", directionMessage.c_str()); // Print right-aligned
    attroff(COLOR_PAIR(COLOR_GREEN));
}

pair<int, int> getMinAndMaxDamage(void) {
    int maxDamage = 0, minDamage = 0;
    bool weaponEquipped = false;
    for (Item i: dungeon.getPC().getEquippedItems()) {
        if (i.getType() == "WEAPON") {
            weaponEquipped = true;
        }
        // calculate min and max damage for each item equipped and add to totals
        string damStr = i.getDamage();
        int base = 0, dice = 0, sides = 0;
        char d; // To hold 'd' character
        stringstream ss(damStr);
        if (damStr.find('+') != string::npos) {
            ss >> base >> d >> dice >> d >> sides;
        } else {
            ss >> dice >> d >> sides;
        }
        minDamage += (base + dice);
        maxDamage += (base + (dice * sides));
    }
    if (!weaponEquipped) { // just bare handed
        minDamage += 1;
        maxDamage += 4;
    }
    pair<int, int> ret;
    ret.first = minDamage;
    ret.second = maxDamage;
    return ret;
}

void checkGameConditions(void) {
    if (dungeon.getPC().getHealth() <= 0) {
        gameOver = true;
        endwin();
        printf("\nTo the grave you go...\n");
        printf("______ ___________\n");
        printf("| ___ \\_   _| ___ \\\n");
        printf("| |_/ / | | | |_/ /\n");
        printf("|    /  | | |  __/\n");
        printf("| |\\ \\ _| |_| |\n");
        printf("\\_| \\_|\\___/\\_|\n");
        printf("\nYou died! Better luck next time!\n");
    }
    
    // Check if a boss monster has been killed
    for (int i = 0; i < dungeon.getNumMonsters(); ++i) {
        if (!dungeon.getMonsters()[i].isAlive() && containsString(dungeon.getMonsters()[i].getAbilities(), string("BOSS"))) {
            gameOver = true;
            endwin();
            printf("\nTruly a remarkable player...\n");
            printf("__   _______ _   _   _    _  _____ _   _ _\n");
            printf("\\ \\ / /  _  | | | | | |  | ||  _  | \\ | | |\n");
            printf(" \\ V /| | | | | | | | |  | || | | |  \\| | |\n");
            printf("  \\ / | | | | | | | | |/\\| || | | | . ` | |\n");
            printf("  | | \\ \\_/ / |_| | \\  /\\  /\\ \\_/ / |\\  |_|\n");
            printf("  \\_/  \\___/ \\___/   \\/  \\/  \\___/\\_| \\_(_)\n");
            printf("\nCongratulations! You won since you defeated boss %s!\n", dungeon.getMonsters()[i].getName().c_str());
            break;
        }
    }
}

void calculateDistances(int mapNum) {
    // using the priority queue and dijikstra's algorithm
    my_priority_queue pq = my_priority_queue();
    int (*dist)[DUNGEON_WIDTH];
    if (mapNum == 0) dist = dungeon.getNonTunnelingMap();
    else if (mapNum == 1) dist = dungeon.getTunnelingMap();
    else dist = dungeon.getNonTunnelingPassMap();

    // Initialize distances
    for (int y = 0; y < DUNGEON_HEIGHT; y++) {
        for (int x = 0; x < DUNGEON_WIDTH; x++) {
            dist[y][x] = INT_MAX;
        }
    }

    // Start with PC position
    int pc_y = dungeon.getPC().getPosY();
    int pc_x = dungeon.getPC().getPosX();
    dist[pc_y][pc_x] = 0;
    pq.insert(pc_x, pc_y, 0);

    while (!pq.is_empty()) {
        pq_node_t node = pq.extract_min();
        int x = node.x;
        int y = node.y;
        
        
        cell_t currentCell = dungeon.getMap()[y][x];

        // Check all 8 neighbors
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                int nx = x + dx;
                int ny = y + dy;

                // Bounds check
                if (nx < 0 || nx >= DUNGEON_WIDTH || ny < 0 || ny >= DUNGEON_HEIGHT) continue;

                // Check passability
                cell_t neighborCell = dungeon.getMap()[ny][nx];
                if (mapNum == 0 && neighborCell.hardness > 0) continue;  // Non-tunneler non-pass can't pass through rock
                if (neighborCell.hardness == 255) continue;             // Immutable rock

                // Calculate weight
                int weight = 1;
                if (mapNum == 1 && currentCell.hardness > 0) { // tunneler meets rock
                    weight = 1 + (currentCell.hardness / 85);
                }

                // Update distance
                int new_dist = dist[y][x] + weight;
                if (new_dist < dist[ny][nx]) {
                    dist[ny][nx] = new_dist;
                    pq.insert(nx, ny, new_dist);
                }
            }
        }
    }
    pq.clear();
}

bool containsInt(const vector<int>& array, const int& value) {
    for (const int& element: array) {
        if (element == value) {
            return true;
        }
    }
    return false;
}

bool containsString(const vector<string>& array, const string& value) {
    for (const string& element: array) {
        if (element == value) {
            return true;
        }
    }
    return false;
}
