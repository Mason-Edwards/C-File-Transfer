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
#include "utils.h"

// Function declarations
void* handle_connection(void* pclient_socket);
void* io_handler();
void login(int client_socket);
void displayUsers(int client_socket);
void uploadFile(int client_socket);
void downloadFile(int client_socket);

//Structs

typedef struct client {
	int fd;
	char name[MSGSIZE]; 
	bool isConnected;
} Client;

typedef struct file {
	char filename[FILE_NAME_SIZE];
	char owner[30]; 
	char* shared[MAXUSERS]; // should be 2d array instead of array of pointers, need to init memory for names
} File;

// Array of users connected
Client users[MAXUSERS] = {0};
File files[100] = {0};
int numUsers = 0; 
int numFiles = 0;
const char menu[] = "1. Upload File\n2. Download File\n3. Exit\n";
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
	
	// Create thread of handling use input
	pthread_t io;
	pthread_create(&io, NULL, io_handler, NULL);
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
	return EXIT_SUCCESS;
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
		recv(client_socket, msg, sizeof(msg), 0);
		
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
		
		// If the user wants to download file
		else if (strcmp(msg, "2") == 0)
		{
			downloadFile(client_socket);
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
	recv(client_socket, userName, sizeof(userName), 0);

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
	char filename[FILE_NAME_SIZE] = {0};
	int response;
	int bs = send(client_socket, initMsg, sizeof(initMsg), 0);
	
	// Get the filename
	recv(client_socket, filename, sizeof filename, 0);

	// Download the file from the socket
	downloadFileFromSocketFd(client_socket, filename);

	// Log info 
	File file;
	strcpy(file.filename, filename);

	for(int i = 0; i < numUsers; i++)
	{
		if(users[i].fd == client_socket)
		{
			strcpy(file.owner, users[i].name);
			break;
		}
	}

	memcpy(&files[numFiles], &file, sizeof(file));
	numFiles++;

	for(int i = 0; i < numFiles; i++)
	{
		printf("\nFile: %s\nOwner: %s\n", files[i].filename, files[i].owner, files[i].shared);
	}

	send(client_socket, "Finished Uploading!\n", 20, 0);

	printf("------------------------\n\n");
}


void downloadFile(int client_socket)
{
	printf("-----User Downloading File-----\n");
	char* user;
	char* avaFiles[10] = {0};
	char avaString[200] = {0};
	char selectedFile[30] = {0};
	char confirm[17]; 
	char finish[] = "File Downloaded!\n";
	int num = 0; 
	int curFiles = 0;
	
	send(client_socket, "DOWNLOADING", 12, 0);

	// Get users name using client_socket
	for(int i = 0; i < numUsers; i++)
	{
		if(users[i].fd == client_socket)
		{
			user = users[i].name;	
			break;		
		}
	}
	
	// Get all the files that they can download
	for(int i = 0; i < numFiles; i++)
	{
		//Check if theyre the owner
		if(strcmp(files[i].owner, user) == 0)
		{
			avaFiles[curFiles] = files[i].filename; 
			//strcpy(avaFiles[curFiles], files[i].filename);
			curFiles++;
			break;
		}
		
		// otherwise check if its shared with them
		for(int j = 0; j < sizeof(files[i].shared) / sizeof(files[i].shared[j]); j++)
		{
			if(strcmp(files[i].shared[j], user) == 0)
			{
				avaFiles[curFiles] = files[i].filename;
				curFiles++;
				break;
			}
		}
	}

	for(int i = 0; i < curFiles; i++)
	{
		printf("%s\n", avaFiles[i]);
		strcat(avaString, avaFiles[i]);
	}

	printf("AVASTRING  :: %s\n", avaString);
	// Send the file the user is able to download
	int bs = send(client_socket, avaString, sizeof(avaString), 0);
	printf("BYTES SENT: %d\n", bs);

	// Recieve file name from user, then send the file
	bs = recv(client_socket, selectedFile, sizeof(selectedFile), 0);
	sendFileToSocketFd(client_socket, selectedFile);

	// Once the file has been sent, wait for confirmation
	bs = recv(client_socket, confirm, sizeof(confirm), 0);
	
	// Once confirmation is recieved, we can send back and client will resume from the manu
	bs = send(client_socket, "Download Complete", 18, 0);
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

void* io_handler()
{
	char input[50];
	scanf("%s", input);

	if (strcmp(input, "close") == 0)
	{
		close(server_socket);
		exit(EXIT_SUCCESS);
	}
}