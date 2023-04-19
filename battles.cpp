//                                   ,'\.
//     _.----.        ____         ,'  _\   ___    ___     ____
// _,-'       `.     |    |  /`.   \,-'    |   \  /   |   |    \  |`.
// \      __    \    '-.  | /   `.  ___    |    \/    |   '-.   \ |  |
//  \.    \ \   |  __  |  |/    ,','_  `.  |          | __  |    \|  |
//    \    \/   /,' _`.|      ,' / / / /   |          ,' _`.|     |  |
//     \     ,-'/  /   \    ,'   | \/ / ,`.|         /  /   \  |     |
//      \    \ |   \_/  |   `-.  \    `'  /|  |    ||   \_/  | |\    |
//       \    \ \      /       `-.`.___,-' |  |\  /| \      /  | |   |
//        \    \ `.__,'|  |`-._    `|      |__| \/ |  `.__,'|  | |   |
//         \_.-'       |__|    `-._ |              '-.|     '-.| |   |
//                                 `'                            '-._|
// `;-.          ___,
//   `.`\_...._/`.-"`
//     \        /      ,
//     /()   () \    .' `-._
//    |)  .    ()\  /   _.'
//    \  -'-     ,; '. <
//     ;.__     ,;|   > \.
//    / ,    / ,  |.-'.-'
//   (_/    (_/ ,;|.<`
//     \    ,     ;-`
//      >   \    /
//     (_,-'`> .'
//          (_,'

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <cstdlib>
#include <unistd.h>
#include <ncurses.h>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include "db_parse.h"
extern "C" {
    #include "heap.h"
}
using namespace std;

#define GRASS_COLOR 123
#define WATER_COLOR 234
#define BOULDER_COLOR 456
#define ROAD_COLOR 678
#define TREE_COLOR 890

const int INFINITY_T = 2147483640;
const int NUM_VERTICES = 1680; // 21 * 80
int playerNeedsPokemon = 1;

class position {
    public:
        int rowPos;
        int colPos;
};

typedef enum {
    None = 0, Up = 1, Down = 2, Left = 3, Right = 4
} direction_t;

class squares {
    public:
        int rowPos;
        int colPos;
        int cost;
        char terrain;
};

class worldPokemon: public position {
    public:
        char name[30];
        move_db pokeMoves[4];
        int gender;
        int shiny;
        int level;
        experience_db levels[101];
        int exp;
        int lvlRate;

        int pokeId;
        int speciesId;

        int initialHP;
        int hp;
        int attack;
        int defense;
        int specialAttack;
        int specialDefense;
        int speed;

        int hpIV;
        int attackIV;
        int defenseIV;
        int specialAttackIV;
        int specialDefenseIV;
        int speedIV;
};

class player: public position {
    public:
        int preset;
};

class character: public position {
    public:
        char npc;
        char value[20];
        direction_t direction;
        worldPokemon heldPokemon[6];
        int numPokemon;
        long int nextMoveTime;
        int defeated;
        char spawn;
};

class terrainMap {
    public:
        int quit;
        int wantToFly;
        int flyRow;
        int flyCol;
        int generated;
        player pc;
        int northSouthExit;
        int westEastExit;
        int worldRow;
        int worldCol;
        char terrain[21][80];
        position roadPositions[101]; // 80 + 21
        character *trainers[];
};

terrainMap *world[401][401];
direction_t lastMove;
int activePokemon;
worldPokemon heldPokemon[6];
int numPokemon = 1;
std::default_random_engine gen;
int pokeballs = 10;
int revives = 15;
int potions = 15;

int decorateTerrain(char map[21][80]) {
    int i, j, k;
    char decorations[2] = {'^', '%'};

    for (k = 0; k < (rand() % (20 - 10)) + 10; k++) {
        j = (rand() % (79 - 1)) + 1;
        i = (rand() % (20 - 1)) + 1;

        if (map[i][j] != '#' && map[i][j] != 'M' && map[i][j] != 'C' && map[i][j] != '~') {
            map[i][j] = decorations[rand() % 2];
        }
    }

    return 0;
}

int generateBuildings(terrainMap *terrainMap, int row, int col) {
    int pC = (rand() % (70 - 10)) + 10;
    int pM = (rand() % (16 - 3)) + 3;

    int i, j;

    while (terrainMap->terrain[terrainMap->westEastExit - 1][pC - 1] == '#' || terrainMap->terrain[terrainMap->westEastExit - 1][pC] == '#') {
        pC = (rand() % (70 - 10)) + 10;
    }

    while (terrainMap->terrain[pM - 1][terrainMap->northSouthExit - 1] == '#' || terrainMap->terrain[pM][terrainMap->northSouthExit - 1] == '#') {
        pM = (rand() % (16 - 3)) + 3;
    }

    for (i = 1; i < 3; i++) {
        for (j = 0; j < 2; j++) {
            terrainMap->terrain[terrainMap->westEastExit - i][pC - j] = 'C';
        }
    }

    for (i = 0; i < 2; i++) {
        for (j = 1; j < 3; j++) {
            terrainMap->terrain[pM - i][terrainMap->northSouthExit - j] = 'M';
        }
    }

    return 0;  
}

int generatePaths(terrainMap *terrainMap, int currWorldRow, int currWorldCol) {
    // Will need to update to check for existence of exits
    int i, j, k = 0;
    int rowStart = 0, colStart = 0;
    int rowEnd = 80, colEnd = 21;

    if (currWorldRow == 0) { // If at the bottom of the world
        colEnd = 20;
    }
    if (currWorldRow == 401) { // If at the top of the world
        colStart = 1;
    }
    if (currWorldCol == 0) {
        rowStart = 1;
    }
    if (currWorldCol == 401) {
        rowEnd = 79;
    }

    for (i = rowStart; i < rowEnd; i++) {
        terrainMap->terrain[terrainMap->westEastExit][i] = '#';
        terrainMap->roadPositions[k].colPos = i;
        terrainMap->roadPositions[k].rowPos = terrainMap->westEastExit;
        k++;
    }

    for (j = colStart; j < colEnd; j++) {
        terrainMap->terrain[j][terrainMap->northSouthExit] = '#';
        terrainMap->roadPositions[k].colPos = terrainMap->northSouthExit;
        terrainMap->roadPositions[k].rowPos = j;
        k++;
    }

    return 0;  
}

int checkSurroundingsForChar(int x, int y, char map[21][80], char checkChar) {
    int i, j;
    
    for (i = -7; i < 8; i++) {
        for (j = -7; j < 8; j++) {
            if (map[y+i][x+j] == checkChar || map[y+i][x+j] == '%' || map[y+i][x+j] == '#') {
                return 1;
            }
        }
    }

    return 0;
}

void generateWater(char map[21][80]) {  
    int x = rand() % 79 + 1;
    int y = rand() % 20 + 1;

    // Ensure that the random point to be used as the center of a monument is at least 5 squares away from any tall grass and
    // the borders of the map
    while(checkSurroundingsForChar(x, y, map, ':') == 1) {
        x = rand() % 79 + 1;
        y = rand() % 20 + 1;
    }

    int i, j;

    int height = (rand() % 7) + 3;
    int width = (rand() % 13) + 5;

    for (i = -height; i < height; i++) {
        for (j = -width; j < width; j++) {
            if (y+i >= 0 && y+i <= 20 && x+j >= 0 && x+j <= 79) {
                if (map[y + i][x + j] != '%' && map[y + i][x + j] != '#') {
                    map[y + i][x + j] = '~';
                }
            }
        }
    }
}

void generateTallGrass(char map[21][80]) {
    int i, j;
    int spotX = (rand() % 29) + 10;
    int spotY = (rand() % 10) + 5;

    while(checkSurroundingsForChar(spotX, spotY, map, '%') == 1) {
        spotX = (rand() % 29) + 10;
        spotY = (rand() % 10) + 5;
    }

    int height = (rand() % (5 - 3)) + 3;
    int width = (rand() % (12 - 7)) + 7;

    for (i = -height; i < height; i++) {
        for (j = -width; j < width; j++) {
            if (spotY+i >= 0 && spotY+i <= 20 && spotX+j >= 0 && spotX+j <= 79) {
                if (map[spotY + i][spotX + j] != '%' && map[spotY + i][spotX + j] != '#') {
                    map[spotY + i][spotX + j] = ':';
                }
            }
        }
    }

    spotX += ((rand() % 20) + 20);
    spotY += ((rand() % 5) - 7);
    
    height = (rand() % 4) + 3;
    width = (rand() % 8) + 4;

    for (i = -height; i < height; i++) {
        for (j = -width; j < width; j++) {
            if (spotY+i >= 0 && spotY+i <= 20 && spotX+j >= 0 && spotX+j <= 79) {
                if (map[spotY + i][spotX + j] != '%' && map[spotY + i][spotX + j] != '#') {
                    map[spotY + i][spotX + j] = ':';
                }
            }
        }
    }
}

void generateExits(terrainMap *terrainMap, int row, int col) {
    int northSouthExit = (rand() % (69 - 10)) + 10;
    int westEastExit = (rand() % (16 - 3)) + 3;

    terrainMap->northSouthExit = northSouthExit;
    terrainMap->westEastExit = westEastExit;

    if ((row - 1) >= 0 && (row + 1) < 401 && (col - 1) >= 0 && (col + 1) < 401) {
        if (world[row - 1][col]->northSouthExit) {
            terrainMap->northSouthExit = world[row - 1][col]->northSouthExit;
        }
        if (world[row + 1][col]->northSouthExit) {
            terrainMap->northSouthExit = world[row + 1][col]->northSouthExit;    
        }
        if (world[row][col - 1]->westEastExit) {
            terrainMap->westEastExit = world[row][col - 1]->westEastExit;
        }
        if (world[row][col + 1]->westEastExit) {
            terrainMap->westEastExit = world[row][col + 1]->westEastExit;
        }
    }
}

void placeCharacter(terrainMap *terrainMap) {
    // Pick a random road
    int selectedRoad = (rand() % (101 - 0)) + 0;
    int selected = 0;
    // 'Place character' 
    while(!selected) {
        if (terrainMap->roadPositions[selectedRoad].rowPos == 0 
        || terrainMap->roadPositions[selectedRoad].rowPos == 20 
        || terrainMap->roadPositions[selectedRoad].colPos == 0 
        || terrainMap->roadPositions[selectedRoad].colPos == 79) {
            selectedRoad = (rand() % (101 - 0)) + 0;
        } else {
            selected = 1;
        }
    }
    terrainMap->pc.rowPos = terrainMap->roadPositions[selectedRoad].rowPos;
    terrainMap->pc.colPos = terrainMap->roadPositions[selectedRoad].colPos;
}

void populateHikerCosts(char terrain[21][80], squares squares[21][80]) {
    int i, j;

    for (i = 0; i < 21; i++) {
        for (j = 0; j < 80; j++) {
            if(terrain[i][j] == '^' // Tree
            || terrain[i][j] == '~' // Water
            || (terrain[i][j] == '#' && ((i == 0 || i == 20) || (j == 0 || j == 79)))) { // Gate
                squares[i][j].cost = INFINITY_T;
            }
            if(terrain[i][j] == '%' || terrain[i][j] == '^') { // Boulder
                if(i != 0 && i != 20 && j != 0 && j != 79) {
                    squares[i][j].cost = 15;
                } else {
                    squares[i][j].cost = INFINITY_T;
                }
            }
            if(terrain[i][j] == '#' && i != 0 && i != 20 && j != 0 && j != 79) { // Road, not gate
                squares[i][j].cost = 10;
            }
            if(terrain[i][j] == 'M' || terrain[i][j] == 'C') {
                squares[i][j].cost = 50;
            }
            if(terrain[i][j] == ':') {
                squares[i][j].cost = 15;
            }
            if(terrain[i][j] == '.') {
                squares[i][j].cost = 10;
            }
        }
    }
}

void populateRivalCosts(char terrain[21][80], squares squares[21][80]) {
    int i, j;

    for (i = 0; i < 21; i++) {
        for (j = 0; j < 80; j++) {
            if(terrain[i][j] == '%' // Boulder
            || terrain[i][j] == '^' // Tree
            || terrain[i][j] == '~' // Water
            || (terrain[i][j] == '#' && ((i == 0 || i == 20) || (j == 0 || j == 79)))) { // Gate
                squares[i][j].cost = INFINITY_T;
            }
            if(terrain[i][j] == '#' && i != 0 && i != 20 && j != 0 && j != 79) { // Road, not gate
                squares[i][j].cost = 10;
            }
            if(terrain[i][j] == 'M' || terrain[i][j] == 'C') {
                squares[i][j].cost = 50;
            }
            if(terrain[i][j] == ':') {
                squares[i][j].cost = 20;
            }
            if(terrain[i][j] == '.') {
                squares[i][j].cost = 10;
            }
            
        }
    }
}

void dijkstra(char map[21][80], squares squares[21][80], player source) {
    static int altCount = 0;
    int i, j;
    int dist[21][80];
    position positions[21][80];
    position *prev[21][80];
    heap h;
    
    // Using costs of positions as keys, NULL compare function will treat keys as signed integers
    heap_create(&h, 21, NULL);

    // For each 'vertex'
    for(i = 0; i < 21; i++) {
        for(j = 0; j < 80; j++) {
            dist[i][j] = INFINITY_T; // set distance from source to INFINITY_T
            prev[i][j] = (position*) malloc(sizeof(position));
            positions[i][j].rowPos = i;
            positions[i][j].colPos = j;
            if(i == source.rowPos && j == source.colPos) { // set source dist[] to 0
                dist[i][j] = 0;
            }
            heap_insert(&h, &dist[i][j], &positions[i][j]); // add to queue
        }
    }

    // dist[source.rowPos][source.colPos] = 0;

    heap_entry u;
    // While the queue is not empty
    while(heap_size(&h) > 0) {
        // Remove 'vertex' u from the queue
        heap_delmin(&h, &u.key, &u.value);

        // Get the position of the 'vertex' off of the element stored in the queue
        //  AKA its row and column indices on the map.
        position *value = static_cast<position*>(u.value);

        // For each neighbor, v or [i][j], of u
        for (i = value->rowPos - 1; i < value->rowPos + 2; i++) {
            for (j = value->colPos - 1; j < value->colPos + 2; j++) {
                // Make sure neighbors are within bounds of map
                if (i > -1 && i < 21 && j > -1 && j < 80) {
                    // alt = distance to u from source + cost of edge from u to neighbor v
                    int alt = squares[value->rowPos][value->colPos].cost + squares[i][j].cost;
                    if (alt < INFINITY_T && alt < dist[i][j]) { // If alternate path is of lower cost
                        // printf("%d, %d\t", alt, altCount);
                        altCount++;
                        dist[i][j] = alt; // set cost to alt
                        prev[i][j]->rowPos = i;
                        prev[i][j]->colPos = j;
                    }
                }
            }
        }
    }

    // Reassign squares.cost to dist to display the map
    for(i = 0; i < 21; i++) {
        for(j = 0; j < 80; j++) {
            free(prev[i][j]);
            squares[i][j].cost = dist[i][j];
        }
    }
    
    heap_destroy(&h);
}

