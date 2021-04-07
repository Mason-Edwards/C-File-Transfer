/* Client demo */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "socketinfo.h"
#include "utils.h"

void uploadFile(int network_socket);
void downloadFile(int network_socket);

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


	//Get filename, also send it to the server
	printf("\nPlease Enter a file to upload\n> ");
	scanf("%s", filename);
	send(network_socket, filename, sizeof filename, 0);

	FILE* fp = fopen(filename, "r");

	printf("-----Uploading-----\n");
	
	char data[DATASIZE] = {0};
	// If file opens correctly
	if(fp != NULL)
	{
		// While there is data in the file, Read 1mb of data into "data" and send it
		while(fgets(data, sizeof data, fp) != NULL)
		{
			printf("Sending %s\n\n\n", data);
			if(send(network_socket, data, sizeof data, 0) == -1)
			{
				printf("ERROR SENDING FILE\n");
			}
			// Clear data for next iteration, otherwise garbage is left
			for(int i = 0; i < DATASIZE; i++)
			{
				data[i] = 0;
			}
		}
	}

	else if (fp == NULL) printf("ERROR READING FILE\n");
	printf("------------------\n\n");
	fclose(fp);
}

void downloadFile(int network_socket)
{
	char choices[200] = {0};
	char input[30] = {0};
	char confirm[] = "FILEDOWNLOADED";

	int bs = recv(network_socket, choices, sizeof(choices), 0);

	printf("BYTES RECIEVED: %d\n", bs);
	printf("Please Select a file from the list...\n");
	printf("%s\n>", choices);
	
	scanf("%s", input);

	// Send the file the user wants to download
	bs = send(network_socket, input, sizeof(input), 0);

	downloadFileFromSocketFd(network_socket, input);

	bs = send(network_socket, "DOWNLOADCOMPLETE", 17, 0);
}