#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include "main.h"

static int establishConnection();
static void readRequest(int fd, char * path);
static int sendFile(int fd, char * file);
static void readLine(char * string, char * buffer);
static int handleOpts(int argc, char ** argv, char * root);
static void printHelp();
static void send404(int fd, char * root);
static void * handleRequest(void * request_info);

int main(int argc, char ** argv)
{
    int fd = 0;
    int c;
    char root[1024] = { 0 }; 
    struct sockaddr_in client;
    request_info_t * request_info = 0;
    pthread_t browser_thread;

    if(handleOpts(argc, argv, root) != 0)
    {
        return 0;
    }

    printf("Starting server\nRoot Dir: %s\nPort: %d\n", 
            root, int_port);

    fd = establishConnection();
    c = sizeof(struct sockaddr_in);

    while(1)
    {
        request_info = malloc(sizeof(request_info_t));        
        strcpy(request_info->file, root);
        strcpy(request_info->root, root);

        if((request_info->browser_fd = 
                accept(fd, (struct sockaddr *)&client, 
                (socklen_t*)&c)) < 0)
        {
            perror("Accept()");
            free(request_info);
            close(fd);
            continue;
        }

        if(pthread_create(&browser_thread, NULL, handleRequest, request_info) != 0)
        {
            //perror("Pthread_create()");
            free(request_info);
            close(fd);
            continue;
        }
        
        //pthread_detach(browser_thread);
    }

    close(fd);

    return 0;
}

/****************************************************************
 * Author: Michael Blake
 * Date 12/5/2017
 *
 * Function for pthreadcreate to read a request from a socket
 * and reply with an http response
 ***************************************************************/

void * handleRequest(void * request_info)
{
    request_info_t * request_struct = (request_info_t *)request_info;
    int send_return = 0;

    readRequest(request_struct->browser_fd, 
                request_struct->file + strlen(request_struct->root));

    printf("file: %s\n", request_struct->file);

    if((send_return = sendFile(request_struct->browser_fd, 
                               request_struct->file)) != 0)
    {
        if(send_return == 1)
        {
            send404(request_struct->browser_fd, request_struct->root);
        }   
    }
    close(request_struct->browser_fd);
    free(request_info);
    pthread_detach(pthread_self());

    return 0;
}

/****************************************************************
 * Author: Michael Blake
 * Date 12/5/2017
 *
 * Creates new socket, starts listening for connections, and
 * returns new fd created
 ***************************************************************/


int establishConnection()
{
    int fd = 0;
    struct sockaddr_in server;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("Socket()");
        exit(1);
    }
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(int_port);
     
    if(bind(fd, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("Bind()");
        exit(1);
    }
     
    listen(fd , 3);

    return fd;   
}

/****************************************************************
 * Author: Michael Blake
 * Date 12/5/2017
 *
 * Waits for data from socket, reads any valid http request, and 
 * writes the file path to "path"
 ***************************************************************/

void readRequest(int fd, char * path)
{
    int read_size = 0;
    char line[500] = { 0 };
    char request[1024] = { 0 };

    printf("Read Request\n");

    if((read_size = 
            recv(fd, request, MAX_REQUEST_SIZE, 0)) < 0)
    {
        perror("Recv()");
        exit(1);
    }

    readLine(request, line);

    sscanf(line, "GET %s HTTP", path);

    if(path[strlen(path) - 1] == '/')
    {
        strcat(path, "index.html");
    }

    /*if(sscanf(path, "/%s?", file) != 1)
    {
        strcpy(file, "/index.html");
    }*/
}

/****************************************************************
 * Author: Michael Blake
 * Date 12/5/2017
 *
 * reads file "file" and writes http response and file to socket 
 * "fd"
 ***************************************************************/

