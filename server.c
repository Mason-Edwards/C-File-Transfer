/* Server demo */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#include "socketinfo.h"

// Function declarations
void* handle_connection(void* pclient_socket);
void login(int client_socket);

// Menu to send to user
char menu[] = "1. File Transfer\n2. Exit\n";

int main ()
{
	// Set up the socket
	int server_socket;
	server_socket= socket (AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family= AF_INET;
	server_address.sin_port= htons(PORT);
	server_address.sin_addr.s_addr= INADDR_ANY;
	bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
	
	// Start Listening
	listen(server_socket,BACKLOG);


	printf("Waiting for connections...\n");
	while(1)
	{
		int client_socket;	
		// Accept blocks if listen backlog is empty
		client_socket=accept(server_socket,NULL,NULL);
		
		// Fork or create thread to handle connection.
		pthread_t t;
		// pthread_create passes an argument as a pointer
		// so create a pointer for the client_socket
		int *pclient_socket = malloc(sizeof(int));
		*pclient_socket = client_socket;
		pthread_create(&t, NULL, handle_connection, pclient_socket);
    	}
	
	printf("\nCLOSING SERVER");
	close(server_socket);
	return 0;
}

// the start routine of pthread_create has to return a void pointer
void* handle_connection(void *pclient_socket)
{
	// Since pclient_socket is a void pointer we need to cast it to
	// an int before we can dereference it.
	int client_socket = *((int*)pclient_socket);
	// Free the pointer because it isnt needed
	free(pclient_socket);
	
	// Infinitely loop to handle specific conenction.
	while(1)
	{
		// Recieve a message and store it into msg
		char msg[MSGSIZE];	
		recv(client_socket, &msg, sizeof(msg), 0);
		printf("MSG: %s\n", msg);
		
		// Check the contents of the msg
		
		// If the user just connected for the first time
		if(strcmp(msg, "successfulConnection") == 0)
		{
			login(client_socket);
		}

		// If the user has selected "File Transfer"
		else if (strcmp(msg, "1") == 0)
		{
			send(client_socket, "confirm", 8, 0);
		}

		// If the user has selected "Exit"
		else if (strcmp(msg, "2") == 0)
		{
			send(client_socket, "EXITING", 8, 0);
			close(client_socket);
			break;
		}
	}
}

void login(int client_socket)
{
	char userName[MSGSIZE];	
	char loginMsg[] = "Welcome!\nPlease enter login info: ";
	char confirm[MSGSIZE];
	
	// Send the user the login message	
	send(client_socket, loginMsg, sizeof(loginMsg), 0);
	
	// Recieve the users login name
	recv(client_socket, &userName, sizeof(userName), 0);

	// Print logged in user on server
	printf("User Logged in: %s\n", userName);
	
	snprintf(confirm, sizeof confirm, "Logged in as %s!\n%s", userName, menu); 
	
	// Send user logged in confirmation and menu
	send(client_socket, confirm, sizeof(confirm), 0);
}
