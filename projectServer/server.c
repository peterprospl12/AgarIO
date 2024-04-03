#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// COMPILE IN TERMINAL WITH: gcc server.c -lpthread -o server

//----------------------------  CONSTS AND STRUCTS  ----------------------------

const int X = 1000;

pthread_mutex_t game_state_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Cell {
	int id;
	double x,y;
	int mass;
};

struct GameState {
	struct Cell players[16];
	int n_players;
};

//---------------------  SENDING AND RECIEVING CELLS  ----------------------------

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
	// Skopiuj dane z bufora do struktury Message
	memcpy(&cell, buffer, sizeof(struct Cell));

	return cell;
}

void send_cell(const int sock, const struct Cell cell)
{
	char buffer[sizeof(struct Cell)];
	memcpy(buffer, &cell, sizeof(struct Cell));

	const ssize_t bytes_sent = send(sock, buffer, sizeof(buffer), 0);

	if (bytes_sent <= 0) {
		fprintf(stderr, "Error sending message\n");
	}
}

//------------------------  GLOBAL GAME STATE  -------------------------------

struct GameState game_state;

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
	const int id = game_state.n_players;
	game_state.n_players++;

	char message[256];
	sprintf(message,"Welcome player with id %d\n", id);
	write(sock, message, strlen(message));
	fprintf(stderr,"Player %d connected\n", id);

	struct Cell clientCell;
	clientCell.id = id;
	clientCell.mass = 10;
	clientCell.x = (double)rand()/RAND_MAX * 2*X - X; // liczba x z zakresu -X do X
	clientCell.y = (double)rand()/RAND_MAX * 2*X - X; // liczba y z zakresu -X do X

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

	for (;;) {
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		fprintf(stderr, "Connection accepted\n");
		pthread_create(&thread_id, NULL, connection_handler , (void *) &connfd);
	}
}