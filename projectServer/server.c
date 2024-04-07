#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>
#include <arpa/inet.h>

// COMPILE IN TERMINAL WITH:
// gcc server.c -lm -lpthread -o server

//----------------------------  CONSTS AND STRUCTS  ----------------------------

const int WIDTH = 1200;
const int HEIGHT = 800;
#define PLAYERS 16
#define FOOD_N 10
const int STARTING_MASS = 1000;
const int FOOD_MASS = 200;
const int MASS_BOOST = 10;

bool isInsideAnyPlayer(double x, double y, int radius);

struct Cell {
    int id;
    double x, y;
    int mass;
    int colorRGB[3];
};

struct Food {
    double x, y;
};

struct GameState {
    struct Cell players[PLAYERS];
    bool running[PLAYERS];
    bool is_dead[PLAYERS];
    struct Food foods[FOOD_N];
    int n_players;
};

//---------------------  SENDING AND RECEIVING CELLS INFO  ----------------------------

struct Cell receive_cell(const int sock) {
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

bool send_cell(const int sock, const struct Cell cell) {
    char buffer[sizeof(struct Cell)];
    memcpy(buffer, &cell, sizeof(struct Cell));

    const ssize_t bytes_sent = send(sock, buffer, sizeof(buffer), 0);

    // check if message was sent
    if (bytes_sent <= 0) {
        fprintf(stderr, "Error sending message\n");
        return false;
    }
    return true;
}

bool send_int(const int sock, const int n) {
    char buffer[sizeof(int)];
    memcpy(buffer, &n, sizeof(int));

    const ssize_t bytes_sent = send(sock, buffer, sizeof(buffer), 0);

    // check if message was sent
    if (bytes_sent <= 0) {
        fprintf(stderr, "Error sending message\n");
        return false;
    }
    return true;
}

bool send_food(const int sock, const struct Food food) {
    char buffer[sizeof(struct Food)];
    memcpy(buffer, &food, sizeof(struct Food));

    const ssize_t bytes_sent = send(sock, buffer, sizeof(buffer), 0);

    // check if message was sent
    if (bytes_sent <= 0) {
        fprintf(stderr, "Error sending message\n");
        return false;
    }
    return true;
}

//------------------------  GLOBAL GAME STATE  -------------------------------

struct GameState game_state;
pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Cell revive_player() {
    struct Cell cell;
    cell.mass = STARTING_MASS;
    cell.colorRGB[0] = rand() % 256;
    cell.colorRGB[1] = rand() % 256;
    cell.colorRGB[2] = rand() % 256;
    int its = 20;
    do {
        cell.x = (double) rand() / RAND_MAX * WIDTH; // liczba x z zakresu od 0 do WIDTH
        cell.y = (double) rand() / RAND_MAX * HEIGHT; // liczba y z zakresu od 0 do HEIGHT
    } while (isInsideAnyPlayer(cell.x, cell.y, 100) && its-- > 0);
    return cell;
}

struct Cell create_new_player(const int sock) {
    pthread_mutex_lock(&game_state_mutex);
    struct Cell cell;
    int id;
    bool found_slot = false;
    for (int i = 0; i < PLAYERS; i++) {
        if (game_state.running[i] == false) {
            id = i;
            game_state.running[id] = true;
            found_slot = true;
            break;
        }
    }
    if (!found_slot) {
        fprintf(stderr, "No free player slots\n");
        close(sock);
        pthread_exit(NULL);
    }
    cell.id = id;
    game_state.n_players++;