position findPath(terrainMap *terrainMap, int row, int col, character *trainer) {
    int i, j;
    // squares arrays hold the dijkstra generated cost map to the player
    squares rivalSquares[21][80], hikerSquares[21][80];

    // position variable to return
    position moveHere;
    // Stand still by default
    moveHere.rowPos = row;
    moveHere.colPos = col;

    if(trainer->npc == 'r') {
        for(i = 0; i < 21; i++) {
            for(j = 0; j < 80; j++) {
                rivalSquares[i][j].rowPos = i;
                rivalSquares[i][j].colPos = j;
                rivalSquares[i][j].terrain = terrainMap->terrain[i][j];
            }
        }

        populateRivalCosts(terrainMap->terrain, rivalSquares);

        dijkstra(terrainMap->terrain, rivalSquares, terrainMap->pc);

        for (i = row - 1; i < row + 2; i++) {
            for (j = col - 1; j < col + 2; j++) {
                if (rivalSquares[i][j].cost < rivalSquares[row][col].cost) {
                    moveHere.rowPos = i;
                    moveHere.colPos = j;
                }
            }
        }
    }
    
    if(trainer->npc == 'h') {
        for(i = 0; i < 21; i++) {
            for(j = 0; j < 80; j++) {
                hikerSquares[i][j].rowPos = i;
                hikerSquares[i][j].colPos = j;
                hikerSquares[i][j].terrain = terrainMap->terrain[i][j];
            }
        }

        populateHikerCosts(terrainMap->terrain, hikerSquares);

        dijkstra(terrainMap->terrain, hikerSquares, terrainMap->pc);

        for (i = row - 1; i < row + 2; i++) {
            for (j = col - 1; j < col + 2; j++) {
                if (hikerSquares[i][j].cost < hikerSquares[row][col].cost) {
                    moveHere.rowPos = i;
                    moveHere.colPos = j;
                }
            }
        }
    }

    return moveHere;
}

int positionOccupied(int arrSize, position arr[], position pos) {
    int i;

    for (i = 0; i < arrSize; i++) {
        if (arr[i].rowPos == pos.rowPos && arr[i].colPos == pos.colPos) {
            return 1;
        }
    }
    return 0;
}

void displayMap(terrainMap *terrainMap, int numTrainers, character *trainers[]) {
    int i, j, k;
    char charToPrint;

    for (i = 0; i < 21; i++) {
        for (j = 0; j < 80; j++) {
            charToPrint = terrainMap->terrain[i][j];
            for (k = 0; k < numTrainers; k++) {
                if (i == trainers[k]->rowPos && j == trainers[k]->colPos && trainers[k]->npc != '@') {
                    charToPrint = trainers[k]->npc;
                }
            }
            if(i == terrainMap->pc.rowPos && j == terrainMap->pc.colPos) {
                charToPrint = '@';
            }
            switch(charToPrint) {
                case '~' :
                    attron(COLOR_PAIR(WATER_COLOR));
                    break;
                case '^' :
                    attron(COLOR_PAIR(TREE_COLOR));
                    break;
                case '.' :
                    attron(COLOR_PAIR(GRASS_COLOR));
                    break;
                case ':' :
                    attron(COLOR_PAIR(GRASS_COLOR));
                    break;
                case '%' :
                    attron(COLOR_PAIR(BOULDER_COLOR));
                    break;
                case '#' :
                    attron(COLOR_PAIR(ROAD_COLOR));
                    break;
                default :
                    break;
            }
            mvprintw(i + 1, j, "%c", charToPrint);
            switch(charToPrint) {
                case '~' :
                    attroff(COLOR_PAIR(WATER_COLOR));
                    break;
                case '^' :
                    attroff(COLOR_PAIR(TREE_COLOR));
                    break;
                case '.' :
                    attroff(COLOR_PAIR(GRASS_COLOR));
                    break;
                case ':' :
                    attroff(COLOR_PAIR(GRASS_COLOR));
                    break;
                case '%' :
                    attroff(COLOR_PAIR(BOULDER_COLOR));
                    break;
                case '#' :
                    attroff(COLOR_PAIR(ROAD_COLOR));
                    break;
                default :
                    break;
            }
        }
    }

    // int nsD = 200 - terrainMap->worldRow;
    // int ewD = 200 - terrainMap->worldCol;

    // mvprintw(22, 0, "%c %d %c %d", (nsD < 0 ? 'S' : 'N'), abs(nsD), (ewD < 0 ? 'E' : 'W'), abs(ewD));
}

void findPosition(character *trainer, terrainMap *terrainMap, int numTrainers) {
    static int positionsMarked = 0;
    int col = (rand() % (70 - 10)) + 10;
    int row = (rand() % (16 - 3)) + 3;

    switch(trainer->npc) {
            case '@' :
                break;
            case 'm' : // is a swimmer
                // pick random position that has not been chosen before and is water
                while(terrainMap->terrain[row][col] != '~') {
                    col = (rand() % (70 - 10)) + 10;
                    row = (rand() % (16 - 3)) + 3;
                }
                // mark trainer position
                trainer->rowPos = row;
                trainer->colPos = col;
                // printf("Placed %c at [%d, %d]\n", trainer->npc, row, col);
                
                positionsMarked++;
                break;
            default : // is any other type of npc
                // pick random position that has not been chosen before and is not a boulder, tree, water, or building
                while(terrainMap->terrain[row][col] == '%'
                || terrainMap->terrain[row][col] == '^'
                || terrainMap->terrain[row][col] == '~'
                || terrainMap->terrain[row][col] == '#'
                || terrainMap->terrain[row][col] == 'M'
                || terrainMap->terrain[row][col] == 'C') {
                    col = (rand() % (70 - 10)) + 10;
                    row = (rand() % (16 - 3)) + 3;
                }
                // mark trainer position
                trainer->rowPos = row;
                trainer->colPos = col;
                // printf("Placed %c at [%d, %d]\n", trainer->npc, row, col);
                // for wanderers to know what terrain they spawned in
                trainer->spawn = terrainMap->terrain[row][col];
                
                positionsMarked++;
                break;
        }
}

int getMoveCost(terrainMap *terrainMap, int row, int col, character *trainer) {
    switch(trainer->npc) {
        case '@' :
            switch(terrainMap->terrain[row][col]) {
                case '#' :
                    return 10;
                case 'M' :
                    return 10;
                case 'C' :
                    return 10;
                case ':' :
                    return 20;
                case '.' :
                    return 10;
                default :
                    return INFINITY_T;
            }
            break;
        case 'h' :
            switch(terrainMap->terrain[row][col]) {
                case '#' :
                    return 10;
                case 'M' :
                    return 50;
                case 'C' :
                    return 50;
                case ':' :
                    return 15;
                case '.' :
                    return 10;
                default :
                    return INFINITY_T;
            }
            break;
        case 'm' :
            switch(terrainMap->terrain[row][col]) {
                case '~' :
                    return 7;
                case '#' :
                    return 7;
                default :
                    return INFINITY_T;
            }
            break;
        case 'w' :
            if (terrainMap->terrain[row][col] == trainer->spawn) {
                return 10;
            } else {
                return INFINITY_T;
            }
            break;
        default :
            switch(terrainMap->terrain[row][col]) {
                case '#' :
                    return 10;
                case 'M' :
                    return 50;
                case 'C' :
                    return 50;
                case ':' :
                    return 20;
                case '.' :
                    return 10;
                case '~' :
                    return INFINITY_T;
                default :
                    return INFINITY_T;
            }
            break;
    }
    return INFINITY_T;
}

int positionNotOccupied(int row, int col, int numTrainers, character *trainers[]) {
    int i;

    for (i = 0; i < numTrainers; i++) {
        if (trainers[i]->rowPos == row && trainers[i]->colPos == col) {
            return i + 10;
        }
    }

    return 1;
}

int notGate(terrainMap *terrainMap, int row, int col) {
    if ((row == 0 && col == terrainMap->northSouthExit)
    || (row == 20 && col == terrainMap->northSouthExit)
    || (col == 0 && row == terrainMap->westEastExit)
    || (col == 79 && row == terrainMap->westEastExit)) {
        return 0;
    }

    return 1; 
}

void createPokemon(worldPokemon *p) {
    int pokeChoice = (rand() % 1092) + 1;

    // Set pokemon_id, species_id, & base_experience from pokemon.csv
    p->pokeId = pokemon[pokeChoice].id;
    p->speciesId = pokemon[pokeChoice].species_id;
    p->exp = pokemon[pokeChoice].base_experience;

    // Random values/chance
    p->gender = (rand() % 2 == 0);
    p->shiny = (rand() % 8192 == 4);

    p->hpIV = rand() % 16;
    p->attackIV = rand() % 16;
    p->defenseIV = rand() % 16;
    p->specialAttackIV = rand() % 16;
    p->specialDefenseIV = rand() % 16;
    p->speedIV = rand() % 16;

    // Setting name/identifier & level growth rate from pokemon_species.csv
    for (int i = 0; i < 899; i++) {
        if (p->speciesId == species[i].id) {
            strcpy(p->name, species[i].identifier);
            p->name[0] = toupper(p->name[0]);
            p->lvlRate = species[i].growth_rate_id;
        }
    }

    // Setting level from experience.csv using lvlRate
    int j = 0;
    for (int i = 0; i < 601; i++) {
        if (experience[i].growth_rate_id == p->lvlRate) {
            p->levels[j].growth_rate_id = experience[i].growth_rate_id;
            p->levels[j].level = experience[i].level;
            p->levels[j].experience = experience[i].experience;
            j++;
        }
    }
    j = 0;
    while (p->exp > p->levels[j].experience) {
        j++;
    }
    p->level = p->levels[j-1].level;

    // Get Base Stats from pokemon_stats.csv
    for (int i = 0; i < 6553; i++) {
        if(pokemon_stats[i].pokemon_id == p->pokeId) {
            switch(pokemon_stats[i].stat_id) {
                case 1 :
                    p->hp = pokemon_stats[i].base_stat;
                    break;
                case 2 :
                    p->attack = pokemon_stats[i].base_stat;
                    break;
                case 3 :
                    p->defense = pokemon_stats[i].base_stat;
                    break;
                case 4 :
                    p->specialAttack = pokemon_stats[i].base_stat;
                    break;
                case 5 :
                    p->specialDefense = pokemon_stats[i].base_stat;
                    break;
                case 6 :
                    p->speed = pokemon_stats[i].base_stat;
                    break;
            }
        }
    }

    // Find moves
    int learnables[250];
    j = 0;
    for (int i = 0; i < 528239; i++) {
        if (pokemon_moves[i].pokemon_id == p->speciesId
        && pokemon_moves[i].pokemon_move_method_id == 1) {
            learnables[j] = pokemon_moves[i].move_id;
            if (j + 1 < 250) {
                j++;
            }
        }
    }

    // pick potentially two moves
    int num1 = learnables[rand() % (j - 1)];
    int num2 = num1;
    while (num2 == num1) {
        num2 = learnables[rand() % (j - 1)];
    }

    j = 0;
    for (int i = 0; i < 845; i++) {
        if (moves[i].id == num1 || moves[i].id == num2) {
            p->pokeMoves[j].id = moves[i].id;
            strcpy(p->pokeMoves[j].identifier, moves[i].identifier);
            p->pokeMoves[j].generation_id = moves[i].generation_id;
            p->pokeMoves[j].type_id = moves[i].type_id;
            p->pokeMoves[j].power = moves[i].power;
            p->pokeMoves[j].pp = moves[i].pp;
            p->pokeMoves[j].accuracy = moves[i].accuracy;
            p->pokeMoves[j].priority = moves[i].priority;
            p->pokeMoves[j].target_id = moves[i].target_id;
            p->pokeMoves[j].damage_class_id = moves[i].damage_class_id;
            p->pokeMoves[j].effect_id = moves[i].effect_id;
            p->pokeMoves[j].effect_chance = moves[i].effect_chance;
            p->pokeMoves[j].contest_type_id = moves[i].contest_type_id;
            p->pokeMoves[j].contest_effect_id = moves[i].contest_effect_id;
            p->pokeMoves[j].super_contest_effect_id = moves[i].super_contest_effect_id;
            j++;
        }
    }
    
    p->initialHP = p->hp;
}

