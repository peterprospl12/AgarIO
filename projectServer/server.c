#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>

// COMPILE IN TERMINAL WITH:
// gcc server.c -lm -lpthread -o server

//----------------------------  CONSTS AND STRUCTS  ----------------------------

const int WIDTH = 1200;
const int HEIGHT = 800;
#define PLAYERS 16
#define FOOD_N 10
const int STARTING_MASS = 1000;
const int FOOD_MASS = 200;

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

//---------------------  SENDING AND RECEIVING CELLS INFO  ----------------------------

struct Cell receive_cell(const int sock)
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

//------------------------  GLOBAL GAME STATE  -------------------------------

struct GameState game_state;
pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Cell revive_player()
{
    struct Cell cell;
    cell.mass = STARTING_MASS;
    cell.colorRGB[0] = rand() % 256;
    cell.colorRGB[1] = rand() % 256;
    cell.colorRGB[2] = rand() % 256;
    cell.x = (double)rand()/RAND_MAX * WIDTH; // number of x from 0 to WIDTH
    cell.y = (double)rand()/RAND_MAX * HEIGHT; // number of y from 0 to HEIGHT
    return cell;
}

struct Cell create_new_player(const int sock)
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
    cell.x = (double)rand()/RAND_MAX * WIDTH; // number of x from 0 to WIDTH
    cell.y = (double)rand()/RAND_MAX * HEIGHT; // number of y from 0 to HEIGHT

    game_state.players[id] = cell;
    game_state.running[id] = true;
	pthread_mutex_unlock(&game_state_mutex);
    send_cell(sock, cell);
	return cell;
}

void update_game_state(struct Cell cell)
{
	game_state.players[cell.id] = cell;
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

void remove_player(const int deleteId)
{
    pthread_mutex_lock(&game_state_mutex);
    game_state.running[deleteId] = false;
    game_state.n_players--;
    pthread_mutex_unlock(&game_state_mutex);
}

//----------------------------  GAME EVENTS  ----------------------------------

pthread_mutex_t game_events_mutex = PTHREAD_MUTEX_INITIALIZER;

double distance(int x1, int x2, int y1, int y2)
{
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

void checkEatFood(struct Cell* cell)
{
    for(int i=0;i<FOOD_N;i++)
    {
        double r1 = sqrt(cell->mass/3.14);
        double r2 = sqrt(FOOD_MASS/3.14);
        if(distance(cell->x, game_state.foods[i].x, cell->y, game_state.foods[i].y) < (r1+r2))
        {
            cell->mass += (FOOD_MASS);
            game_state.foods[i].x = (double)rand()/RAND_MAX * WIDTH;
            game_state.foods[i].y = (double)rand()/RAND_MAX * HEIGHT;
        }
    }
}

void checkCollisions(struct Cell* cell)
{
    double r1 = sqrt(cell->mass/3.14);
    for(int i=0;i<PLAYERS;i++)
    {
        if(game_state.running[i] && i != cell->id)
        {
            double r2 = sqrt(game_state.players[i].mass/3.14);
            if(distance(cell->x, game_state.players[i].x, cell->y, game_state.players[i].y) < (r1+r2))
            {
                if(cell->mass > 1.1*game_state.players[i].mass) // I kill him
                {
                    cell->mass += game_state.players[i].mass;
                    int tmp_id = game_state.players[i].id;
                    game_state.players[i] = revive_player();
                    game_state.players[i].id = tmp_id;
                }
                else if(game_state.players[i].mass > 1.1*cell->mass) // I get killed
                {
                    int tmp_id = cell->id;
                    game_state.players[i].mass += cell->mass;
                    *cell = revive_player();
                    cell->id = tmp_id;
                }
            }
        }
    }
}

//------------------------  CONNECTION HANDLER  -------------------------------

void *connection_handler(void *socket_desc)
{
	// socket descriptor
	int sock = * (int *)socket_desc;

	// create player cell
	struct Cell clientCell = create_new_player(sock);

	fprintf(stderr,"Player %d connected\n", clientCell.id);
    int delete_id = clientCell.id;
	do {
		clientCell = receive_cell(sock);
		if(clientCell.id == -1)
		{
		    fprintf(stderr, "Client quit!\n");
            break;
        }
		update_game_state(clientCell);
		send_game_state(sock);

        pthread_mutex_lock(&game_events_mutex);

        checkEatFood(&clientCell);
	    checkCollisions(&clientCell);

        pthread_mutex_unlock(&game_events_mutex);

        send_cell(sock, clientCell);
		fprintf(stderr, "Cell %d: %f %f %d\n", clientCell.id, clientCell.x, clientCell.y, clientCell.mass);
	} while(clientCell.mass >= 10); // wait till player's death

	fprintf(stderr, "Client disconnected\n");

	// remove player
	remove_player(delete_id);

	// close connection
	close(sock);
	pthread_exit(NULL);
}

//------------------------  MAIN  -------------------------------

int main(int argc, char *argv[])
{
	srand(time(NULL));
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;

	pthread_t thread_id;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(5000);

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(listenfd, 10);

	// config game state
	game_state.n_players = 0;
	for(int i=0;i<PLAYERS;i++) {
		game_state.running[i] = false;
	}

	// create foods
	for(int i = 0; i < FOOD_N; i++)
	{
		game_state.foods[i].x = (double)rand()/RAND_MAX * WIDTH;
        game_state.foods[i].y = (double)rand()/RAND_MAX * HEIGHT;
	}

	//fprintf(stderr, "%lu %lu", sizeof(struct Cell), sizeof(struct Food));
	// get players
	for (;;) {
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		fprintf(stderr, "Connection accepted\n");
		pthread_create(&thread_id, NULL, connection_handler , (void *) &connfd);
	}
}