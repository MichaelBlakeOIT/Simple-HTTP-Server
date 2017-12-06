#pragma once

#define CRLF "\r\n"
#define BAD_REQUEST "400 Bad Request"
#define NOT_FOUND "404 Not Found"
#define METHOD_NOT_ALLOWED "405 Method Not Allowed"
#define OK "200 OK"
#define HTTP_VERSION "HTTP/1.1 "
#define CONTENT_TYPE "Content-Type: text/html"
#define CONTENT_LENGTH "Content-Length: "
#define MAX_REQUEST_SIZE 2048
#define MAX_ROOT 64
#define MAX_PORT 6

unsigned short int int_port = 8000;
unsigned short int shutdown_server = 0;

typedef struct request
{
    char path[500];
} request_t;

typedef struct request_info
{
    int browser_fd;
    char file[1024];
    char root[MAX_ROOT];
} request_info_t;