void givePlayerPokemon() {
    worldPokemon *option1 = static_cast<worldPokemon*>(malloc(sizeof(worldPokemon)));
    createPokemon(option1);
    option1->level = 1;
    option1->exp = 0;
    option1->shiny = 0;
    worldPokemon *option2 = static_cast<worldPokemon*>(malloc(sizeof(worldPokemon)));
    createPokemon(option2);
    option2->level = 1;
    option2->exp = 0;
    option2->shiny = 0;
    worldPokemon *option3 = static_cast<worldPokemon*>(malloc(sizeof(worldPokemon)));
    createPokemon(option3);
    option3->level = 1;
    option3->exp = 0;
    option3->shiny = 0;

    WINDOW *win = newwin(19, 78, 2, 1);
    refresh();
    box(win, 0, 0);

    mvwprintw(win, 0, 10, "Before you begin your journey, you must pick your companion");
    
    mvwprintw(win, 2, 7, "%s", option1->name);
    mvwprintw(win, 3, 4, "Gender: %s", (option1->gender ? "Male" : "Female"));
    mvwprintw(win, 4, 4, "Level: %d", option1->level);
    mvwprintw(win, 6, 4, "HP: %d", option1->hp);
    mvwprintw(win, 7, 4, "ATK: %d", option1->attack);
    mvwprintw(win, 8, 4, "DEF: %d", option1->defense);
    mvwprintw(win, 9, 4, "Sp-ATK: %d", option1->specialAttack);
    mvwprintw(win, 10, 4, "Sp-DEF: %d", option1->specialDefense);
    mvwprintw(win, 11, 4, "SPD: %d", option1->speed);
    mvwprintw(win, 13, 4, "Knows:");
    mvwprintw(win, 14, 5, "%s", option1->pokeMoves[0].identifier);
    mvwprintw(win, 15, 5, "%s", option1->pokeMoves[1].identifier);
    
    mvwprintw(win, 2, 32, "%s", option2->name);
    mvwprintw(win, 3, 28, "Gender: %s", (option2->gender ? "Male" : "Female"));
    mvwprintw(win, 4, 28, "Level: %d", option2->level);
    mvwprintw(win, 6, 28, "HP: %d", option2->hp);
    mvwprintw(win, 7, 28, "ATK: %d", option2->attack);
    mvwprintw(win, 8, 28, "DEF: %d", option2->defense);
    mvwprintw(win, 9, 28, "Sp-ATK: %d", option2->specialAttack);
    mvwprintw(win, 10, 28, "Sp-DEF: %d", option2->specialDefense);
    mvwprintw(win, 11, 28, "SPD: %d", option2->speed);
    mvwprintw(win, 13, 28, "Knows:");
    mvwprintw(win, 14, 29, "%s", option2->pokeMoves[0].identifier);
    mvwprintw(win, 15, 29, "%s", option2->pokeMoves[1].identifier);

    mvwprintw(win, 2, 59, "%s", option3->name);
    mvwprintw(win, 3, 55, "Gender: %s", (option3->gender ? "Male" : "Female"));
    mvwprintw(win, 4, 55, "Level: %d", option3->level);
    mvwprintw(win, 6, 55, "HP: %d", option3->hp);
    mvwprintw(win, 7, 55, "ATK: %d", option3->attack);
    mvwprintw(win, 8, 55, "DEF: %d", option3->defense);
    mvwprintw(win, 9, 55, "Sp-ATK: %d", option3->specialAttack);
    mvwprintw(win, 10, 55, "Sp-DEF: %d", option3->specialDefense);
    mvwprintw(win, 11, 55, "SPD: %d", option3->speed);
    mvwprintw(win, 13, 55, "Knows:");
    mvwprintw(win, 14, 56, "%s", option3->pokeMoves[0].identifier);
    mvwprintw(win, 15, 56, "%s", option3->pokeMoves[1].identifier);

    echo();
    mvwprintw(win, 17, 10, "Please enter 1, 2, or 3 to make your selection: ");
    move(19, 59);
    wrefresh(win);
    char str[10] = "";
    while(strcmp(str, "") == 0) {
        getstr(str);
        if (strcmp(str, "1") == 0) { // Choose Option 1
            strcpy(heldPokemon[0].name, option1->name);

            // Copy moves
                heldPokemon[0].pokeMoves[0].id = option1->pokeMoves[0].id;
                strcpy(heldPokemon[0].pokeMoves[0].identifier, option1->pokeMoves[0].identifier);
                heldPokemon[0].pokeMoves[0].generation_id = option1->pokeMoves[0].generation_id;
                heldPokemon[0].pokeMoves[0].type_id = option1->pokeMoves[0].type_id;
                heldPokemon[0].pokeMoves[0].power = option1->pokeMoves[0].power;
                heldPokemon[0].pokeMoves[0].pp = option1->pokeMoves[0].pp;
                heldPokemon[0].pokeMoves[0].accuracy = option1->pokeMoves[0].accuracy;
                heldPokemon[0].pokeMoves[0].priority = option1->pokeMoves[0].priority;
                heldPokemon[0].pokeMoves[0].target_id = option1->pokeMoves[0].target_id;
                heldPokemon[0].pokeMoves[0].damage_class_id = option1->pokeMoves[0].damage_class_id;
                heldPokemon[0].pokeMoves[0].effect_id = option1->pokeMoves[0].effect_id;
                heldPokemon[0].pokeMoves[0].effect_chance = option1->pokeMoves[0].effect_chance;
                heldPokemon[0].pokeMoves[0].contest_type_id = option1->pokeMoves[0].contest_type_id;
                heldPokemon[0].pokeMoves[0].contest_effect_id = option1->pokeMoves[0].contest_effect_id;
                heldPokemon[0].pokeMoves[0].super_contest_effect_id = option1->pokeMoves[0].super_contest_effect_id;

                heldPokemon[0].pokeMoves[1].id = option1->pokeMoves[1].id;
                strcpy(heldPokemon[0].pokeMoves[1].identifier, option1->pokeMoves[1].identifier);
                heldPokemon[0].pokeMoves[1].generation_id = option1->pokeMoves[1].generation_id;
                heldPokemon[0].pokeMoves[1].type_id = option1->pokeMoves[1].type_id;
                heldPokemon[0].pokeMoves[1].power = option1->pokeMoves[1].power;
                heldPokemon[0].pokeMoves[1].pp = option1->pokeMoves[1].pp;
                heldPokemon[0].pokeMoves[1].accuracy = option1->pokeMoves[1].accuracy;
                heldPokemon[0].pokeMoves[1].priority = option1->pokeMoves[1].priority;
                heldPokemon[0].pokeMoves[1].target_id = option1->pokeMoves[1].target_id;
                heldPokemon[0].pokeMoves[1].damage_class_id = option1->pokeMoves[1].damage_class_id;
                heldPokemon[0].pokeMoves[1].effect_id = option1->pokeMoves[1].effect_id;
                heldPokemon[0].pokeMoves[1].effect_chance = option1->pokeMoves[1].effect_chance;
                heldPokemon[0].pokeMoves[1].contest_type_id = option1->pokeMoves[1].contest_type_id;
                heldPokemon[0].pokeMoves[1].contest_effect_id = option1->pokeMoves[1].contest_effect_id;
                heldPokemon[0].pokeMoves[1].super_contest_effect_id = option1->pokeMoves[1].super_contest_effect_id;

            
            heldPokemon[0].gender = option1->gender;
            heldPokemon[0].shiny = option1->shiny;
            heldPokemon[0].level = option1->level;
            for (int i = 0; i < 101; i++) {
                heldPokemon[0].levels[i].growth_rate_id = option1->levels[i].growth_rate_id;
                heldPokemon[0].levels[i].level = option1->levels[i].level;
                heldPokemon[0].levels[i].experience = option1->levels[i].experience;
            }
            heldPokemon[0].exp = option1->exp;
            heldPokemon[0].lvlRate = option1->lvlRate;
            heldPokemon[0].pokeId = option1->pokeId;
            heldPokemon[0].speciesId = option1->speciesId;
            heldPokemon[0].hp = option1->hp;
            heldPokemon[0].initialHP = option1->initialHP;
            heldPokemon[0].hpIV = option1->hpIV;
            heldPokemon[0].attack = option1->attack;
            heldPokemon[0].attackIV = option1->attackIV;
            heldPokemon[0].defense = option1->defense;
            heldPokemon[0].defenseIV = option1->defenseIV;
            heldPokemon[0].specialAttack = option1->specialAttack;
            heldPokemon[0].specialAttackIV = option1->specialAttackIV;
            heldPokemon[0].specialDefense = option1->specialDefense;
            heldPokemon[0].specialDefenseIV = option1->specialDefenseIV;
            heldPokemon[0].speed = option1->speed;
            heldPokemon[0].speedIV = option1->speedIV;
        } else if (strcmp(str, "2") == 0) { // Choose Option 2
            strcpy(heldPokemon[0].name, option2->name);

            // Copy moves
                heldPokemon[0].pokeMoves[0].id = option2->pokeMoves[0].id;
                strcpy(heldPokemon[0].pokeMoves[0].identifier, option2->pokeMoves[0].identifier);
                heldPokemon[0].pokeMoves[0].generation_id = option2->pokeMoves[0].generation_id;
                heldPokemon[0].pokeMoves[0].type_id = option2->pokeMoves[0].type_id;
                heldPokemon[0].pokeMoves[0].power = option2->pokeMoves[0].power;
                heldPokemon[0].pokeMoves[0].pp = option2->pokeMoves[0].pp;
                heldPokemon[0].pokeMoves[0].accuracy = option2->pokeMoves[0].accuracy;
                heldPokemon[0].pokeMoves[0].priority = option2->pokeMoves[0].priority;
                heldPokemon[0].pokeMoves[0].target_id = option2->pokeMoves[0].target_id;
                heldPokemon[0].pokeMoves[0].damage_class_id = option2->pokeMoves[0].damage_class_id;
                heldPokemon[0].pokeMoves[0].effect_id = option2->pokeMoves[0].effect_id;
                heldPokemon[0].pokeMoves[0].effect_chance = option2->pokeMoves[0].effect_chance;
                heldPokemon[0].pokeMoves[0].contest_type_id = option2->pokeMoves[0].contest_type_id;
                heldPokemon[0].pokeMoves[0].contest_effect_id = option2->pokeMoves[0].contest_effect_id;
                heldPokemon[0].pokeMoves[0].super_contest_effect_id = option2->pokeMoves[0].super_contest_effect_id;

                heldPokemon[0].pokeMoves[1].id = option2->pokeMoves[1].id;
                strcpy(heldPokemon[0].pokeMoves[1].identifier, option2->pokeMoves[1].identifier);
                heldPokemon[0].pokeMoves[1].generation_id = option2->pokeMoves[1].generation_id;
                heldPokemon[0].pokeMoves[1].type_id = option2->pokeMoves[1].type_id;
                heldPokemon[0].pokeMoves[1].power = option2->pokeMoves[1].power;
                heldPokemon[0].pokeMoves[1].pp = option2->pokeMoves[1].pp;
                heldPokemon[0].pokeMoves[1].accuracy = option2->pokeMoves[1].accuracy;
                heldPokemon[0].pokeMoves[1].priority = option2->pokeMoves[1].priority;
                heldPokemon[0].pokeMoves[1].target_id = option2->pokeMoves[1].target_id;
                heldPokemon[0].pokeMoves[1].damage_class_id = option2->pokeMoves[1].damage_class_id;
                heldPokemon[0].pokeMoves[1].effect_id = option2->pokeMoves[1].effect_id;
                heldPokemon[0].pokeMoves[1].effect_chance = option2->pokeMoves[1].effect_chance;
                heldPokemon[0].pokeMoves[1].contest_type_id = option2->pokeMoves[1].contest_type_id;
                heldPokemon[0].pokeMoves[1].contest_effect_id = option2->pokeMoves[1].contest_effect_id;
                heldPokemon[0].pokeMoves[1].super_contest_effect_id = option2->pokeMoves[1].super_contest_effect_id;

            
            heldPokemon[0].gender = option2->gender;
            heldPokemon[0].shiny = option2->shiny;
            heldPokemon[0].level = option2->level;
            for (int i = 0; i < 101; i++) {
                heldPokemon[0].levels[i].growth_rate_id = option2->levels[i].growth_rate_id;
                heldPokemon[0].levels[i].level = option2->levels[i].level;
                heldPokemon[0].levels[i].experience = option2->levels[i].experience;
            }
            heldPokemon[0].exp = option2->exp;
            heldPokemon[0].lvlRate = option2->lvlRate;
            heldPokemon[0].pokeId = option2->pokeId;
            heldPokemon[0].speciesId = option2->speciesId;
            heldPokemon[0].hp = option2->hp;
            heldPokemon[0].initialHP = option2->initialHP;
            heldPokemon[0].hpIV = option2->hpIV;
            heldPokemon[0].attack = option2->attack;
            heldPokemon[0].attackIV = option2->attackIV;
            heldPokemon[0].defense = option2->defense;
            heldPokemon[0].defenseIV = option2->defenseIV;
            heldPokemon[0].specialAttack = option2->specialAttack;
            heldPokemon[0].specialAttackIV = option2->specialAttackIV;
            heldPokemon[0].specialDefense = option2->specialDefense;
            heldPokemon[0].specialDefenseIV = option2->specialDefenseIV;
            heldPokemon[0].speed = option2->speed;
            heldPokemon[0].speedIV = option2->speedIV;
        } else if (strcmp(str, "3") == 0) { // Choose Option 3
            strcpy(heldPokemon[0].name, option3->name);

            // Copy moves
                heldPokemon[0].pokeMoves[0].id = option3->pokeMoves[0].id;
                strcpy(heldPokemon[0].pokeMoves[0].identifier, option3->pokeMoves[0].identifier);
                heldPokemon[0].pokeMoves[0].generation_id = option3->pokeMoves[0].generation_id;
                heldPokemon[0].pokeMoves[0].type_id = option3->pokeMoves[0].type_id;
                heldPokemon[0].pokeMoves[0].power = option3->pokeMoves[0].power;
                heldPokemon[0].pokeMoves[0].pp = option3->pokeMoves[0].pp;
                heldPokemon[0].pokeMoves[0].accuracy = option3->pokeMoves[0].accuracy;
                heldPokemon[0].pokeMoves[0].priority = option3->pokeMoves[0].priority;
                heldPokemon[0].pokeMoves[0].target_id = option3->pokeMoves[0].target_id;
                heldPokemon[0].pokeMoves[0].damage_class_id = option3->pokeMoves[0].damage_class_id;
                heldPokemon[0].pokeMoves[0].effect_id = option3->pokeMoves[0].effect_id;
                heldPokemon[0].pokeMoves[0].effect_chance = option3->pokeMoves[0].effect_chance;
                heldPokemon[0].pokeMoves[0].contest_type_id = option3->pokeMoves[0].contest_type_id;
                heldPokemon[0].pokeMoves[0].contest_effect_id = option3->pokeMoves[0].contest_effect_id;
                heldPokemon[0].pokeMoves[0].super_contest_effect_id = option3->pokeMoves[0].super_contest_effect_id;

                heldPokemon[0].pokeMoves[1].id = option3->pokeMoves[1].id;
                strcpy(heldPokemon[0].pokeMoves[1].identifier, option3->pokeMoves[1].identifier);
                heldPokemon[0].pokeMoves[1].generation_id = option3->pokeMoves[1].generation_id;
                heldPokemon[0].pokeMoves[1].type_id = option3->pokeMoves[1].type_id;
                heldPokemon[0].pokeMoves[1].power = option3->pokeMoves[1].power;
                heldPokemon[0].pokeMoves[1].pp = option3->pokeMoves[1].pp;
                heldPokemon[0].pokeMoves[1].accuracy = option3->pokeMoves[1].accuracy;
                heldPokemon[0].pokeMoves[1].priority = option3->pokeMoves[1].priority;
                heldPokemon[0].pokeMoves[1].target_id = option3->pokeMoves[1].target_id;
                heldPokemon[0].pokeMoves[1].damage_class_id = option3->pokeMoves[1].damage_class_id;
                heldPokemon[0].pokeMoves[1].effect_id = option3->pokeMoves[1].effect_id;
                heldPokemon[0].pokeMoves[1].effect_chance = option3->pokeMoves[1].effect_chance;
                heldPokemon[0].pokeMoves[1].contest_type_id = option3->pokeMoves[1].contest_type_id;
                heldPokemon[0].pokeMoves[1].contest_effect_id = option3->pokeMoves[1].contest_effect_id;
                heldPokemon[0].pokeMoves[1].super_contest_effect_id = option3->pokeMoves[1].super_contest_effect_id;

            
            heldPokemon[0].gender = option3->gender;
            heldPokemon[0].shiny = option3->shiny;
            heldPokemon[0].level = option3->level;
            for (int i = 0; i < 101; i++) {
                heldPokemon[0].levels[i].growth_rate_id = option3->levels[i].growth_rate_id;
                heldPokemon[0].levels[i].level = option3->levels[i].level;
                heldPokemon[0].levels[i].experience = option3->levels[i].experience;
            }
            heldPokemon[0].exp = option3->exp;
            heldPokemon[0].lvlRate = option3->lvlRate;
            heldPokemon[0].pokeId = option3->pokeId;
            heldPokemon[0].speciesId = option3->speciesId;
            heldPokemon[0].hp = option3->hp;
            heldPokemon[0].initialHP = option3->initialHP;
            heldPokemon[0].hpIV = option3->hpIV;
            heldPokemon[0].attack = option3->attack;
            heldPokemon[0].attackIV = option3->attackIV;
            heldPokemon[0].defense = option3->defense;
            heldPokemon[0].defenseIV = option3->defenseIV;
            heldPokemon[0].specialAttack = option3->specialAttack;
            heldPokemon[0].specialAttackIV = option3->specialAttackIV;
            heldPokemon[0].specialDefense = option3->specialDefense;
            heldPokemon[0].specialDefenseIV = option3->specialDefenseIV;
            heldPokemon[0].speed = option3->speed;
            heldPokemon[0].speedIV = option3->speedIV;
        } else {
            strcpy(str, "");
            mvwprintw(win, 17, 10, "%-67s", "Please enter 1, 2, or 3 to make your selection: ");
            move(19, 59);
        }
        wrefresh(win);
        free(option1);
        free(option2);
        free(option3);
    }

    activePokemon = 0;
    
    werase(win);
    box(win, 0, 0);

    mvwprintw(win, 8, 25, "Good luck!");
    mvwprintw(win, 17, 48, "Press any key to continue...");
    wrefresh(win);

    refresh();
    noecho();
}

