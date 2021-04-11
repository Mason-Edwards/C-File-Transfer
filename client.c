/* Client demo */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#include "socketinfo.h"
#include "utils.h"

void uploadFile(int network_socket);
void downloadFile(int network_socket);
void shareFile(int network_socket);

int main(){

	// Handle Ctrl+c to SIG_IGN to ignore ctrl + c so it doesnt 
	// close server too.
	signal(SIGINT, SIG_IGN);

	//create a socket
	int network_socket;
	network_socket = socket (AF_INET, SOCK_STREAM, 0);

	// specify an address for the socket and connect to it
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr= INADDR_ANY;
	int connection_status = connect(network_socket, (struct sockaddr*) &server_address, sizeof(server_address));

	//check for error with the connection
	if(connection_status < 0){
		printf("There was an error while making a connection to the server\n\n");
	}
	
	// Send connection established message to server
	send(network_socket, "successfulConnection", 21, 0);

	while(1)
	{
		char response[MSGSIZE] = {0};
		char input[20];
		// Recieve Server Response
		recv(network_socket, response, sizeof(response), 0);	
		
		// Check if the response is "EXITING" then we can break out of the loop to 
		// close the socket 
		if(strcmp(response, "EXITING") == 0) break;

		// Check if the username is taken
		if(strcmp(response, "NEWUSERNAME") == 0) 
		{
			printf("Username taken, please enter a new one:\n>");
			goto newUsername;
		}

		//Check if the respone is "FILEUPLOAD"
		if(strcmp(response, "UPLOAD") == 0)
		{
			uploadFile(network_socket);
			continue;
		}

		if(strcmp(response, "DOWNLOADING") == 0)
		{
			downloadFile(network_socket);
			continue;
		}

		if(strcmp(response, "SHAREFILE") == 0)
		{
			shareFile(network_socket);
			memset(response, 0, sizeof response);
		}
		
		// Print out the server response and create a prompt
		printf("%s\n> ", response);

newUsername:
		// Take User input to select option
		scanf("%s", input);

		// Send User Input
		int bs = send(network_socket, input, sizeof(input), 0);
		//printf("loop | bytes: %d\n", bs);
	}
	
	close(network_socket);
	return 0;
}

void uploadFile(int network_socket)
{
	// User enter file name and send that to server
 	char filename[30] = {0} ;
	char response[MSGSIZE];

start:
	//Get filename, also send it to the server
	printf("\nPlease Enter a file to upload\n> ");
	scanf("%s", filename);
	send(network_socket, filename, sizeof filename, 0);

	recv(network_socket, response, sizeof response, 0);
	
	if(strcmp(response, "FILEEXISTS") == 0)
	{
		memset(filename, 0, sizeof(filename));
		memset(response, 0, sizeof(response));
		printf("This file already exists on the server.\nPlease change the filename and reupload.\n\n");
		fflush(stdout);
		goto start;
	}

	sendEncryFileToSocketFd(network_socket, filename);
}
void downloadFile(int network_socket)
{
	char choices[200] = {0};
	char input[30] = {0};
	char perms[5];
	char confirm[] = "FILEDOWNLOADED";

	int bs = recv(network_socket, choices, sizeof(choices), 0);

	printf("BYTES RECIEVED: %d\n", bs);
	fflush(stdout);
	printf("Please Select a file from the list...\n");
	fflush(stdout);
	printf("%s\n>", choices);
	fflush(stdout);
	
	scanf("%s", input);

	// Send the file the user wants to download
	bs = send(network_socket, input, sizeof(input), 0);

	downloadEncryFileFromSocketFd(network_socket, input);

	// Send confimation
	bs = send(network_socket, "DOWNLOADCOMPLETE", 17, 0);

	bs = recv(network_socket, perms, sizeof perms, 0);

	/* Note: only need to check for read perms since they can edit the file
			 however they like by default
	*/

	// Read Only Perms
	if(perms[0] == '1')
	{
		// Set file to readonly for everyone
		// Read permissions owner: S_IRUSR
		// Read permissions group: S_IRGRP
		// Read permissions others: S_IROTH
		chmod(input, S_IRUSR | S_IRGRP | S_IROTH);
	}

	bs = send(network_socket, "FINALLYDONE", 12, 0);
}

void shareFile(int network_socket)
{
	int bs;
	char response[MSGSIZE] = {0};
	char choice[MSGSIZE];

start:

	memset(choice, 0, sizeof choice);
	// print menu msg
	printf("What file do you want to share permissions with other users? (Enter 0 to exit)\n");
	fflush(stdout);

	scanf("%s>", choice);


	bs = send(network_socket, choice, sizeof(choice), 0);

	if(choice[0] == '0') return;

	bs = recv(network_socket, response, sizeof(response), 0);

	if(strcmp(response, "FILENOTEXIST") == 0)
	{
		printf("File does not exist on the server.\n\n");
		goto start;
	} 
	
	if(strcmp(response, "NOTOWNER") == 0)
	{
		printf("You are not the owner of this file, please select one you are an owner of.\n\n");
		goto start;
	}

	printf("What is the user you want to share the file with?\n");
	fflush(stdout);
	scanf("%s>", choice);

	// If stuff starts going weird then memset choice
	send(network_socket, choice, sizeof choice, 0);

	printf("\nWhat is the permissions you want the user to have?\n1. Read\n2. Read and Write\n\n>");

	fflush(stdout);
	scanf("%s>", choice);
	
	send(network_socket, choice, sizeof choice, 0);
}