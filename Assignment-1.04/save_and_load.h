//
//  save_and_load.h
//  Dungeon_Game_327
//
//  Created by Eitan Tuchin on 1/30/25.
//

#ifndef save_and_load_h
#define save_and_load_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <arpa/inet.h>
#define FILE_MARKER "RLG327-S2025"

void saveFile(void);
void loadFile(void);

#endif
