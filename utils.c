#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "utils.h"
#include "socketinfo.h"

void sendFileToSocketFd(int fd, char* filename)
{
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
			if(send(fd, data, sizeof data, 0) == -1)
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

void downloadFileFromSocketFd(int fd, char* filename)
{
	printf("-----DOWNLOADING FILE-----\n");
	
	int response;
	char data[DATASIZE] = {0}; // init to 0
	
	// Get the filename
	printf("Filename: %s\n", filename);
	// Create the file
	FILE* fp = fopen(filename, "w");

	if(fp == NULL) printf("ERROR OPENING FILE\n");

	// Set fd to nonblocking
	// Once data has been recieved 
	while(1)
	{
		char cleanedData[DATASIZE] = {0}; 
		int cdSize = 0;
		response = recv(fd, data, sizeof(data), 0);
		setBlockingFd(0, fd);
		int errnum;
		errnum = errno;

		//ERROR STUFF
		//fprintf(stderr, "Value of errno %d\n", errno);
		//perror("Error Printed by Perror\n");
		//fprintf(stderr, "Error Opening File %s\n", strerror(errnum));

		// Print data
		printf("Data: %s\n", data);
		printf("res: %d\n", response);
		
		// If no msgs are avaliable or there is an error then break
		if(response <= 0) 
		{
			printf("End of Stream\n");
			break;
		}
		
		// Clean up stream
		// Client sends bunch of nulls, so only write chars that arent null
		// to cleanedData, then write that to the file
		for(int i = 0; i < sizeof(data)/sizeof(char); i++)
		{
			// If the char is not null, add it to the array
			if(*(data+i) != '\0')
			{
				cleanedData[cdSize] = *(data+i);
				cdSize++;
			}
			//Add null terminator
		}

		memset(data, 0, sizeof(data));
		cleanedData[cdSize] = '\0';
		
		//Write data into file
		fprintf(fp, "%s", cleanedData);

		printf("Writing data %s\n\n\n", cleanedData);
	}
	
	// Set fd back to blocking 
	setBlockingFd(1, fd);

	printf("DONE WRITING");
	fflush(fp);
	fclose(fp);
	printf("-----------------------");
}

void setBlockingFd(_Bool toggle, int fd)
{
	if(toggle == 0)
	{
		// Set the socket to non blocking
		int flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1) printf("ERROR GETTING SOCKET FLAGS\n");
		flags = (flags | O_NONBLOCK);
		int status = fcntl(fd, F_SETFL, flags);
		if(status == -1) printf("ERROR SETTING SOCKET TO NON BLOCKING 2\n");
	}
	else 
	{
		int flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1) printf("ERROR GETTING SOCKET FLAGS\n");
		flags = (flags & ~O_NONBLOCK);
		int status = fcntl(fd, F_SETFL, flags);
		if(status == -1) printf("ERROR SETTING SOCKET TO BLOCKING \n");
	}
}