void attackPlayer(worldPokemon *enemy, WINDOW *win, int enemyMove) {
    double enemyDamage = enemy->level;
    enemyDamage *= 2;
    enemyDamage /= 5;
    enemyDamage += 2;
    if (enemy->pokeMoves[enemyMove].power != 2147483647) {
        enemyDamage *= enemy->pokeMoves[enemyMove].power;
    } else {
        enemyDamage *= 40;
    }
    int atk2 = enemy->attack + enemy->attackIV;
    int def2 = enemy->defense + enemy->defenseIV;
    double attDef2 = ((double)atk2) / def2;
    enemyDamage *= attDef2;
    enemyDamage /= 15;
    enemyDamage += 2;
    if (rand() % 256 < (enemy->speed / 2)) {
        enemyDamage *= 1.5;
    }
    enemyDamage *= (rand() % (100 + 1 - 85) + 85) / 100.00;
    enemyDamage = ceil(enemyDamage);
    if (enemyDamage > 1000) {
        enemyDamage = rand() % (21 + 1 - 5) + 5;
    }

    if ((rand() % 100) < enemy->pokeMoves[enemyMove].accuracy) {
        mvwprintw(win, 2, 10, "%s used %s!%15s", enemy->name, enemy->pokeMoves[enemyMove].identifier, "");
        if (heldPokemon[activePokemon].hp - enemyDamage > 0) {
            heldPokemon[activePokemon].hp -= enemyDamage;
        } else {
            heldPokemon[activePokemon].hp = 0;
        }
        mvwprintw(win, 10, 10, "HP: %3d / %-3d%-5s", heldPokemon[activePokemon].hp, heldPokemon[activePokemon].initialHP, "");
        mvwprintw(win, 4, 11, "%s did %d damage!%30s", enemy->pokeMoves[enemyMove].identifier, (int)enemyDamage, "");
        wrefresh(win);
        usleep(1500000);
    } else {
        mvwprintw(win, 2, 10, "%s missed...%20s", enemy->pokeMoves[enemyMove].identifier, "");
        mvwprintw(win, 4, 11, "%-60s", "");
        usleep(1500000);
    }
}

void attackSequence(worldPokemon *enemy, WINDOW *win, move_db moveMade) {
    int enemyMoveCount = 0;
    for (int i = 0; i < 4; i++) {
        if (strcmp(enemy->pokeMoves[i].identifier, "") != 0) {
            enemyMoveCount++;
        }
    }
    int enemyMove = rand() % 2;
    int enemyPriority = enemy->pokeMoves[enemyMove].priority;
    wrefresh(win);

    double damage = heldPokemon[activePokemon].level;
    damage *= 2;
    damage /= 5;
    damage += 2;
    if (moveMade.power != 2147483647) {
        damage *= moveMade.power;
    } else {
        damage *= 40;
    }
    int atk = heldPokemon[activePokemon].attack + heldPokemon[activePokemon].attackIV;
    int def = heldPokemon[activePokemon].defense + heldPokemon[activePokemon].defenseIV;
    double attDef = ((double)atk) / def;
    damage *= attDef;
    damage /= 15;
    damage += 2;
    if (rand() % 256 < (heldPokemon[activePokemon].speed / 2)) {
        damage *= 1.5;
    }
    damage *= (rand() % (100 + 1 - 85) + 85) / 100.00;
    damage = ceil(damage);
    // damage *= stab

    // attack enemy then get attacked
    if (moveMade.priority > enemyPriority) {
        if ((rand() % 100) < moveMade.accuracy) {
            mvwprintw(win, 2, 10, "%s used %s!%15s", heldPokemon[activePokemon].name, moveMade.identifier, "");
            if (enemy->hp - damage > 0) {
                enemy->hp -= damage;
            } else {
                enemy->hp = 0;
            }
            mvwprintw(win, 3, 55, "HP: %3d / %-3d", enemy->hp, enemy->initialHP);
            mvwprintw(win, 4, 11, "%s did %d damage!%30s", moveMade.identifier, (int)damage, "");
            wrefresh(win);
            usleep(1500000);
        } else {
            mvwprintw(win, 2, 10, "%s missed...%20s", moveMade.identifier, "");
            wrefresh(win);
            usleep(1500000);
        }
        if (enemy->hp > 0) {
            attackPlayer(enemy, win, enemyMove);
        }
    } else {
        attackPlayer(enemy, win, enemyMove);
        if (heldPokemon[activePokemon].hp > 0) {
            if ((rand() % 100) < moveMade.accuracy) {
                mvwprintw(win, 2, 10, "%s used %s!%15s", heldPokemon[activePokemon].name, moveMade.identifier, "");
                if (enemy->hp - damage > 0) {
                    enemy->hp -= damage;
                } else {
                    enemy->hp = 0;
                }
                mvwprintw(win, 3, 55, "HP: %3d / %-3d", enemy->hp, enemy->initialHP);
                mvwprintw(win, 4, 11, "%s did %d damage!%30s", moveMade.identifier, (int)damage, "");
                wrefresh(win);
                usleep(1500000);
            } else {
                mvwprintw(win, 2, 10, "%s missed...%20s", moveMade.identifier, "");
                wrefresh(win);
                usleep(1500000);
            }
        }
    }
}

