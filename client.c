/* Client demo */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>

#include "socketinfo.h"

int main(){

	// Handle Ctrl+c
	signal(SIGINT, SIG_IGN);

	//create a socket
	int network_socket;
	network_socket = socket (AF_INET, SOCK_STREAM, 0);

	// specify an address for the socket
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
	send(network_socket, "connected", 10, 0);

	while(1)
	{
		char response[256];
		// Recieve Server Response
		recv(network_socket, &response, sizeof(response), 0);	
		if(strcmp(response, "EXITING") == 0) break;	
		// Print out the server response
		printf("%s\n", response);

		// Take User input to select option
		char input[20];
		scanf("%s", input);

		// Send User Input
		int bs = send(network_socket, input, sizeof(input), 0);
		printf("loop | bytes: %d\n", bs);
	}
	
	close(network_socket);
	return 0;
}
