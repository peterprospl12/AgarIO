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

//----------------------------  CONSTS AND STRUCTS  ----------------------------

const int WIDTH = 1200;
const int HEIGHT = 800;
#define PLAYERS 16
const int STARTING_MASS = 50;

struct Cell {
	int id;
	double x,y;
	int mass;
	int colorRGB[3];
	bool running;
};

struct GameState {
	struct Cell players[PLAYERS];
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

//------------------------  GLOBAL GAME STATE  -------------------------------

struct GameState game_state;
pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_game_state(struct Cell cell)
{
	pthread_mutex_lock(&game_state_mutex);


	pthread_mutex_unlock(&game_state_mutex);
}

//------------------------  CONNECTION HANDLER  -------------------------------

void *connection_handler(void *socket_desc)
{
	// socket descriptor
	int sock = * (int *)socket_desc;

	// create player cell
	int id;
	game_state.n_players++;

	fprintf(stderr,"Player %d connected\n", id);

	struct Cell clientCell;

	// find first free player slot
	bool found_slot = false;
	for(int i=0;i<PLAYERS;i++)
	{
		if(game_state.players[i].running == false)
		{
			id = i;
			clientCell.id = id;
			clientCell.mass = STARTING_MASS;
			clientCell.colorRGB[0] = rand() % 256;
			clientCell.colorRGB[1] = rand() % 256;
			clientCell.colorRGB[2] = rand() % 256;
			clientCell.x = (double)rand()/RAND_MAX * WIDTH; // liczba x z zakresu od 0 do WIDTH
			clientCell.y = (double)rand()/RAND_MAX * HEIGHT; // liczba y z zakresu od 0 do HEIGHT
			game_state.players[i] = clientCell;
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

	send_cell(sock, clientCell);

	do {
		clientCell = recieve_cell(sock);
		update_game_state(clientCell);
		fprintf(stderr, "Cell %d: %f %f %d\n", id, clientCell.x, clientCell.y, clientCell.mass);
	} while(clientCell.mass >= 10); // wait till player's death

	fprintf(stderr, "Client disconnected\n");

	close(sock);
	pthread_exit(NULL);
}

//------------------------  MAIN  -------------------------------

int main(int argc, char *argv[])
{
	srand(time(NULL)); game_state.n_players = 0;
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

	pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

	for(int i=0;i<PLAYERS;i++) {
		game_state.players[i].id = false;
	}
	for (;;) {
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		fprintf(stderr, "Connection accepted\n");
		pthread_create(&thread_id, NULL, connection_handler , (void *) &connfd);
	}
}