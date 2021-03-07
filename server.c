/* Server demo */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "socketinfo.h"


/** Returns true on success, or false if there was an error */
bool SetSocketBlockingEnabled(int fd, bool blocking)
{
   if (fd < 0) return false;

#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return false;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif
}

// Function declarations
void* handle_connection(void* pclient_socket);
void login(int client_socket);
void displayUsers(int client_socket);
void uploadFile(int client_socket);

//Structs

typedef struct client {
	int fd;
	char name[MSGSIZE]; // could use char**?
	bool isConnected;
} Client;

// Menu to send to user

// Array of users connected
Client users[MAXUSERS] = {0};
int numUsers = 0; 
const char menu[] = "1. Upload File\n2. Display Users\n3. Exit\n";
int server_socket;

int main ()
{
	// Set up the socket
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
		//printf("MSG: %s\n", msg);
		
		// Check the contents of the msg
		
		// If the user just connected for the first time
		if(strcmp(msg, "successfulConnection") == 0)
		{
			login(client_socket);
		}

		// If the user has selected "Upload File"
		else if (strcmp(msg, "1") == 0)
		{
			uploadFile(client_socket);
		}
		
		// Display Users
		else if (strcmp(msg, "2") == 0)
		{
			displayUsers(client_socket);
		}

		// If the user has selected "Exit"
		else if (strcmp(msg, "3") == 0)
		{
			send(client_socket, "EXITING", 8, 0);
			for(int i = 0; i < numUsers; i++)
			{
				if(users[i].fd == client_socket)
				{
					users[i].isConnected = 0;
					printf("User has disconnected: %s\n\n", users[i].name);
					break;
				}
			}
			close(client_socket);
			break;
		}
	}
}

void login(int client_socket)
{
	char userName[MSGSIZE] = {0};	
	const char loginMsg[] = "Welcome!\nPlease enter login info: ";
	char confirm[MSGSIZE] = {0};
	
	
	// Send the user the login message	
	send(client_socket, loginMsg, sizeof(loginMsg), 0);
	
	// Recieve the users login name
	recv(client_socket, &userName, sizeof(userName), 0);

	// Store the clients file descriptor and name
	// then add it to the users array
	Client client;
	client.fd = client_socket;
	client.isConnected = 1;
	strcpy(client.name, userName);

	users[numUsers] = client;
	numUsers++;	

	// Print logged in user on server
	printf("User Logged in: %s\n\n", userName);
	
	snprintf(confirm, sizeof confirm, "\nLogged in as %s!\n\n%s", userName, menu); 
	
	// Send user logged in confirmation and menu
	send(client_socket, confirm, sizeof confirm, 0);
}

void uploadFile(int client_socket)
{
	printf("-----Uploading File-----\n");
	
	char* initMsg = "UPLOAD";
	char filename[30] = {0};
	char data[DATASIZE] = {0};
	int response;
	int bs = send(client_socket, initMsg, sizeof(initMsg), 0);
	//printf("BS: %d\n", bs);
	
	// Get the filename
	recv(client_socket, filename, sizeof filename, 0);
	printf("Filename: %s\n", filename);
	// Create the file
	FILE* fp = fopen(filename, "w");

	if(fp == NULL) printf("ERROR OPENING FILE\n");

	int flags = fcntl(client_socket, F_GETFL, 0);
	if (flags == -1) printf("ERROR GETTING SOCKET FLAGS\n");
	flags = (flags | O_NONBLOCK);
	int status = fcntl(client_socket, F_SETFL, flags);
	if(status == -1) printf("ERROR SETTING SOCKET TO NON BLOCKING 2\n");

	while(1)
	{
		
		response = recv(client_socket, data, sizeof(data), 0);
		int errnum;
		errnum = errno;
		//fprintf(stderr, "Value of errno %d\n", errno);
		//perror("Error Printed by Perror\n");
		//fprintf(stderr, "Error Opening File %s\n", strerror(errnum));

		printf("res: %d\n", response);
		
		// If no msgs are avaliable or there is an error then break
		if(response <= 0) 
		{
			printf("End of Stream\n");
			break;
		}
	}
			
	
	
	
	flags = (flags & ~O_NONBLOCK);
	status = fcntl(client_socket, F_SETFL, flags);
	if(status == -1) printf("ERROR SETTING SOCKET TO BLOCKING \n");

	fprintf(fp, "%s", data);
	printf("Done Writing\n");
	fflush(fp);
	fclose(fp);
	send(client_socket, "Finished Uploading!\n", 20, 0);

	printf("------------------------\n\n");
}


void displayUsers(int client_socket)
{
	char bigmsg[MSGSIZE] = {0};
	char msg[50] = {0};
	for(int i = 0; i < numUsers; i++)
	{
		if(users[i].isConnected == 1)
		{
			snprintf(msg, sizeof msg, "Username: %s | FD: %d\n", users[i].name, users[i].fd);
			strncat(bigmsg, msg, sizeof msg);
		}
	}
	int bs = send(client_socket, bigmsg, sizeof bigmsg, 0);
	printf("BYTES SENT: %d\n", bs );
}