int battleSequence(worldPokemon *enemy, WINDOW *win, int isWild) {
    int battling = 1;
    int c = 0;
    mvprintw(0, 0, "You are now battling %s%20s", enemy->name, "");

    while (battling) {
        int enemyMoveCount = 0;
        for (int i = 0; i < 4; i++) {
            if (strcmp(enemy->pokeMoves[i].identifier, "") != 0) {
                enemyMoveCount++;
            }
        }
        int enemyMove = rand() % 2;
        
        mvwvline(win, 14, 40, ACS_VLINE, 4);
        mvwhline(win, 14, 1, ACS_HLINE, 76);
        mvwhline(win, 16, 1, ACS_HLINE, 76);

        mvwprintw(win, 2, 55, "%-22s", enemy->name);
        mvwprintw(win, 3, 55, "HP: %3d / %-3d%-5s", enemy->hp, enemy->initialHP, "");

        mvwprintw(win, 9, 10, "%-22s", heldPokemon[activePokemon].name);
        mvwprintw(win, 10, 10, "HP: %3d / %-3d%-5s", heldPokemon[activePokemon].hp, heldPokemon[activePokemon].initialHP, "");

        mvwprintw(win, 15, 15, "%-22s", "1. Fight");
        mvwprintw(win, 17, 15, "%-22s", "2. Bag");
        mvwprintw(win, 15, 52, "%-22s", "3. Run");
        mvwprintw(win, 17, 52, "%-22s", "4. Pokemon");

        wrefresh(win);
        
        mvprintw(22, 0, "%-75s", "Please enter an action: ");
        mvwprintw(win, 2, 10, "%40s", "");
        mvwprintw(win, 4, 11, "%40s", "");
        wrefresh(win);
        move(22, 24);
        refresh();
        echo();

        char str[10] = "";
        while(strcmp(str, "") == 0) {
            getstr(str);
            move(22, 0);
            clrtoeol();
            int moveCount = 0;
            // Fight
            if (strcmp(str, "1") == 0) {
                for (int i = 0; i < 4; i++) {
                    if (strcmp(heldPokemon[activePokemon].pokeMoves[i].identifier, "") != 0) {
                        moveCount++;
                    }
                }
                mvwprintw(win, 15, 15, "%-22s", "");
                mvwprintw(win, 17, 15, "%-22s", "");
                mvwprintw(win, 15, 52, "%-22s", "");
                mvwprintw(win, 17, 52, "%-22s", "");
                if (moveCount > 0) {
                    mvwprintw(win, 15, 15, "1. %-18s", heldPokemon[activePokemon].pokeMoves[0].identifier);
                }
                if (moveCount > 1) {
                    mvwprintw(win, 17, 15, "2. %-18s", heldPokemon[activePokemon].pokeMoves[1].identifier);
                }
                if (moveCount > 2) {
                    mvwprintw(win, 15, 52, "3. %-18s", heldPokemon[activePokemon].pokeMoves[2].identifier);
                }
                if (moveCount > 3) {
                    mvwprintw(win, 17, 52, "4. %-18s", heldPokemon[activePokemon].pokeMoves[3].identifier);
                }
                mvprintw(22, 0, "%-60s", "Please enter a move: ");
                move(22, 22);
                refresh();
                wrefresh(win);
                char str[10] = "";
                while(strcmp(str, "") == 0) {
                    refresh();
                    getstr(str);
                    if (strcmp(str, "1") == 0 && moveCount >= 1) {
                        attackSequence(enemy, win, heldPokemon[activePokemon].pokeMoves[0]);
                    } else if (strcmp(str, "2") == 0 && moveCount >= 2) {
                        attackSequence(enemy, win, heldPokemon[activePokemon].pokeMoves[1]);
                    } else if (strcmp(str, "3") == 0 && moveCount >= 3) {
                        attackSequence(enemy, win, heldPokemon[activePokemon].pokeMoves[2]);
                    } else if (strcmp(str, "4") == 0 && moveCount >= 4) { 
                        attackSequence(enemy, win, heldPokemon[activePokemon].pokeMoves[3]);
                    } else {
                        strcpy(str, "");
                    }
                }
                usleep(500000);
            // Bag
            } else if (strcmp(str, "2") == 0) {
                // open bag
                int cancel = 0;
                mvprintw(22, 0, "%-60s", "Please pick an item to use: ");
                mvwprintw(win, 15, 15, "%-22s", "");
                mvwprintw(win, 17, 15, "%-22s", "");
                mvwprintw(win, 15, 52, "%-22s", "");
                mvwprintw(win, 17, 52, "%-22s", "");

                mvwprintw(win, 15, 15, "1. Potion (%d left)", potions);
                mvwprintw(win, 17, 15, "2. Revive (%d left)", revives);
                mvwprintw(win, 15, 52, "3. Pokeball (%d left)", pokeballs);

                move(22, 28);
                refresh();
                wrefresh(win);
                char str[10] = "";
                while(strcmp(str, "") == 0) {
                    wrefresh(win);
                    refresh();
                    getstr(str);
                    // int chosenPokemon;
                    if (strcmp(str, "1") == 0) {
                        // use potion on active pokemon
                        if (potions > 0) {
                            mvwvline(win, 12, 40, ACS_VLINE, 6);
                            mvwhline(win, 12, 1, ACS_HLINE, 76);
                            mvwprintw(win, 13, 15, "%20s", "");
                            mvwprintw(win, 15, 15, "%20s", "");
                            mvwprintw(win, 17, 15, "%20s", "");
                            mvwprintw(win, 13, 52, "%20s", "");
                            mvwprintw(win, 15, 52, "%20s", "");
                            mvwprintw(win, 17, 52, "%20s", "");
                            
                            if (numPokemon >= 1) {
                                mvwprintw(win, 13, 15, "1. %s (%d/%d)%14s", heldPokemon[0].name, heldPokemon[0].hp, heldPokemon[0].initialHP, "");
                            }
                            if (numPokemon >= 2) {
                                mvwprintw(win, 15, 15, "2. %s (%d/%d)%14s", heldPokemon[1].name, heldPokemon[1].hp, heldPokemon[1].initialHP, "");
                            }
                            if (numPokemon >= 3) {
                                mvwprintw(win, 17, 15, "3. %s (%d/%d)%14s", heldPokemon[2].name, heldPokemon[2].hp, heldPokemon[2].initialHP, "");
                            }
                            if (numPokemon >= 4) {
                                mvwprintw(win, 13, 52, "4. %s (%d/%d)%14s", heldPokemon[3].name, heldPokemon[3].hp, heldPokemon[3].initialHP, "");
                            }
                            if (numPokemon >= 5) {
                                mvwprintw(win, 15, 52, "5. %s (%d/%d)%14s", heldPokemon[4].name, heldPokemon[4].hp, heldPokemon[4].initialHP, "");
                            }
                            if (numPokemon >= 6) {
                                mvwprintw(win, 17, 52, "6. %s (%d/%d)%14s", heldPokemon[5].name, heldPokemon[5].hp, heldPokemon[5].initialHP, "");
                            }
                            mvprintw(22, 0, "Please choose which pokemon to heal (+20): ");
                            move(22, 43);
                            potions--;
                            char str[10] = "";
                            while(strcmp(str, "") == 0) {
                                wrefresh(win);
                                getstr(str);
                                if (strcmp(str, "1") == 0) {
                                    if (heldPokemon[0].hp + 20 < heldPokemon[0].initialHP) {
                                        heldPokemon[0].hp += 20;
                                    } else {
                                        heldPokemon[0].hp = heldPokemon[0].initialHP;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "2") == 0) {
                                    if (heldPokemon[1].hp + 20 < heldPokemon[1].initialHP) {
                                        heldPokemon[1].hp += 20;
                                    } else {
                                        heldPokemon[1].hp = heldPokemon[1].initialHP;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "3") == 0) {
                                    if (heldPokemon[2].hp + 20 < heldPokemon[2].initialHP) {
                                        heldPokemon[2].hp += 20;
                                    } else {
                                        heldPokemon[2].hp = heldPokemon[2].initialHP;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "4") == 0) {
                                    if (heldPokemon[3].hp + 20 < heldPokemon[3].initialHP) {
                                        heldPokemon[3].hp += 20;
                                    } else {
                                        heldPokemon[3].hp = heldPokemon[3].initialHP;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "5") == 0) {
                                    if (heldPokemon[4].hp + 20 < heldPokemon[4].initialHP) {
                                        heldPokemon[4].hp += 20;
                                    } else {
                                        heldPokemon[4].hp = heldPokemon[4].initialHP;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "6") == 0) {
                                    if (heldPokemon[5].hp + 20 < heldPokemon[5].initialHP) {
                                        heldPokemon[5].hp += 20;
                                    } else {
                                        heldPokemon[5].hp = heldPokemon[5].initialHP;
                                    }
                                    usleep(1500000);
                                } else {
                                    strcpy(str, "");
                                }
                        
                                mvprintw(0, 0, "%79s", "");
                                mvwprintw(win, 12, 1, "%76s", "");
                                mvwprintw(win, 13, 1, "%76s", "");
                                mvwprintw(win, 14, 1, "%76s", "");
                            }
                        } else {
                            mvwprintw(win, 4, 11, "%25s", "You're out of potions.");
                            usleep(1500000);
                        }
                    } else if (strcmp(str, "2") == 0) {
                        // use revive
                        if (revives > 0) {
                            mvwvline(win, 12, 40, ACS_VLINE, 6);
                            mvwhline(win, 12, 1, ACS_HLINE, 76);
                            mvwprintw(win, 13, 15, "%20s", "");
                            mvwprintw(win, 15, 15, "%20s", "");
                            mvwprintw(win, 17, 15, "%20s", "");
                            mvwprintw(win, 13, 52, "%20s", "");
                            mvwprintw(win, 15, 52, "%20s", "");
                            mvwprintw(win, 17, 52, "%20s", "");
                            
                            if (numPokemon >= 1) {
                                mvwprintw(win, 13, 15, "1. %s (%d/%d)%14s", heldPokemon[0].name, heldPokemon[0].hp, heldPokemon[0].initialHP, "");
                            }
                            if (numPokemon >= 2) {
                                mvwprintw(win, 15, 15, "2. %s (%d/%d)%14s", heldPokemon[1].name, heldPokemon[1].hp, heldPokemon[1].initialHP, "");
                            }
                            if (numPokemon >= 3) {
                                mvwprintw(win, 17, 15, "3. %s (%d/%d)%14s", heldPokemon[2].name, heldPokemon[2].hp, heldPokemon[2].initialHP, "");
                            }
                            if (numPokemon >= 4) {
                                mvwprintw(win, 13, 52, "4. %s (%d/%d)%14s", heldPokemon[3].name, heldPokemon[3].hp, heldPokemon[3].initialHP, "");
                            }
                            if (numPokemon >= 5) {
                                mvwprintw(win, 15, 52, "5. %s (%d/%d)%14s", heldPokemon[4].name, heldPokemon[4].hp, heldPokemon[4].initialHP, "");
                            }
                            if (numPokemon >= 6) {
                                mvwprintw(win, 17, 52, "6. %s (%d/%d)%14s", heldPokemon[5].name, heldPokemon[5].hp, heldPokemon[5].initialHP, "");
                            }
                            mvprintw(22, 0, "Please choose which pokemon to revive (must be at 0 hp): ");
                            move(22, 56);
                            revives--;
                            char str[10] = "";
                            while(strcmp(str, "") == 0) {
                                wrefresh(win);
                                getstr(str);
                                if (strcmp(str, "1") == 0) {
                                    if (heldPokemon[0].hp == 0) {
                                        heldPokemon[0].hp = heldPokemon[0].initialHP / 2;
                                        usleep(1500000);
                                    } else {
                                        cancel = 1;
                                    }
                                } else if (strcmp(str, "2") == 0) {
                                    if (heldPokemon[1].hp == 0) {
                                        heldPokemon[1].hp = heldPokemon[1].initialHP / 2;
                                        usleep(1500000);
                                    } else {
                                        cancel = 1;
                                    }
                                } else if (strcmp(str, "3") == 0) {
                                    if (heldPokemon[2].hp == 0) {
                                        heldPokemon[2].hp = heldPokemon[2].initialHP / 2;
                                        usleep(1500000);
                                    } else {
                                        cancel = 1;
                                    }
                                } else if (strcmp(str, "4") == 0) {
                                    if (heldPokemon[3].hp == 0) {
                                        heldPokemon[3].hp = heldPokemon[3].initialHP / 2;
                                        usleep(1500000);
                                    } else {
                                        cancel = 1;
                                    }
                                } else if (strcmp(str, "5") == 0) {
                                    if (heldPokemon[4].hp == 0) {
                                        heldPokemon[4].hp = heldPokemon[4].initialHP / 2;
                                        usleep(1500000);
                                    } else {
                                        cancel = 1;
                                    }
                                } else if (strcmp(str, "6") == 0) {
                                    if (heldPokemon[5].hp == 0) {
                                        heldPokemon[5].hp = heldPokemon[5].initialHP / 2;
                                        usleep(1500000);
                                    } else {
                                        cancel = 1;
                                    }
                                } else {
                                    strcpy(str, "");
                                }
                            }
                    
                            mvprintw(0, 0, "%79s", "");
                            mvwprintw(win, 12, 1, "%76s", "");
                            mvwprintw(win, 13, 1, "%76s", "");
                            mvwprintw(win, 14, 1, "%76s", "");
                        } else {
                            mvwprintw(win, 4, 11, "%25s", "You're out of revives.");
                            wrefresh(win);
                            usleep(1500000);
                        }
                    } else if (strcmp(str, "3") == 0) {
                        // use pokeball
                        if (!isWild || enemy->hp != 0) {
                            mvwprintw(win, 4, 10, "%25s", "You can't capture this pokemon.");
                            wrefresh(win);
                            usleep(1500000);
                        }
                    } else {
                        strcpy(str, "");
                    }
                }
                wrefresh(win);
                if (!cancel) {
                    attackPlayer(enemy, win, enemyMove);
                }
                usleep(500000);
            // Run
            } else if (strcmp(str, "3") == 0) { 
                if (isWild) {
                    mvwprintw(win, 4, 11, "You ran away...");
                    wrefresh(win);
                    usleep(1000000);
                    battling = 0;
                } else {
                    mvwprintw(win, 4, 11, "You can't run away from a trainer");
                    wrefresh(win);
                    usleep(1000000);
                }
            // Switch Pokemon
            } else if (strcmp(str, "4") == 0) {
                if (numPokemon > 1) {
                    mvprintw(22, 0, "Please choose which pokemon to switch to: ");

                    mvwvline(win, 12, 40, ACS_VLINE, 6);
                    mvwhline(win, 12, 1, ACS_HLINE, 76);
                    mvwprintw(win, 13, 15, "%20s", "");
                    mvwprintw(win, 15, 15, "%20s", "");
                    mvwprintw(win, 17, 15, "%20s", "");
                    mvwprintw(win, 13, 52, "%20s", "");
                    mvwprintw(win, 15, 52, "%20s", "");
                    mvwprintw(win, 17, 52, "%20s", "");
                    if (numPokemon >= 1) {
                        mvwprintw(win, 13, 15, "1. %s%14s", heldPokemon[0].name, "");
                    }
                    if (numPokemon >= 2) {
                        mvwprintw(win, 15, 15, "2. %s%14s", heldPokemon[1].name, "");
                    }
                    if (numPokemon >= 3) {
                        mvwprintw(win, 17, 15, "3. %s%14s", heldPokemon[2].name, "");
                    }
                    if (numPokemon >= 4) {
                        mvwprintw(win, 13, 52, "4. %s%14s", heldPokemon[3].name, "");
                    }
                    if (numPokemon >= 5) {
                        mvwprintw(win, 15, 52, "5. %s%14s", heldPokemon[4].name, "");
                    }
                    if (numPokemon >= 6) {
                        mvwprintw(win, 17, 52, "6. %s%14s", heldPokemon[5].name, "");
                    }
                    wrefresh(win);
                    move(22, 43);

                    int prevChoice = activePokemon;
                    refresh();
                    char str[10] = "";
                    while(strcmp(str, "") == 0) {
                        wrefresh(win);
                        getstr(str);
                        if (strcmp(str, "1") == 0) {
                            activePokemon = 0;
                            wrefresh(win);
                            usleep(1500000);
                        } else if (strcmp(str, "2") == 0) {
                            activePokemon = 1;
                            wrefresh(win);
                            usleep(1500000);
                        } else if (strcmp(str, "3") == 0) {
                            activePokemon = 2;
                            wrefresh(win);
                            usleep(1500000);
                        } else if (strcmp(str, "4") == 0) {
                            activePokemon = 3;
                            wrefresh(win);
                            usleep(1500000);
                        } else if (strcmp(str, "5") == 0) {
                            activePokemon = 4;
                            wrefresh(win);
                            usleep(1500000);
                        } else if (strcmp(str, "6") == 0) {
                            activePokemon = 5;
                            wrefresh(win);
                            usleep(1500000);
                        } else {
                            strcpy(str, "");
                        }
                        if (heldPokemon[activePokemon].hp <= 0) {                
                            mvwprintw(win, 4, 11, "You can't switch to a pokemon with 0 hp.");
                            activePokemon = prevChoice;
                            wrefresh(win);
                            usleep(1500000);
                        }
                    }

                    mvprintw(0, 0, "%79s", "");
                    mvwprintw(win, 12, 1, "%76s", "");
                    mvwprintw(win, 13, 1, "%76s", "");
                    mvwprintw(win, 14, 1, "%76s", "");
                    wrefresh(win);
                } else {
                    mvprintw(22, 0, "You have no other pokemon...");
                    refresh();
                    usleep(500000);
                }
                attackPlayer(enemy, win, enemyMove);
                usleep(500000);
            } 
        }
        noecho();
        if (heldPokemon[activePokemon].hp == 0) {
            mvwprintw(win, 4, 11, "Your %s was defeated...%30s", heldPokemon[activePokemon].name, "");
            int swap = 999;
            for (int i = 0; i < numPokemon; i++) {
                if (heldPokemon[i].hp > 0) {
                    swap = i;
                }
            }
            if (swap != 999) {
                activePokemon = swap;
            } else {
                wrefresh(win);
                battling = 0;
            }
            wrefresh(win);
            usleep(1250000);
        }
        if (enemy->hp == 0) {
            mvwprintw(win, 2, 10, "You defeated a %s!%-15s", enemy->name, "");
            if (isWild) {
                mvwprintw(win, 4, 10, "Would you like to try capture it?%20s", "");
                mvprintw(22, 0, "Please enter 1 for yes, 2 for no: ");
                move(22, 34);
                refresh();
                wrefresh(win);
                echo();
                char str[10] = "";
                while(strcmp(str, "") == 0) {
                    refresh();
                    getstr(str);
                    if (strcmp(str, "1") == 0) {
                        if (numPokemon < 6 && pokeballs > 0) {
                            pokeballs--;
                            mvwprintw(win, 4, 10, "You threw a pokeball!%20s", "");
                            wrefresh(win);
                            usleep(1500000);
                            mvwprintw(win, 4, 10, "You captured %s!%-20s", enemy->name, "");
                            wrefresh(win);
                            // copy pokemon to heldPokemon[numPokemon];
                                heldPokemon[numPokemon].pokeMoves[0].id = enemy->pokeMoves[0].id;
                                strcpy(heldPokemon[numPokemon].pokeMoves[0].identifier, enemy->pokeMoves[0].identifier);
                                heldPokemon[numPokemon].pokeMoves[0].generation_id = enemy->pokeMoves[0].generation_id;
                                heldPokemon[numPokemon].pokeMoves[0].type_id = enemy->pokeMoves[0].type_id;
                                heldPokemon[numPokemon].pokeMoves[0].power = enemy->pokeMoves[0].power;
                                heldPokemon[numPokemon].pokeMoves[0].pp = enemy->pokeMoves[0].pp;
                                heldPokemon[numPokemon].pokeMoves[0].accuracy = enemy->pokeMoves[0].accuracy;
                                heldPokemon[numPokemon].pokeMoves[0].priority = enemy->pokeMoves[0].priority;
                                heldPokemon[numPokemon].pokeMoves[0].target_id = enemy->pokeMoves[0].target_id;
                                heldPokemon[numPokemon].pokeMoves[0].damage_class_id = enemy->pokeMoves[0].damage_class_id;
                                heldPokemon[numPokemon].pokeMoves[0].effect_id = enemy->pokeMoves[0].effect_id;
                                heldPokemon[numPokemon].pokeMoves[0].effect_chance = enemy->pokeMoves[0].effect_chance;
                                heldPokemon[numPokemon].pokeMoves[0].contest_type_id = enemy->pokeMoves[0].contest_type_id;
                                heldPokemon[numPokemon].pokeMoves[0].contest_effect_id = enemy->pokeMoves[0].contest_effect_id;
                                heldPokemon[numPokemon].pokeMoves[0].super_contest_effect_id = enemy->pokeMoves[0].super_contest_effect_id;

                                heldPokemon[numPokemon].pokeMoves[1].id = enemy->pokeMoves[1].id;
                                strcpy(heldPokemon[numPokemon].pokeMoves[1].identifier, enemy->pokeMoves[1].identifier);
                                heldPokemon[numPokemon].pokeMoves[1].generation_id = enemy->pokeMoves[1].generation_id;
                                heldPokemon[numPokemon].pokeMoves[1].type_id = enemy->pokeMoves[1].type_id;
                                heldPokemon[numPokemon].pokeMoves[1].power = enemy->pokeMoves[1].power;
                                heldPokemon[numPokemon].pokeMoves[1].pp = enemy->pokeMoves[1].pp;
                                heldPokemon[numPokemon].pokeMoves[1].accuracy = enemy->pokeMoves[1].accuracy;
                                heldPokemon[numPokemon].pokeMoves[1].priority = enemy->pokeMoves[1].priority;
                                heldPokemon[numPokemon].pokeMoves[1].target_id = enemy->pokeMoves[1].target_id;
                                heldPokemon[numPokemon].pokeMoves[1].damage_class_id = enemy->pokeMoves[1].damage_class_id;
                                heldPokemon[numPokemon].pokeMoves[1].effect_id = enemy->pokeMoves[1].effect_id;
                                heldPokemon[numPokemon].pokeMoves[1].effect_chance = enemy->pokeMoves[1].effect_chance;
                                heldPokemon[numPokemon].pokeMoves[1].contest_type_id = enemy->pokeMoves[1].contest_type_id;
                                heldPokemon[numPokemon].pokeMoves[1].contest_effect_id = enemy->pokeMoves[1].contest_effect_id;
                                heldPokemon[numPokemon].pokeMoves[1].super_contest_effect_id = enemy->pokeMoves[1].super_contest_effect_id;
                            
                                strcpy(heldPokemon[numPokemon].name, enemy->name);
                                heldPokemon[numPokemon].gender = enemy->gender;
                                heldPokemon[numPokemon].shiny = enemy->shiny;
                                heldPokemon[numPokemon].level = enemy->level;
                                for (int i = 0; i < 101; i++) {
                                    heldPokemon[numPokemon].levels[i].growth_rate_id = enemy->levels[i].growth_rate_id;
                                    heldPokemon[numPokemon].levels[i].level = enemy->levels[i].level;
                                    heldPokemon[numPokemon].levels[i].experience = enemy->levels[i].experience;
                                }
                                heldPokemon[numPokemon].exp = enemy->exp;
                                heldPokemon[numPokemon].lvlRate = enemy->lvlRate;
                                heldPokemon[numPokemon].pokeId = enemy->pokeId;
                                heldPokemon[numPokemon].speciesId = enemy->speciesId;
                                heldPokemon[numPokemon].hp = enemy->hp;
                                heldPokemon[numPokemon].initialHP = enemy->initialHP;
                                heldPokemon[numPokemon].hpIV = enemy->hpIV;
                                heldPokemon[numPokemon].attack = enemy->attack;
                                heldPokemon[numPokemon].attackIV = enemy->attackIV;
                                heldPokemon[numPokemon].defense = enemy->defense;
                                heldPokemon[numPokemon].defenseIV = enemy->defenseIV;
                                heldPokemon[numPokemon].specialAttack = enemy->specialAttack;
                                heldPokemon[numPokemon].specialAttackIV = enemy->specialAttackIV;
                                heldPokemon[numPokemon].specialDefense = enemy->specialDefense;
                                heldPokemon[numPokemon].specialDefenseIV = enemy->specialDefenseIV;
                                heldPokemon[numPokemon].speed = enemy->speed;
                                heldPokemon[numPokemon].speedIV = enemy->speedIV;
                            numPokemon++;
                        } else {
                            mvwprintw(win, 4, 10, "You already have 6 pokemon!");
                            mvwprintw(win, 5, 10, "The %s fled!", enemy->name);
                        }
                    } else if (strcmp(str, "2") == 0) {
                        continue;
                    } else {
                        strcpy(str, "");
                    }
                }
                noecho();
            }

            
            mvprintw(0, 0, "%60s", "");
            mvwprintw(win, 3, 10, "%65s", "");
            wrefresh(win);
            usleep(1250000);
            battling = 0;
        }
    }

    return c;
}