    cell.mass = STARTING_MASS;
    cell.colorRGB[0] = rand() % 256;
    cell.colorRGB[1] = rand() % 256;
    cell.colorRGB[2] = rand() % 256;
    int its = 0;
    do {
        cell.x = (double) rand() / RAND_MAX * WIDTH; // liczba x z zakresu od 0 do WIDTH
        cell.y = (double) rand() / RAND_MAX * HEIGHT; // liczba y z zakresu od 0 do HEIGHT
    } while (isInsideAnyPlayer(cell.x, cell.y, 100) && its++ < 20);
    game_state.players[id] = cell;
    game_state.running[id] = true;
    pthread_mutex_unlock(&game_state_mutex);
    if (!send_cell(sock, cell)) {
        fprintf(stderr, "Error sending cell structure!\n");
        close(sock);
        pthread_exit(NULL);
    }
    return cell;
}

void update_game_state(struct Cell cell) {
    //pthread_mutex_lock(&game_state_mutex);
    game_state.players[cell.id] = cell;
    //pthread_mutex_unlock(&game_state_mutex);
}

bool send_game_state(const int sock) {
    //pthread_mutex_lock(&game_state_mutex);
    if (!send_int(sock, game_state.n_players)) {
        fprintf(stderr, "Error sending int value!\n");
        return false;
    }
    for (int i = 0; i < PLAYERS; i++) {
        if (game_state.running[i]) {
            if (!send_cell(sock, game_state.players[i])) {
                fprintf(stderr, "Error sending cell structure!\n");
                return false;
            }
        }
    }
    for (int i = 0; i < FOOD_N; i++) {
        if (!send_food(sock, game_state.foods[i])) {
            fprintf(stderr, "Error sending food structure!\n");
            return false;
        }
    }
    //pthread_mutex_unlock(&game_state_mutex);
    return true;
}

void remove_player(const int deleteId) {
    pthread_mutex_lock(&game_state_mutex);
    game_state.running[deleteId] = false;
    game_state.n_players--;
    pthread_mutex_unlock(&game_state_mutex);
}

//----------------------------  GAME EVENTS  ----------------------------------

pthread_mutex_t check_food_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t check_collision_mutex = PTHREAD_MUTEX_INITIALIZER;

double distance(int x1, int x2, int y1, int y2) {
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

bool isInsideAnyPlayer(double x, double y, int radius) {
    for (int i = 0; i < PLAYERS; i++) {
        if (game_state.running[i]) {
            double r = sqrt(game_state.players[i].mass / 3.14);
            if (distance(x, game_state.players[i].x, y, game_state.players[i].y) < (r + radius)) {
                return true;
            }
        }
    }
    return false;
}

void checkEatFood(struct Cell *cell) {
    pthread_mutex_lock(&check_food_mutex);
    for (int i = 0; i < FOOD_N; i++) {
        double r1 = sqrt(cell->mass / 3.14);
        double r2 = sqrt(FOOD_MASS / 3.14);
        if (distance(cell->x, game_state.foods[i].x, cell->y, game_state.foods[i].y) < (r1 + r2)) {
            cell->mass += MASS_BOOST * FOOD_MASS;
            int its = 20;
            do {
                game_state.foods[i].y = (double) rand() / RAND_MAX * HEIGHT;
                game_state.foods[i].x = (double) rand() / RAND_MAX * WIDTH;
            } while (isInsideAnyPlayer(game_state.foods[i].x, game_state.foods[i].y, 5) && its-- > 0);
        }
    }
    pthread_mutex_unlock(&check_food_mutex);
}

int max(int a, int b) { return a > b ? a : b; }

void checkCollisions(struct Cell *cell) {
    pthread_mutex_lock(&check_collision_mutex);
    double r1 = sqrt(cell->mass / 3.14);
    for (int i = 0; i < PLAYERS; i++) {
        if (game_state.running[i] && i != cell->id) {
            double r2 = sqrt(game_state.players[i].mass / 3.14);
            if (distance(cell->x, game_state.players[i].x, cell->y, game_state.players[i].y) < max(r1, r2)) {
                if (cell->mass > 1.1 * game_state.players[i].mass) // I kill him
                {
                    cell->mass += game_state.players[i].mass;
                    fprintf(stderr, "Player %d killed Player %d\n", cell->id, game_state.players[i].id);
                    pthread_mutex_lock(&game_state_mutex);
                    game_state.is_dead[i] = true;
                    pthread_mutex_unlock(&game_state_mutex);
                }
            }
        }
        pthread_mutex_unlock(&check_collision_mutex);
    }
}
//------------------------  CONNECTION HANDLER  -------------------------------

void *connection_handler(void *socket_desc) {
    // socket descriptor
    int sock = *(int *) socket_desc;

    // create player cell
    struct Cell clientCell = create_new_player(sock);

    fprintf(stderr, "Player %d connected\n", clientCell.id);
    int delete_id = clientCell.id;
    do {
        clientCell = receive_cell(sock);
        if (clientCell.id == -1) {
            fprintf(stderr, "Client quit!\n");
            break;
        }
        if (!send_game_state(sock))
            break;
        checkEatFood(&clientCell);
        if (game_state.is_dead[clientCell.id]) {
            int tmp_id = clientCell.id;
            clientCell = revive_player();
            clientCell.id = tmp_id;
            game_state.is_dead[clientCell.id] = false;
            update_game_state(clientCell);
            //sleep(1);
        } else {
            checkCollisions(&clientCell);
        }
        update_game_state(clientCell);
        if (!send_cell(sock, clientCell))
            break;
        // fprintf(stderr, "Cell %d: %f %f %d\n", clientCell.id, clientCell.x, clientCell.y, clientCell.mass);
    } while (clientCell.mass >= 10); // wait till player's death

    fprintf(stderr, "Client disconnected\n");

    // remove player
    remove_player(delete_id);

    // close connection
    close(sock);
    pthread_exit(NULL);
}

//------------------------  MAIN  -------------------------------

int main(int argc, char *argv[]) {
    FILE *f = fopen("/dev/urandom", "r");
    unsigned int seed;
    fread(&seed, sizeof(seed), 1, f);
    fclose(f);
    srand(seed);
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    pthread_t thread_id;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);


    int bind_result = bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (bind_result == -1) {
        fprintf(stderr, "Server connection error\n");
        exit(EXIT_FAILURE);
    }

    listen(listenfd, 10);

    // config game state
    game_state.n_players = 0;
    for (int i = 0; i < PLAYERS; i++) {
        game_state.running[i] = false;
        game_state.is_dead[i] = false;
    }

    // create foods
    for (int i = 0; i < FOOD_N; i++) {
        game_state.foods[i].x = (double) rand() / RAND_MAX * WIDTH;
        game_state.foods[i].y = (double) rand() / RAND_MAX * HEIGHT;
    }

    //fprintf(stderr, "%lu %lu", sizeof(struct Cell), sizeof(struct Food));
    // get players
    fprintf(stderr, "Starting server\n");
    for (;;) {
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        fprintf(stderr, "Connection accepted\n");
        pthread_create(&thread_id, NULL, connection_handler, (void *) &connfd);
    }
}