int sendFile(int fd, char * file)
{
    struct stat st;
    int bytes_read = 0;
    char file_buffer[1024] = { 0 };    
    int file_fd = 0;
    char file_size[10] = { 0 };

    printf("Send File\n");

    if((file_fd = open(file, O_RDONLY)) < 0)
    {
        return 1;
    }
    
    stat(file, &st);
    sprintf(file_size, "%ld", st.st_size);
    memset(file_buffer, 0, 1024);

    send(fd, HTTP_VERSION, strlen(HTTP_VERSION), 0);
    send(fd, OK, strlen(OK), 0);
    send(fd, CRLF, strlen(CRLF), 0);
    send(fd, CONTENT_TYPE, strlen(CONTENT_TYPE), 0);
    send(fd, CRLF, strlen(CRLF), 0);
    send(fd, CONTENT_LENGTH, strlen(CONTENT_LENGTH), 0);
    send(fd, file_size, strlen(file_size), 0);
    send(fd, CRLF, strlen(CRLF), 0);
    send(fd, CRLF, strlen(CRLF), 0);

    do
    {
        bytes_read = read(file_fd, file_buffer, sizeof(file_buffer));
        if(send(fd, file_buffer, bytes_read, 0) < 0)
        {
            perror("Send()");
            exit(1);
        }
        memset(file_buffer, 0, 1024);
    } while(bytes_read > 0);

    close(file_fd);

    return 0;
}

/****************************************************************
 * Author: Michael Blake
 * Date 12/5/2017
 *
 * Reads line seperated by a CRLF
 ***************************************************************/

void readLine(char * string, char * buffer)
{
    int i = 0;

    while(i < strlen(string) &&
              !(string[i] == '\r' && string[i + 1] == '\n'))
    {
        buffer[i] = string[i];
        i++;
    }

    buffer[i + 1] = 0;
}

int handleOpts(int argc, char ** argv, char * root)
{
    char port[MAX_PORT] = { 0 };
    int c;

    while ((c = getopt (argc, argv, "hp:r:")) != -1)
    {
        switch (c)
        {
            case 'h':
                printHelp();
                return 1;
                break;
            case 'p':
                strncpy(port, optarg, MAX_PORT - 1);
                sscanf(port, "%hu", &int_port);
                break;
            case 'r':
                strncpy(root, optarg, MAX_ROOT - 1);
                break;
            case '?':
                break;
            default:
                printf("Invalid arguments\n");
                return 1;
        }

        if(root[0] == 0)
        {
            printf("Please specify a root directory with -r\n");
            exit(1);
        }
    }
    return 0;
}

/****************************************************************
 * Author: Michael Blake
 * Date 12/5/2017
 *
 * Prints help info
 ***************************************************************/

void printHelp()
{
    printf("Command Line Arguments\n \
            -h: help\n \
            -p port (default: 8000): port web server runs on\n \
            -r dir (required): root directory of website\n");
    printf("To add a custom 404 page, name the file 404.html \
            and put it in your website's root directory");
}

/****************************************************************
 * Author: Michael Blake
 * Date 12/5/2017
 *
 * Sends 404 response with custom 404 page contents if available
 ***************************************************************/

void send404(int fd, char * root)
{
    struct stat st;
    int bytes_read = 0;
    char file_buffer[1024] = { 0 };
    int file_fd = 0;
    char file_size[10] = { 0 };
    char file[MAX_ROOT] = { 0 };

    strcpy(file, root);
    strcat(file, "/404.html");
    printf("404: %s\n", file);

    send(fd, HTTP_VERSION, strlen(HTTP_VERSION), 0);
    send(fd, NOT_FOUND, strlen(NOT_FOUND), 0);
    send(fd, CRLF, strlen(CRLF), 0);
    send(fd, CONTENT_TYPE, strlen(CONTENT_TYPE), 0);
    send(fd, CRLF, strlen(CRLF), 0);

    if((file_fd = open(file, O_RDONLY)) < 0)
    {
        send(fd, CONTENT_LENGTH, strlen(CONTENT_LENGTH), 0);
        send(fd, "0", 1, 0);
        send(fd, CRLF, strlen(CRLF), 0);
        send(fd, CRLF, strlen(CRLF), 0);
    }
    else
    {
        stat(file, &st);
        sprintf(file_size, "%ld", st.st_size);
        memset(file_buffer, 0, 1024);

        send(fd, CONTENT_LENGTH, strlen(CONTENT_LENGTH), 0);
        send(fd, file_size, strlen(file_size), 0);
        send(fd, CRLF, strlen(CRLF), 0);
        send(fd, CRLF, strlen(CRLF), 0);

        do
        {
            bytes_read = 
                read(file_fd, file_buffer, sizeof(file_buffer));

            if(send(fd, file_buffer, bytes_read, 0) < 0)
            {
                perror("Send()");
                exit(1);
            }
            memset(file_buffer, 0, 1024);
        } while(bytes_read > 0);

        close(file_fd);
    }
}