void pokemonBattle(character *trainer) {

    int numDefeatedPokemon = 0;
    for (int i = 0; i < numPokemon; i++) {
        if (heldPokemon[i].hp == 0 && strcmp(heldPokemon[i].name, "") != 0) {
            numDefeatedPokemon++;
        }
    }

    int trainerPokemonDefeated = 0;
    if(numDefeatedPokemon != numPokemon && trainer->defeated == 0) {
        WINDOW *win = newwin(19, 78, 2, 1);
        refresh();
        box(win, 0, 0);
        wrefresh(win);

        mvprintw(10, 20, "!!!  You entered a battle with %c  !!!", trainer->npc);
        refresh();
        usleep(375000);
        mvprintw(10, 20, "     You entered a battle with %c     ", trainer->npc);
        refresh();
        usleep(375000);
        mvprintw(10, 20, "!!!  You entered a battle with %c  !!!", trainer->npc);
        refresh();
        usleep(375000);
        mvprintw(10, 20, "     You entered a battle with %c     ", trainer->npc);
        refresh();
        usleep(375000);
        mvprintw(10, 20, "!!!  You entered a battle with %c  !!!", trainer->npc);
        refresh();
        usleep(375000);
        mvprintw(10, 20, "     You entered a battle with %c     ", trainer->npc);
        refresh();
        usleep(375000);
        mvprintw(10, 5, "%65s", "");
        refresh();
        for (int i = 0; i < trainer->numPokemon; i++) {
            battleSequence(&trainer->heldPokemon[i], win, 0);
        }
        

        for (int i = 0; i < trainer->numPokemon; i++) {
            if (trainer->heldPokemon[i].hp == 0) {
                trainerPokemonDefeated++;
            }
        }

        if (trainerPokemonDefeated == trainer->numPokemon) {
            werase(win);
            box(win, 0, 0);
            wrefresh(win);
            mvprintw(10, 30, "!!!  You defeated %c  !!!", trainer->npc);
            refresh();
            usleep(375000);
            mvprintw(10, 30, "     You defeated %c     ", trainer->npc);
            refresh();
            usleep(375000);
            mvprintw(10, 30, "!!!  You defeated %c  !!!", trainer->npc);
            refresh();
            usleep(375000);
            mvprintw(10, 30, "     You defeated %c     ", trainer->npc);
            refresh();
            usleep(375000);
            mvprintw(10, 30, "!!!  You defeated %c  !!!", trainer->npc);
            refresh();
            usleep(375000);
            mvprintw(10, 30, "     You defeated %c     ", trainer->npc);
            refresh();
            usleep(375000);
            mvprintw(10, 5, "%65s", "");
            refresh();
            wrefresh(win);
            usleep(1500000);
            trainer->defeated = 1;
        }
    }
}

void encounterPokemon() {
    int numDefeatedPokemon = 0;
    for (int i = 0; i < numPokemon; i++) {
        if (heldPokemon[i].hp == 0 && strcmp(heldPokemon[i].name, "") != 0) {
            numDefeatedPokemon++;
        }
    }
    usleep(100000);
    refresh();
    // generate a pokemon
    if(numDefeatedPokemon != numPokemon) {
        worldPokemon *p = static_cast<worldPokemon*>(malloc(sizeof(worldPokemon)));
        createPokemon(p);

        WINDOW *win = newwin(19, 78, 2, 1);
        refresh();
        box(win, 0, 0);
        wrefresh(win);
        battleSequence(p, win, 1);

        free(p);
    }
}

