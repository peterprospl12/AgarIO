//
// Created by tomasz on 03.04.24.
//
#include "cell_communication.h"
#include "game_state.h"
#include <stdio.h>
#include <string.h>

struct Cell recieve_cell(const int sock)
{
    struct Cell cell;
    char buffer[sizeof(struct Cell)];

    const ssize_t bytes_received = recv(sock, buffer, sizeof(buffer), 0);

    // check if anything was sent
    if (bytes_received <= 0) {
        cell.mass = 0;
        cell.x = 0;
        cell.y = 0;
        cell.id = -1;
        fprintf(stderr, "Error receiving player cell!\n");
        return cell;
    }

    memcpy(&cell, buffer, sizeof(struct Cell));

    return cell;
}

void send_cell(const int sock, const struct Cell cell)
{
    char buffer[sizeof(struct Cell)];
    memcpy(buffer, &cell, sizeof(struct Cell));

    const ssize_t bytes_sent = send(sock, buffer, sizeof(buffer), 0);

    // check if message was sent
    if (bytes_sent <= 0) {
        fprintf(stderr, "Error sending message\n");
    }
}

void send_int(const int sock, const int n)
{
    char buffer[sizeof(int)];
    memcpy(buffer, &n, sizeof(int));

    const ssize_t bytes_sent = send(sock, buffer, sizeof(buffer), 0);

    // check if message was sent
    if (bytes_sent <= 0) {
        fprintf(stderr, "Error sending message\n");
    }
}

void send_food(const int sock, const struct Food food)
{
    char buffer[sizeof(struct Food)];
    memcpy(buffer, &food, sizeof(struct Food));

    const ssize_t bytes_sent = send(sock, buffer, sizeof(buffer), 0);

    // check if message was sent
    if (bytes_sent <= 0) {
        fprintf(stderr, "Error sending message\n");
    }
}