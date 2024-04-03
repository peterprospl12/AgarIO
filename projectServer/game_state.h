//
// Created by tomasz on 03.04.24.
//

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>
#include <pthread.h>
#include "communication.h"

const int WIDTH = 1200;
const int HEIGHT = 800;
#define PLAYERS 16
#define FOOD_N 10
const int STARTING_MASS = 50;
struct GameState game_state;
pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Cell {
    int id;
    double x,y;
    int mass;
    int colorRGB[3];
};

struct Food {
    double x,y;
};

struct GameState {
    struct Cell players[PLAYERS];
    bool running[PLAYERS];
    struct Food foods[FOOD_N];
    int n_players;
};


struct Cell create_new_player(int sock);
void update_game_state(struct Cell cell);
void send_game_state(const int sock);
void remove_player(struct Cell cell, int sock);

#endif //GAME_STATE_H