void generateTrainers(terrainMap *terrainMap, int numTrainers) {
    // pick random assortment of trainers, including at least one hiker and one rival
    // unless numTrainers < 2
    int i;

    character *trainers[numTrainers];
    char trainerOptions[7] = {'r', 'h', 'p', 'w', 's', 'e', 'm'};

    for (i = 0; i < numTrainers; i++) {
        trainers[i] = static_cast<character*>(malloc(sizeof(*trainers[i])));
    }

    // Give trainers pokemon, up to 6 max
    for (i = 0; i < numTrainers - 1; i++) {
        createPokemon(&trainers[i]->heldPokemon[0]);
        int addingPokemon = 1;
        while(addingPokemon != 0) {
            int val = rand() % 10;
            if (val < 6 && addingPokemon < 6) {
                createPokemon(&trainers[i]->heldPokemon[addingPokemon]);
                addingPokemon++;
            } else {
                trainers[i]->numPokemon = addingPokemon;
                addingPokemon = 0;
            }
        }
    }


    for (i = 0; i < numTrainers; i++) {
        trainers[i]->defeated = 0;
    }

    // Fill up trainers[] with random npcs, guaranteeing the first to be a hiker and the second to be a rival, rest are random
    for (i = 0; i < numTrainers; i++) {
        if (i == 0) {
            trainers[i]->npc = 'h';
            trainers[i]->nextMoveTime = 0;
        } else if (i == 1) {
            trainers[i]->npc = 'r';
            trainers[i]->nextMoveTime = 0;
        } else {
            trainers[i]->npc = trainerOptions[rand() % 7];
            trainers[i]->nextMoveTime = 0;
        }
    }
    
    // Player for the queue to allow the user a turn to move and to redraw the map
    trainers[numTrainers - 1]->npc = '@';
    trainers[numTrainers - 1]->rowPos = terrainMap->pc.rowPos;
    trainers[numTrainers - 1]->colPos = terrainMap->pc.colPos;
    trainers[numTrainers - 1]->nextMoveTime = 0;

    // If the player has a preset position, AKA just came from another map, place it next to the corresponding exit.
    if (terrainMap->pc.preset) {
        switch(lastMove) {
            case Up :
                terrainMap->pc.rowPos = 19;
                terrainMap->pc.colPos = terrainMap->northSouthExit;
                break;
            case Down :
                terrainMap->pc.rowPos = 1;
                terrainMap->pc.colPos = terrainMap->northSouthExit;
                break;
            case Left :
                terrainMap->pc.rowPos = terrainMap->westEastExit;
                terrainMap->pc.colPos = 78;
                break;
            case Right :
                terrainMap->pc.rowPos = terrainMap->westEastExit;
                terrainMap->pc.colPos = 1;
                break;
            case None :
                break;
        }
        trainers[numTrainers - 1]->rowPos = terrainMap->pc.rowPos;
        trainers[numTrainers - 1]->colPos = terrainMap->pc.colPos;
    }

    // Place all trainers and give pacers, wanderers, and explorers, a random direction to start with
    direction_t directionOptions[4] = {Up, Down, Left, Right};
    
    for (i = 0; i < numTrainers; i++) {
        if (trainers[i]->npc != '@') {
            findPosition(trainers[i], terrainMap, numTrainers);
        }

        // Build value string to use in heap
        snprintf(trainers[i]->value, sizeof(trainers[i]->value), "%c %d", trainers[i]->npc, i);

        if (trainers[i]->npc == 'w' || trainers[i]->npc == 'p' || trainers[i]->npc == 'e' || trainers[i]->npc == 'm') {
            trainers[i]->direction = directionOptions[rand() % 4];
        }
    }

    heap characterHeap;
    heap_create(&characterHeap, 9999, NULL);

    if (terrainMap->generated == 1) {
        // AKA overwrite trainers[i] with terrainMap->trainers[i] or trainers stored from previous generation.
        for (i = 0; i < numTrainers; i++) {
            trainers[i] = terrainMap->trainers[i];
        }
    } else {
        // Mark as generated.
        terrainMap->generated = 1;
    }

    // Insert trainers into queue.
    for (i = 0; i < numTrainers; i++) {
        heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->value);
    }

    // While the queue of trainers isn't empty, dequeue the trainer with the cheapest next move, make the move, 
    //  then reinsert with old cost + next move cost
    heap_entry u;
    int dontQuit = 1;
    while(heap_delmin(&characterHeap, &u.key, &u.value) && dontQuit) {
        // move the things
        char value[20];
        char *npc;
        char *index;
        long int moveCost = 0;

        // Deconstruct Value String
        strcpy(value, (char*)u.value);
        npc = strtok(value, " ");
        index = strtok(NULL, " ");
        int i = atoi(index);

        switch(*npc) {
            case '@' : {
                usleep(250000);
                displayMap(terrainMap, numTrainers, trainers);
                if (playerNeedsPokemon) {
                    givePlayerPokemon();
                    playerNeedsPokemon = 0;
                }
                refresh();
                int c = getch();
                // mvprintw(0, 60, "You pressed %d    ", c);
                switch(c) {
                    case(102) : {
                        int flying = 1;
                        char str[10] = "";
                        mvprintw(0, 0, "You have pressed 'f', activating the fly command.");
                        mvprintw(22, 0, "Press any key to continue, 'esc' to cancel.");
                        int ch = getch();
                        if (ch == 27) {
                            flying = 0;
                            mvprintw(0, 0, "%-80s", "");
                            mvprintw(22, 0, "%-80s", "");
                        }
                        refresh();
                        while(flying) {
                            mvprintw(22, 0, "Please enter two integers from -200 to 200 separated by a space: ");
                            echo();
                            getstr(str);
                            if (strcmp(str, "") != 0) {
                                terrainMap->flyRow = atoi(strtok(str, " "));
                                terrainMap->flyCol = atoi(strtok(NULL, " "));
                                terrainMap->wantToFly = 1;
                                mvprintw(0, 0, "%-80s", "");
                                mvprintw(22, 0, "%-80s", "");
                                flying = 0;
                                dontQuit = 0;
                            }
                            noecho();
                        }
                        break;
                    }
                    case (55) :
                    case (121) : {
                        mvprintw(0, 0, "%-50s", " ");
                        int posNotOcc = positionNotOccupied(trainers[i]->rowPos - 1, trainers[i]->colPos - 1, numTrainers, trainers);
                        if (terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos - 1] != '%'
                        && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos - 1] != '^'
                        && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos - 1] != '~'
                        && notGate(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos - 1)
                        && posNotOcc == 1) {
                            trainers[i]->colPos--;
                            trainers[i]->rowPos--;
                            terrainMap->pc.rowPos--;
                            terrainMap->pc.colPos--;                
                            moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1, trainers[i]);
                        }
                        if (posNotOcc != 1) {
                            if(trainers[posNotOcc - 10]->defeated != 0) {
                                mvprintw(0, 0, "%-50s", "You've already defeated this trainer in battle...");
                            } else {
                                pokemonBattle(trainers[posNotOcc - 10]);
                            }
                        }
                        break;
                    }
                    case (56) :
                    case (107) : {
                        mvprintw(0, 0, "%-50s", " ");
                        int posNotOcc = positionNotOccupied(trainers[i]->rowPos - 1, trainers[i]->colPos, numTrainers, trainers);
                        int gateCheck = notGate(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos);

                        if (terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '%'
                        && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '^'
                        && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '~'
                        && gateCheck
                        && posNotOcc == 1) {
                            trainers[i]->rowPos--;
                            terrainMap->pc.rowPos--;
                            moveCost = getMoveCost(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos, trainers[i]);
                        }
                        if (posNotOcc != 1) {
                            if(trainers[posNotOcc - 10]->defeated != 0) {
                                mvprintw(0, 0, "%-50s", "You've already defeated this trainer in battle...");
                            } else {
                                pokemonBattle(trainers[posNotOcc - 10]);
                            }
                        }

                        if (gateCheck == 0) { // If player wants to move into gate
                            // Save trainer array to terrainMap
                            for(int k = 0; k < numTrainers; k++) {
                                terrainMap->trainers[k] = trainers[k];
                            }
                            lastMove = Up;
                            dontQuit = 0;
                        }

                        break;
                    }
                    case (57) :
                    case (117) : {
                        mvprintw(0, 0, "%-50s", " ");
                        int posNotOcc = positionNotOccupied(trainers[i]->rowPos - 1, trainers[i]->colPos + 1, numTrainers, trainers);
                        if (terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos + 1] != '%'
                        && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos + 1] != '^'
                        && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos + 1] != '~'
                        && notGate(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos + 1)
                        && posNotOcc == 1) {
                            trainers[i]->rowPos--;
                            trainers[i]->colPos++;
                            terrainMap->pc.rowPos--;
                            terrainMap->pc.colPos++;
                            moveCost = getMoveCost(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos + 1, trainers[i]);
                        }
                        if (posNotOcc != 1) {
                            if(trainers[posNotOcc - 10]->defeated != 0) {
                                mvprintw(0, 0, "%-50s", "You've already defeated this trainer in battle...");
                            } else {
                                pokemonBattle(trainers[posNotOcc - 10]);
                            }
                        }
                        break;
                    }
                    case (54) :
                    case (108) : {
                        mvprintw(0, 0, "%-50s", " ");
                        int posNotOcc = positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos + 1, numTrainers, trainers);
                        int gateCheck = notGate(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1);

                        if (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '%'
                        && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '^'
                        && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '~'
                        && gateCheck
                        && posNotOcc == 1) {
                            trainers[i]->colPos++;
                            terrainMap->pc.colPos++;
                            moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1, trainers[i]);
                        }
                        if (posNotOcc != 1) {
                            if(trainers[posNotOcc - 10]->defeated != 0) {
                                mvprintw(0, 0, "%-50s", "You've already defeated this trainer in battle...");
                            } else {
                                pokemonBattle(trainers[posNotOcc - 10]);
                            }
                        }

                        if (gateCheck == 0) { // If player wants to move into gate
                            // Save trainer array to terrainMap
                            for(int k = 0; k < numTrainers; k++) {
                                terrainMap->trainers[k] = trainers[k];
                                terrainMap->trainers[k]->rowPos = trainers[k]->rowPos;
                                terrainMap->trainers[k]->colPos = trainers[k]->colPos;
                            }
                            lastMove = Right;
                            dontQuit = 0;
                        }
                        break;
                    }
                    case (51) :
                    case (110) : {
                        mvprintw(0, 0, "%-50s", " ");
                        int posNotOcc = positionNotOccupied(trainers[i]->rowPos + 1, trainers[i]->colPos + 1, numTrainers, trainers);
                        if (terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos + 1] != '%'
                        && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos + 1] != '^'
                        && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos + 1] != '~'
                        && notGate(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos + 1)
                        && posNotOcc == 1) {
                            trainers[i]->colPos++;
                            trainers[i]->rowPos++;
                            terrainMap->pc.colPos++;
                            terrainMap->pc.rowPos++;
                            moveCost = getMoveCost(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos + 1, trainers[i]);
                        }
                        if (posNotOcc != 1) {
                            if(trainers[posNotOcc - 10]->defeated != 0) {
                                mvprintw(0, 0, "%-50s", "You've already defeated this trainer in battle...");
                            } else {
                                pokemonBattle(trainers[posNotOcc - 10]);
                            }
                        }
                        break;
                    }
                    case (50) :
                    case (106) : {
                        mvprintw(0, 0, "%-50s", " ");
                        int posNotOcc = positionNotOccupied(trainers[i]->rowPos + 1, trainers[i]->colPos, numTrainers, trainers);
                        int gateCheck = notGate(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos);

                        if (terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '%'
                        && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '^'
                        && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '~'
                        && gateCheck
                        && posNotOcc == 1) {
                            trainers[i]->rowPos++;
                            terrainMap->pc.rowPos++;
                            moveCost = getMoveCost(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos, trainers[i]);
                        }
                        if (posNotOcc != 1) {
                            if(trainers[posNotOcc - 10]->defeated != 0) {
                                mvprintw(0, 0, "%-50s", "You've already defeated this trainer in battle...");
                            } else {
                                pokemonBattle(trainers[posNotOcc - 10]);
                            }
                        }

                        if (gateCheck == 0) { // If player wants to move into gate
                            // Save trainer array to terrainMap
                            for(int k = 0; k < numTrainers; k++) {
                                terrainMap->trainers[k] = trainers[k];
                            }
                            lastMove = Down;
                            dontQuit = 0;
                        }
                        break;
                    }
                    case (49) :
                    case (98) : {
                        mvprintw(0, 0, "%-50s", " ");
                        int posNotOcc = positionNotOccupied(trainers[i]->rowPos + 1, trainers[i]->colPos - 1, numTrainers, trainers);
                        if (terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos - 1] != '%'
                        && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos - 1] != '^'
                        && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos - 1] != '~'
                        && notGate(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos - 1)
                        && posNotOcc == 1) {
                            trainers[i]->colPos--;
                            trainers[i]->rowPos++;
                            terrainMap->pc.colPos--;
                            terrainMap->pc.rowPos++;
                            moveCost = getMoveCost(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos - 1, trainers[i]);
                        }
                        if (posNotOcc != 1) {
                            if(trainers[posNotOcc - 10]->defeated != 0) {
                                mvprintw(0, 0, "%-50s", "You've already defeated this trainer in battle...");
                            } else {
                                pokemonBattle(trainers[posNotOcc - 10]);
                            }
                        }
                        break;
                    }
                    case (52) :
                    case (104) : {
                        mvprintw(0, 0, "%-50s", " ");
                        int posNotOcc = positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos - 1, numTrainers, trainers);
                        int gateCheck = notGate(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1);
                        
                        if (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '%'
                        && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '^'
                        && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '~'
                        && gateCheck
                        && posNotOcc == 1) {
                            trainers[i]->colPos--;
                            terrainMap->pc.colPos--;
                            moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1, trainers[i]);
                        }
                        if (posNotOcc != 1) {
                            if(trainers[posNotOcc - 10]->defeated != 0) {
                                mvprintw(0, 0, "%-50s", "You've already defeated this trainer in battle...");
                            } else {
                                pokemonBattle(trainers[posNotOcc - 10]);
                            }
                        }

                        if (gateCheck == 0) { // If player wants to move into gate
                            // Save trainer array to terrainMap
                            for(int k = 0; k < numTrainers; k++) {
                                terrainMap->trainers[k] = trainers[k];
                            }
                            lastMove = Left;
                            dontQuit = 0;
                        }
                        break;
                    }
                    case (53) :
                    case (32) :
                    case (46) : {
                        mvprintw(0, 0, "%-50s", "Resting...");
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos, trainers[i]);
                        break;
                    }
                    case (66) : {
                        WINDOW *win = newwin(10, 76, 4, 2);
                        box(win, 0, 0);
                        mvprintw(0, 0, "%65s", "");
                        mvwprintw(win, 0, 3, "Inventory");
                        mvwprintw(win, 2, 2, "1. Potions: %d", potions);
                        mvwprintw(win, 4, 2, "2. Revives: %d", revives);
                        mvwprintw(win, 6, 2, "3. Pokeballs: %d", pokeballs);
                        mvwprintw(win, 9, 3, "Enter 'q' to exit");
                        mvprintw(22, 0, "Press 1, 2, or 3, to use an item: ");
                        mvwvline(win, 1, 24, ACS_VLINE, 8);
                        if (numPokemon >= 1) {
                            mvwprintw(win, 2, 26, "1. %s (%d/%d)", heldPokemon[0].name, heldPokemon[0].hp, heldPokemon[0].initialHP);
                        }
                        if (numPokemon >= 2) {
                            mvwprintw(win, 4, 26, "2. %s (%d/%d)", heldPokemon[1].name, heldPokemon[1].hp, heldPokemon[1].initialHP);
                        }
                        if (numPokemon >= 3) {
                            mvwprintw(win, 6, 26, "3. %s (%d/%d)", heldPokemon[2].name, heldPokemon[2].hp, heldPokemon[2].initialHP);
                        }
                        if (numPokemon >= 4) {
                            mvwprintw(win, 2, 53, "4. %s (%d/%d)", heldPokemon[3].name, heldPokemon[3].hp, heldPokemon[3].initialHP);
                        }
                        if (numPokemon >= 5) {
                            mvwprintw(win, 4, 53, "5. %s (%d/%d)", heldPokemon[4].name, heldPokemon[4].hp, heldPokemon[4].initialHP);
                        }
                        if (numPokemon >= 6) {
                            mvwprintw(win, 6, 53, "6. %s (%d/%d)", heldPokemon[5].name, heldPokemon[5].hp, heldPokemon[5].initialHP);
                        }
                        move(22, 34);
                        refresh();
                        wrefresh(win);
                        echo();
                        char str[10] = "";
                        while(strcmp(str, "") == 0) {
                            wrefresh(win);
                            getstr(str);
                            if (strcmp(str, "1") == 0) {
                                mvprintw(22, 0, "%-65s", "Select a pokemon to use a potion on (+20 HP): ");
                                move(22, 46);
                                char str[10] = "";
                                while(strcmp(str, "") == 0) {
                                    wrefresh(win);
                                    getstr(str);
                                    if (strcmp(str, "1") == 0) {
                                        if (heldPokemon[0].hp + 20 < heldPokemon[0].initialHP) {
                                            heldPokemon[0].hp += 20;
                                        } else {
                                            heldPokemon[0].hp = heldPokemon[0].initialHP;
                                        }
                                        usleep(1500000);
                                    } else if (strcmp(str, "2") == 0) {
                                        if (heldPokemon[1].hp + 20 < heldPokemon[1].initialHP) {
                                            heldPokemon[1].hp += 20;
                                        } else {
                                            heldPokemon[1].hp = heldPokemon[1].initialHP;
                                        }
                                        usleep(1500000);
                                    } else if (strcmp(str, "3") == 0) {
                                        if (heldPokemon[2].hp + 20 < heldPokemon[2].initialHP) {
                                            heldPokemon[2].hp += 20;
                                        } else {
                                            heldPokemon[2].hp = heldPokemon[2].initialHP;
                                        }
                                        usleep(1500000);
                                    } else if (strcmp(str, "4") == 0) {
                                        if (heldPokemon[3].hp + 20 < heldPokemon[3].initialHP) {
                                            heldPokemon[3].hp += 20;
                                        } else {
                                            heldPokemon[3].hp = heldPokemon[3].initialHP;
                                        }
                                        usleep(1500000);
                                    } else if (strcmp(str, "5") == 0) {
                                        if (heldPokemon[4].hp + 20 < heldPokemon[4].initialHP) {
                                            heldPokemon[4].hp += 20;
                                        } else {
                                            heldPokemon[4].hp = heldPokemon[4].initialHP;
                                        }
                                        usleep(1500000);
                                    } else if (strcmp(str, "6") == 0) {
                                        if (heldPokemon[5].hp + 20 < heldPokemon[5].initialHP) {
                                            heldPokemon[5].hp += 20;
                                        } else {
                                            heldPokemon[5].hp = heldPokemon[5].initialHP;
                                        }
                                        usleep(1500000);
                                    } else {
                                        strcpy(str, "");
                                    }
                                }
                                potions--;
                                strcpy(str, "");
                                usleep(750000);
                            } else if (strcmp(str, "2") == 0) {
                                mvprintw(22, 0, "%-65s", "Select a pokemon to use a revive on (must be 0 HP): ");
                                move(22, 54);
                                char str[10] = "";
                                while(strcmp(str, "") == 0) {
                                    wrefresh(win);
                                    getstr(str);
                                    if (strcmp(str, "1") == 0) {
                                    if (heldPokemon[0].hp == 0) {
                                        heldPokemon[0].hp = heldPokemon[0].initialHP / 2;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "2") == 0) {
                                    if (heldPokemon[1].hp == 0) {
                                        heldPokemon[1].hp = heldPokemon[1].initialHP / 2;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "3") == 0) {
                                    if (heldPokemon[2].hp == 0) {
                                        heldPokemon[2].hp = heldPokemon[2].initialHP / 2;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "4") == 0) {
                                    if (heldPokemon[3].hp == 0) {
                                        heldPokemon[3].hp = heldPokemon[3].initialHP / 2;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "5") == 0) {
                                    if (heldPokemon[4].hp == 0) {
                                        heldPokemon[4].hp = heldPokemon[4].initialHP / 2;
                                    }
                                    usleep(1500000);
                                } else if (strcmp(str, "6") == 0) {
                                    if (heldPokemon[5].hp == 0) {
                                        heldPokemon[5].hp = heldPokemon[5].initialHP / 2;
                                    }
                                    usleep(1500000);
                                } else {
                                    strcpy(str, "");
                                }
                                }
                                revives--;
                                strcpy(str, "");
                                usleep(750000);
                            } else if (strcmp(str, "3") == 0) {
                                mvprintw(0, 0, "You can't use a pokeball outside of combat.");
                                usleep(1500000);
                            } else if (strcmp(str, "q") == 0) {
                                continue;
                            } else {
                                strcpy(str, "");
                            }
                        }
                        noecho();
                        break;
                    }
                    case (62) : {
                        mvprintw(0, 0, "Attempt to enter building    ");
                        if(terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos] == 'M' || terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos] == 'C') {
                            mvprintw(0, 0, "%-50s", "Entered Building");
                            refresh();
                            usleep(1500000);
                            int inBuilding = 1;
                            while(inBuilding) {
                                if (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos] == 'C') {
                                    for (int i = 0; i < numPokemon; i++) {
                                        mvprintw(0, 0, "Healing %s%50s", heldPokemon[i].name, "");
                                        refresh();
                                        heldPokemon[i].hp = heldPokemon[i].initialHP;
                                        usleep(1000000);
                                    }
                                    mvprintw(0, 0, "Healed all pokemon.%50s", "");
                                    refresh();
                                }
                                if (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos] == 'M') {
                                    mvprintw(0, 0, "Refilled inventory!%20s", "");
                                    pokeballs = 10;
                                    revives = 15;
                                    potions = 15;
                                    refresh();
                                }
                                mvprintw(0, 30, "%50s", "You are in the building, press '<' to exit");
                                int ch = getch();
                                if (ch == 60) {
                                    mvprintw(0, 0, "%-50s", "Exited building");
                                    mvprintw(0, 30, "%50s", " ");
                                    inBuilding = 0;
                                }
                                refresh();
                            }
                        } else {
                            mvprintw(0, 0, "%-50s", "Not standing on Building");
                        }
                        break;
                    }
                    case (116) : {
                        mvprintw(0, 0, "%-50s", "List of Nearby Trainers");
                        int displayingList = 1;
                        int scroll = 0;
                        while(displayingList) {
                            int northSouth = trainers[numTrainers - 1]->rowPos - trainers[scroll]->rowPos;

                            int westEast = trainers[numTrainers - 1]->colPos - trainers[scroll]->colPos;
                            mvprintw(0, 0, "Trainer: %c\t Number of Pokemon: %d   Distance to Player: %d %s %d %s \t\t", trainers[scroll]->npc, trainers[scroll]->numPokemon, (northSouth > 0 ? northSouth : northSouth * -1), (northSouth > 0 ? "North" : "South"), (westEast > 0 ? westEast : westEast * -1), ((westEast > 0 ? "West" : "East")));
                            mvprintw(22, 0, "Displaying Trainers: Press UP or DOWN to scroll, 'esc' to Close List");
                            refresh();
                            int c = getch();
                            if (c == 259 && (scroll - 1) >= 0 && (scroll - 1) < numTrainers - 2) { // up
                                scroll--;
                            }
                            if (c == 258 && (scroll + 1) >= 0 && (scroll + 1) < numTrainers - 2) { // down
                                scroll++;
                            }
                            if (c == 27) {
                                mvprintw(0, 0, "%80s", " ");
                                mvprintw(22, 0, "%80s", " ");
                                displayingList = 0;
                            }
                        }
                        break;
                    }
                    case (113) : {
                        mvprintw(0, 0, "%-50s", "Quitting...");
                        refresh();
                        usleep(500000);
                        terrainMap->quit = 1;
                        dontQuit = 0;
                        break;
                    }
                }
                if (moveCost < INFINITY_T) {
                    trainers[i]->nextMoveTime += moveCost;
                }
                
                if (terrainMap->terrain[terrainMap->pc.rowPos][terrainMap->pc.colPos] == ':') {
                    int num = rand() % 10;
                    if (num == 4) {
                        encounterPokemon();
                    }
                }
                for (int i = 22; i < 50; i++) {
                    for (int j = 0; j < 80; j++) {
                        mvprintw(i, j, " ");
                    }
                }
                refresh();

                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                break;
            }
            case 'r' : {
                position rivalMove = findPath(terrainMap, trainers[i]->rowPos, trainers[i]->colPos, trainers[i]);
                if (rivalMove.rowPos != terrainMap->pc.rowPos && rivalMove.colPos != terrainMap->pc.colPos) {
                    trainers[i]->rowPos = rivalMove.rowPos;
                    trainers[i]->colPos = rivalMove.colPos;
                }
                moveCost = getMoveCost(terrainMap, rivalMove.rowPos, rivalMove.colPos, trainers[i]);
                if (moveCost < INFINITY_T) {
                    trainers[i]->nextMoveTime += moveCost;
                }
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                break;
            }
            case 'h' : {
                position hikerMove = findPath(terrainMap, trainers[i]->rowPos, trainers[i]->colPos, trainers[i]);
                if (hikerMove.rowPos != terrainMap->pc.rowPos && hikerMove.colPos != terrainMap->pc.colPos) {
                    trainers[i]->rowPos = hikerMove.rowPos;
                    trainers[i]->colPos = hikerMove.colPos;
                }
                moveCost = getMoveCost(terrainMap, hikerMove.rowPos, hikerMove.colPos, trainers[i]);
                if (moveCost < INFINITY_T) {
                    trainers[i]->nextMoveTime += moveCost;
                }
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                break;
            }
            case 'p' : {
                if (trainers[i]->direction == Left) {
                    if (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '%'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '^'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '~'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != 'M'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != 'C'
                    && notGate(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1)
                    && positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos - 1, numTrainers, trainers)) {
                        trainers[i]->colPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = Right;
                    }
                }
                if (trainers[i]->direction == Right) {
                    if (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '%'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '^'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '~'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != 'M'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != 'C'
                    && notGate(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1)
                    && positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos + 1, numTrainers, trainers)) {
                        trainers[i]->colPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = Left;
                    }
                }
                if (trainers[i]->direction == Down) {
                    if (terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '%'
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '^'
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '~'
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != 'M'
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != 'C'
                    && notGate(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos)
                    && positionNotOccupied(trainers[i]->rowPos + 1, trainers[i]->colPos, numTrainers, trainers)) {
                        trainers[i]->rowPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = Up;
                    }
                }
                if (trainers[i]->direction == Up) {
                    if (terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '%'
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '^'
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '~'
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != 'M'
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != 'C'
                    && notGate(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos)
                    && positionNotOccupied(trainers[i]->rowPos - 1, trainers[i]->colPos, numTrainers, trainers)) {
                        trainers[i]->rowPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = Down;
                    }
                }
                // printf("Moved Pacer\n");
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                // printf("Inserted Pacer with nextMoveTime: %ld\n", trainers[i]->nextMoveTime);
                break;
            }
            case 'w' : {
                // move in direction until reach edge of spawn terrain then walk in random new direction
                if (trainers[i]->direction == Left) {
                    if (positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos - 1, numTrainers, trainers)
                    && notGate(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1)
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] == trainers[i]->spawn) {
                        trainers[i]->colPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Right) {
                    if (positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos + 1, numTrainers, trainers)
                    && notGate(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1)
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] == trainers[i]->spawn) {
                        trainers[i]->colPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Down) {
                    if (positionNotOccupied(trainers[i]->rowPos + 1, trainers[i]->colPos, numTrainers, trainers)
                    && notGate(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos)
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] == trainers[i]->spawn) {
                        trainers[i]->rowPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Up) {
                    if (positionNotOccupied(trainers[i]->rowPos - 1, trainers[i]->colPos, numTrainers, trainers)
                    && notGate(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos)
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] == trainers[i]->spawn) {
                        trainers[i]->rowPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                // printf("Moved Wanderer\n");
                // usleep(250000);
                // displayMap(terrainMap, numTrainers, trainers);
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                // printf("Inserted Wanderer with nextMoveTime: %ld\n", trainers[i]->nextMoveTime);
                break;
            }
            case 's' :
                // Sentries don't move
                break;
            case 'e' : {
                // move in direction until reach impassable terrain (boulder, tree, building, or water) then walk in random new direction
                if (trainers[i]->direction == Left) {
                    if (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '%'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '^'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != '~'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != 'M'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] != 'C'
                    && notGate(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1)
                    && positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos - 1, numTrainers, trainers)) {
                        trainers[i]->colPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Right) {
                    if (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '%'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '^'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != '~'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != 'M'
                    && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] != 'C'
                    && notGate(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1)
                    && positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos + 1, numTrainers, trainers)) {
                        trainers[i]->colPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Down) {
                    if (terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '%'
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '^'
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != '~'
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != 'M'
                    && terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] != 'C'
                    && notGate(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos)
                    && positionNotOccupied(trainers[i]->rowPos + 1, trainers[i]->colPos, numTrainers, trainers)) {
                        trainers[i]->rowPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Up) {
                    if (terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '%'
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '^'
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != '~'
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != 'M'
                    && terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] != 'C'
                    && notGate(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos)
                    && positionNotOccupied(trainers[i]->rowPos - 1, trainers[i]->colPos, numTrainers, trainers)) {
                        trainers[i]->rowPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                // printf("Moved Explorer\n");
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                // printf("Inserted Explorer with nextMoveTime: %ld\n", trainers[i]->nextMoveTime);
                break;
            }
            case 'm' : {
                // move in direction until reach edge of spawn terrain then walk in random new direction
                // if player is cardinally adjacent/on edge of water directly north, south, west, or east, move towards player
                if (trainers[i]->direction == Left) {
                    if (positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos - 1, numTrainers, trainers)
                    && (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] == '~'
                    || (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 1] == '#' && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos - 2] == '#'))) {
                        trainers[i]->colPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos - 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Right) {
                    if (positionNotOccupied(trainers[i]->rowPos, trainers[i]->colPos + 1, numTrainers, trainers)
                    && (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] == '~'
                    || (terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 1] == '#' && terrainMap->terrain[trainers[i]->rowPos][trainers[i]->colPos + 2] == '~'))) {
                        trainers[i]->colPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos, trainers[i]->colPos + 1, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Down) {
                    if (positionNotOccupied(trainers[i]->rowPos + 1, trainers[i]->colPos, numTrainers, trainers)
                    && (terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] == '~'
                    || (terrainMap->terrain[trainers[i]->rowPos + 1][trainers[i]->colPos] == '#' && terrainMap->terrain[trainers[i]->rowPos + 2][trainers[i]->colPos] == '#'))) {
                        trainers[i]->rowPos++;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos + 1, trainers[i]->colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                if (trainers[i]->direction == Up) {
                    if (positionNotOccupied(trainers[i]->rowPos - 1, trainers[i]->colPos, numTrainers, trainers)
                    &&(terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] == '~'
                    || (terrainMap->terrain[trainers[i]->rowPos - 1][trainers[i]->colPos] == '#' && terrainMap->terrain[trainers[i]->rowPos - 2][trainers[i]->colPos] == '#'))) {
                        trainers[i]->rowPos--;
                        moveCost = getMoveCost(terrainMap, trainers[i]->rowPos - 1, trainers[i]->colPos, trainers[i]);
                        if (moveCost < INFINITY_T) {
                            trainers[i]->nextMoveTime += moveCost;
                        }
                    } else {
                        trainers[i]->direction = directionOptions[rand() % 4];
                    }
                }
                heap_insert(&characterHeap, &trainers[i]->nextMoveTime, &trainers[i]->npc);
                break;
            }
            default :
                break;
        }
    }

    for (i = 0; i < numTrainers; i++) {
        free(trainers[i]);
    }

    heap_destroy(&characterHeap);
}

