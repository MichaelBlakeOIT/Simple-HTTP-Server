#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define HTTP 8000
#define CRLF "\r\n"
#define MAX_REQUEST_SIZE 2048

typedef struct request 
{
	char path[500];
} request_t;

int establishConnection();
void readRequest(int fd, char * request, char * path);
void sendFile(int fd, char * file);
void readLine(char * string, char * buffer)
{
	int i = 0;
	//memcpy(buffer, 0, sizeof(buffer));

	while(i < strlen(string) &&
			  !(string[i] == '\r' && string[i + 1] == '\n'))
	{
		buffer[i] = string[i];
		i++;
	}

	buffer[i + 1] = 0;
}


int main(int argc, char ** argv)
{
	int bytes_written = 0;
	int fd = 0;
	int read_size = 0;
	char client_message[MAX_REQUEST_SIZE] = { 0 };
	char request_buffer[MAX_REQUEST_SIZE] = { 0 };
	char file[1024] = { 0 };
	char * reply = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 19\r\n\r\n";
	
	{

		fd = establishConnection();

		readRequest(fd, request_buffer, file);

		sendFile(fd, file);

		/*if((bytes_written = write(fd, reply, strlen(reply))) < 0)
		{
			perror("Write()");
		}*/

		close(fd);

	}

	return 0;
}


int establishConnection()
{
	int fd;
	int client_sockl;
	int c;
	int read_size;
    struct sockaddr_in server;
	struct sockaddr_in client;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        printf("Could not create socket");
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(HTTP);
     
    if(bind(fd, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }
     
    listen(fd , 3);
     
    c = sizeof(struct sockaddr_in);

    return accept(fd, (struct sockaddr *)&client, (socklen_t*)&c);
}

void readRequest(int fd, char * request, char * path)
{
	int read_size = 0;
	//char path[222] = { 0 };	
	char line[500] = { 0 };
	char file[100] = { 0 };

	if((read_size = 
			recv(fd, request, MAX_REQUEST_SIZE, 0)) < 0)
    {
        perror("Recv()");
    }

	readLine(request, line);

	sscanf(line, "GET %s HTTP", path);

	if(sscanf(path, "/%s?", file) != 1)
	{
		strcpy(file, "index.html");
	}
}

void sendFile(int fd, char * file)
{
	int bytes_read = 0;
	char file_buffer[1024] = { 0 };
	file = "/home/michael/index.txt";
	char * reply = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 14\r\n\r\n";
	//FILE * fp = 0;
	int file_fd = 0;

	/*if((fp = fopen(file, "r")) == NULL)
	{
		perror("fopen()");
	}*/
	if((file_fd = open(file, O_RDONLY)) < 0)
	{
		perror("open()");
	}
	
	memset(file_buffer, 0, 1024);

	send(fd, reply, strlen(reply), 0);

	//while((bytes_read = 
	//		fread(file_buffer, sizeof(char), 1024, fp)) < 0)
	do
	{
		bytes_read = read(file_fd, file_buffer, sizeof(file_buffer));
		printf("sending %s\n", file_buffer);
		if(send(fd, file_buffer, bytes_read, 0) < 0)
		{
			perror("Send()");
		}
		memset(file_buffer, 0, 1024);
	} while(bytes_read > 0);
}
