/* Server demo */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define MSGSIZE 256

void sendWelcomeMsg(int client_socket)
{
	char response[] = "Welcome!\nPlease enter login info: ";
	send(client_socket, response, sizeof(response), 0);
}

void login(int client_socket)
{
	char msg[MSGSIZE];	
	sendWelcomeMsg(client_socket);
	recv(client_socket, &msg, sizeof(msg), 0);

	printf("Logged in user: %s\n", msg);
	char confirm[100];
	snprintf(confirm, sizeof confirm, "Logged in as %s!\n1: File Transfer", msg); 
	send(client_socket, confirm, sizeof(confirm), 0);
}

void *handle_connection(void *pclient_socket)
{
	int client_socket = *((int*)pclient_socket);
	free(pclient_socket);
	while(1)
	{
		char msg[MSGSIZE];	
		recv(client_socket, &msg, sizeof(msg), 0);
		
		printf("MSG: %s\n", msg);
		if(strcmp(msg, "connected") == 0)
		{
			login(client_socket);
		}
		else if (strcmp(msg, "1") == 0)
		{
			send(client_socket, "confirm", 8, 0);
		}
	}
}

int main ()
{
	// Set up the socket
	int server_socket;
	server_socket= socket (AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_address;
	server_address.sin_family= AF_INET;
	server_address.sin_port= htons(9002);
	server_address.sin_addr.s_addr= INADDR_ANY;
	bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
	
	// Start Listening
	listen(server_socket,5);
	printf("Waiting for connections...\n");
			

	while(1)
	{
		int client_socket;	
		// Accept blocks if listen backlog is empty
		client_socket=accept(server_socket,NULL,NULL);
		// Fork or create thread to handle connection.
		//handle_connection(client_socket);
		pthread_t t;
		int *pclient_socket = malloc(sizeof(int));
		*pclient_socket = client_socket;
		pthread_create(&t, NULL, handle_connection, pclient_socket);
		printf("Loop\n");

    	}

	close(server_socket);
	return 0;
}
