//
// Created by tomasz on 03.04.24.
//

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "game_state.h"

struct Cell recieve_cell(const int sock);
void send_cell(const int sock, const struct Cell cell);
void send_int(const int sock, const int n);
void send_food(const int sock, const struct Food food);

#endif //COMMUNICATION_H
