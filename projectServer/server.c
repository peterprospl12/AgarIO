#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

// COMPILE IN TERMINAL WITH: gcc server.c -lpthread -o server
// JAK DODASZ NAGLOWKOWE?
// gcc server.c game_state.c communication.c -lpthread -o server
//----------------------------  CONSTS AND STRUCTS  ----------------------------

const int WIDTH = 1200;
const int HEIGHT = 800;
#define PLAYERS 16
#define FOOD_N 10
const int STARTING_MASS = 50;

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

//---------------------  SENDING AND RECIEVING CELLS INFO  ----------------------------

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

//------------------------  GLOBAL GAME STATE  -------------------------------

struct GameState game_state;
pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

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

//------------------------  CONNECTION HANDLER  -------------------------------

void *connection_handler(void *socket_desc)
{
	// socket descriptor
	int sock = * (int *)socket_desc;

	// create player cell
	struct Cell clientCell = create_new_player(sock);

	fprintf(stderr,"Player %d connected\n", clientCell.id);

	do {
		clientCell = recieve_cell(sock);
		if(clientCell.id == -1)
		{
            break;
        }
		update_game_state(clientCell);
		send_game_state(sock);
		fprintf(stderr, "Cell %d: %f %f %d\n", clientCell.id, clientCell.x, clientCell.y, clientCell.mass);
	} while(clientCell.mass >= 10); // wait till player's death

	fprintf(stderr, "Client disconnected\n");

	// remove player
	remove_player(clientCell, sock);

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

	// create semaphore
	pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

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