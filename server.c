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
void shareFile(int client_socket);

//Structs

typedef struct client {
	int fd;
	char name[MSGSIZE]; 
	bool isConnected;
} Client;

typedef struct clientPerms {
	char name[MSGSIZE];
	char perms[5];
} ClientPerms;

typedef struct file {
	char filename[FILE_NAME_SIZE];
	char owner[30]; 
	int numShared;
	ClientPerms shared[MAXUSERS];
} File;

// Array of users connected
Client users[MAXUSERS] = {0};
// Files Uploaded
File files[100] = {0};
int numUsers = 0; 
int numFiles = 0;
const char menu[] = "1. Upload File\n2. Download File\n3. Share File\n4.Exit\n";
int server_socket;

// Mutex Locks
pthread_mutex_t usersLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t filesLock = PTHREAD_MUTEX_INITIALIZER;

int main ()
{
	// Set up the socket
	server_socket= socket (AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family= AF_INET;
	server_address.sin_port= htons(PORT);
	server_address.sin_addr.s_addr= INADDR_ANY;
	bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
	
	// Create thread of handling user input
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
			if(numFiles == 0)
			{ 
				send(client_socket, "No files on server...\n", 24, 0);
			}
			else shareFile(client_socket);
		}
		
		else if (strcmp(msg, "4") == 0)
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

newusername:
	// Recieve the users login name
	recv(client_socket, userName, sizeof(userName), 0);

	// If the login name is taken, ask for new one
	for(int i = 0; i < numUsers; i++)
	{
		if(strcmp(users[i].name, userName) == 0)
		{
			send(client_socket, "NEWUSERNAME", 12, 0);
			goto newusername;
		}
	}

	// Store the clients file descriptor and name
	// then add it to the users array
	Client client;
	client.fd = client_socket;
	client.isConnected = 1;
	strcpy(client.name, userName);

	pthread_mutex_lock(&usersLock);
	users[numUsers] = client;
	numUsers++;
	pthread_mutex_unlock(&usersLock);


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
start:
	// Get the filename
	recv(client_socket, filename, sizeof filename, 0);
	
	// Check if the file already exists
	if(fopen(filename, "r") != NULL)
	{
		send(client_socket, "FILEEXISTS", 11, 0);
		memset(filename, 0, sizeof filename);
		goto start;
	}

	// After first goto it doesnt block on recv, so if the name is null then goto start again
	// and then in should block
	if(filename[0] == '\0') goto start;
	
	// Satisfy client recv
	send(client_socket, " ", 2, 0);
	
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

	pthread_mutex_lock(&filesLock);
	memcpy(&files[numFiles], &file, sizeof(file));
	numFiles++;
	pthread_mutex_unlock(&filesLock);

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
	int bs;
	char* user;
	char* avaFiles[10] = {0};
	char avaString[200] = {0};
	char selectedFile[30] = {0};
	char confirm[17]; 
	char perms[5];
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
			if(strcmp(files[i].shared[j].name, user) == 0)
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
	bs = send(client_socket, avaString, sizeof(avaString), 0);
	
	printf("BYTES SENT: %d\n", bs);

	
	printf("BYTES SENT: %d\n", bs);

	// Recieve file name from user, then send the file
	bs = recv(client_socket, selectedFile, sizeof(selectedFile), 0);
	sendFileToSocketFd(client_socket, selectedFile);

	// Once the file has been sent, wait for confirmation
	bs = recv(client_socket, confirm, sizeof(confirm), 0);
	
	// Get the users permissions
	for(int i = 0; i < numFiles; i++)
	{
		// Get the selected file
		if(strcmp(files[i].filename, selectedFile) == 0)
		{
			// Loop though all the shared users
			for(int j = 0; i < files[i].numShared; i++)
			{
				// If the user is found
				if(strcmp(files[i].shared[j].name, user) == 0)
				{
					// Store their permissions
					strncpy(perms, files[i].shared[j].perms, sizeof(perms));
				}
			}
		}
	}
	// Once confirmation is recieved, send permissions
	bs = send(client_socket, perms, sizeof perms, 0);


	// Once confirmation is recieved, we can send back and client will resume from the manu
	bs = send(client_socket, "Download Complete", 18, 0);
}

void shareFile(int client_socket)
{
	int bs;
	char file[MSGSIZE];
	char selUser[MSGSIZE];
	char perms[5];
	char* init = "SHAREFILE";
	char* fileNotExist = "FILENOTEXIST";
	char* notOwner ="NOTOWNER";
	_Bool isOwner = 0;
	_Bool isFound = 0;
	char* username;
	int idx;
	
	printf("-----User Sharing File Permissions ------\n");
	fflush(stdout);

	bs = send(client_socket, init, strlen(init), 0);
start:
	// memset file in case of goto
	memset(file, 0, sizeof file);

	// Get the file the user wants to add or remove user permissions from
	bs = recv(client_socket, file, sizeof(file), 0);

	// If the user chose to quit
	if(file[0] == '0') 
	{
		printf("User Canceled file share\n---------------------\n");
		return;
	}
	
	// Get their username
	for(int i = 0; i < numUsers; i++)
	{
		if(users[i].fd == client_socket)
		{
			username = users[i].name;
		}
	}


	// Check if the file exists
	for(int i = 0; i < numFiles; i++)
	{
		// If it does, are they the owner
		if(strcmp(files[i].filename, file) == 0) 
		{
			isFound = 1;
			idx = i;
			if(strcmp(files[i].owner, username) == 0)
			{
				isOwner = 1;
			}
			break;
		}
	}

	// If the file isnt found
	if(!isFound)
	{
		bs = send(client_socket, fileNotExist, strlen(fileNotExist), 0);
		goto start;
	}
	
	// Check if they are not the owner
	if(!isOwner)
	{
		bs = send(client_socket,notOwner, strlen(notOwner), 0);
		printf("isOwner: %d\n", bs);
		goto start;
	}

	// Satisfy Client recv, msg not important
	bs = send(client_socket, "FILEOK", 7, 0);
	
	// Get the username and file permissions they want to add from the user
	bs = recv(client_socket, selUser, sizeof selUser, 0);
	bs = recv(client_socket, perms, sizeof perms, 0);
	
	// Check if the username exists in the shared
	// TODO Check if its possible to reference struct that hasnt been created
	for(int i = 0; i < MAXUSERS; i++)
	{
		if(strcmp(files[idx].shared[i].name, selUser) == 0)
		{
			// User already exists in shared so update perms
			strcpy(files[idx].shared[i].perms, perms);
			printf("Updated %s permissions to \"%s\"\n----------------------------------------\n", 
				files[idx].shared[i].perms,
				files[idx].shared[i].name);
			return;
		}
	}

	// If not add new 
	// Probably Should fix this mess ))
	strncpy(files[idx].shared[files[idx].numShared].name, selUser, sizeof(files[idx].shared[files[idx].numShared].name));
	strncpy(files[idx].shared[files[idx].numShared].perms, perms, sizeof(files[idx].shared[files[idx].numShared].perms));
	printf("Added %s permissions to \"%s\"\n----------------------------------------\n", 
		files[idx].shared[files[idx].numShared].perms,
		files[idx].shared[files[idx].numShared].name);

	files[idx].numShared++;
	fflush(stdout);
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