void generateTerrain(terrainMap *tM, int a, int b, int firstGeneration, int numTrainers) {
    int i, j;

    // Add border to map fill rest with short grass
    for (i = 0; i < 21; i++) {
        for (j = 0; j < 80; j++) {
            if (i == 0 || i == 20 || j == 0 || j == 79) {
                tM->terrain[i][j] = '%';
            } else {
                tM->terrain[i][j] = '.';
            }
        }
    }

    // Calculate building spawn chance
    double chance = (rand() / (RAND_MAX / 1.00));
    double bldngSpawnChance = abs(a - 200) + abs(b - 200);
    bldngSpawnChance *= -45.00;
    bldngSpawnChance /= 400.00;
    bldngSpawnChance += 50.00;
    bldngSpawnChance /= 100.00;

    generateExits(tM, a, b);
    generateTallGrass(tM->terrain);
    generateWater(tM->terrain);
    generatePaths(tM, a, b);
    if ((chance < bldngSpawnChance && chance > 0.00) || firstGeneration) {
        generateBuildings(tM, a, b);
    }
    decorateTerrain(tM->terrain);
    placeCharacter(tM);
    generateTrainers(tM, numTrainers);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int i, j;
    int quit = 0;
    int currWorldRow = 200;
    int currWorldCol = 200;
    int numTrainers = 8; // Default number of trainers = 8

    // If the user passed a parameter
    if(argv[1]) {
        if (strcmp(argv[1], "--numtrainers") == 0) {
            // Generate terrain with the number they passed
            numTrainers = atoi(argv[2]) + 1;
        }
    }

    initscr();
    keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();			/* Don't echo() while we do getch */
    start_color();
    init_pair(GRASS_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(WATER_COLOR, COLOR_BLUE, COLOR_BLACK);
    init_pair(TREE_COLOR, COLOR_BLACK, COLOR_GREEN);
    init_pair(BOULDER_COLOR, COLOR_CYAN, COLOR_BLACK);
    init_pair(ROAD_COLOR, COLOR_YELLOW, COLOR_BLACK);

    db_parse(false);

    for (i = 0; i < 401; i++) {
        for (j = 0; j < 401; j++) {
            world[i][j] = static_cast<terrainMap*>(malloc(sizeof(*world[i][j])));
        }
    }

    for (i = 0; i < 401; i++) {
        for (j = 0; j < 401; j++) {
            world[i][j]->generated = 0;
            world[i][j]->worldRow = i;
            world[i][j]->worldCol = j;
        }
    }
    
    generateTerrain(world[currWorldRow][currWorldCol], currWorldRow, currWorldCol, 1, numTrainers);

    while(!quit) {
        if (world[currWorldRow][currWorldCol]->quit) {
            quit = 1;
        }
        if (world[currWorldRow][currWorldCol]->wantToFly) {
            int newRow = world[currWorldRow][currWorldCol]->flyRow;
            int newCol = world[currWorldRow][currWorldCol]->flyCol;
            world[currWorldRow][currWorldCol]->wantToFly = 0;
            currWorldRow = newRow;
            currWorldCol = newCol;
            if (!world[currWorldRow][currWorldCol]->generated) {
                generateTerrain(world[currWorldRow][currWorldCol], currWorldRow, currWorldCol, 0, numTrainers);
            } else {
                generateTrainers(world[currWorldRow][currWorldCol], numTrainers);
            }
        }
        switch(lastMove) {
            case Up :
                currWorldRow--;
                world[currWorldRow][currWorldCol]->worldRow = currWorldRow;
                world[currWorldRow][currWorldCol]->worldCol = currWorldCol;
                if (!world[currWorldRow][currWorldCol]->generated) {
                    lastMove = Up;
                    world[currWorldRow][currWorldCol]->pc.preset = 1;
                    generateTerrain(world[currWorldRow][currWorldCol], currWorldRow, currWorldCol, 0, numTrainers);
                } else {
                    generateTrainers(world[currWorldRow][currWorldCol], numTrainers);
                }
                break;
            case Down :
                currWorldRow++;
                world[currWorldRow][currWorldCol]->worldRow = currWorldRow;
                world[currWorldRow][currWorldCol]->worldCol = currWorldCol;
                if (!world[currWorldRow][currWorldCol]->generated) {
                    lastMove = Down;
                    world[currWorldRow][currWorldCol]->pc.preset = 1;
                    generateTerrain(world[currWorldRow][currWorldCol], currWorldRow, currWorldCol, 0, numTrainers);
                } else {
                    generateTrainers(world[currWorldRow][currWorldCol], numTrainers);
                }
                break;
            case Left :
                currWorldCol--;
                world[currWorldRow][currWorldCol]->worldRow = currWorldRow;
                world[currWorldRow][currWorldCol]->worldCol = currWorldCol;
                if (!world[currWorldRow][currWorldCol]->generated) {
                    lastMove = Left;
                    world[currWorldRow][currWorldCol]->pc.preset = 1;
                    generateTerrain(world[currWorldRow][currWorldCol], currWorldRow, currWorldCol, 0, numTrainers);
                } else {
                    generateTrainers(world[currWorldRow][currWorldCol], numTrainers);
                }
                break;
            case Right :
                currWorldCol++;
                world[currWorldRow][currWorldCol]->worldRow = currWorldRow;
                world[currWorldRow][currWorldCol]->worldCol = currWorldCol;
                if (!world[currWorldRow][currWorldCol]->generated) {
                    lastMove = Right;
                    world[currWorldRow][currWorldCol]->pc.preset = 1;
                    generateTerrain(world[currWorldRow][currWorldCol], currWorldRow, currWorldCol, 0, numTrainers);
                } else {
                    generateTrainers(world[currWorldRow][currWorldCol], numTrainers);
                }
                break;
            case None:
                break;
        }
    }

    for (i = 0; i < 401; i++) {
        for (j = 0; j < 401; j++) {
            free(world[i][j]);
        }
    }

    endwin();

    return 0;
}