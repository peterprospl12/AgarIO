//
// Created by tomasz on 03.04.24.
//
#include "game_state.h"
#include "communication.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct GameState game_state;

struct Cell create_new_player(int sock)
{
    pthread_mutex_lock(&game_state_mutex);
    struct Cell cell;
    int id;
    bool found_slot = false;
    for(int i=0;i<PLAYERS;i++)
    {
        if(game_state.running[i] == false)
        {
            id = i;
            game_state.running[id] = true;
            found_slot = true;
            break;
        }
    }
    if(!found_slot)
    {
        cell.id = -1;
        return cell;
    }
    cell.id = id;
    game_state.n_players++;
    cell.mass = STARTING_MASS;
    cell.colorRGB[0] = rand() % 256;
    cell.colorRGB[1] = rand() % 256;
    cell.colorRGB[2] = rand() % 256;
    cell.x = (double)rand()/RAND_MAX * WIDTH; // liczba x z zakresu od 0 do WIDTH
    cell.y = (double)rand()/RAND_MAX * HEIGHT; // liczba y z zakresu od 0 do HEIGHT
    game_state.players[id] = cell;
    game_state.running[id] = true;
    pthread_mutex_unlock(&game_state_mutex);
    send_cell(sock, cell);
    return cell;
}

void update_game_state(struct Cell cell)
{
    pthread_mutex_lock(&game_state_mutex);
    game_state.players[cell.id] = cell;
    pthread_mutex_unlock(&game_state_mutex);
}

void send_game_state(const int sock)
{
    pthread_mutex_lock(&game_state_mutex);
    send_int(sock, game_state.n_players);
    for(int i=0;i<PLAYERS;i++)
    {
        if(game_state.running[i])
        {
            send_cell(sock, game_state.players[i]);
        }
    }

    for(int i=0;i<FOOD_N;i++)
    {
        send_food(sock, game_state.foods[i]);
    }
    pthread_mutex_unlock(&game_state_mutex);
}

void remove_player(struct Cell cell, int sock)
{
    pthread_mutex_lock(&game_state_mutex);
    game_state.running[cell.id] = false;
    game_state.n_players--;
    pthread_mutex_unlock(&game_state_mutex);
}