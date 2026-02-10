#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sizes
#define MAX_PATH 256
#define MAX_METHOD 10
#define MAX_VERSION 10

// Status codes
#define HTTP_STATUS_200 "200 OK"
#define HTTP_STATUS_201 "201 Created"
#define HTTP_STATUS_404 "404 Not Found"
#define HTTP_STATUS_400 "400 Bad Request"
#define HTTP_STATUS_500 "500 Internal Server Error"
#define HTTP_STATUS_501 "501 Not Implemented"
#define HTTP_STATUS_204 "204 No Content"

// MIME Types

#define MIME_HTML "text/html"
#define MIME_TXT "text/plain"
#define MIME_JPG "image/jpg"
#define MIME_PNG "image/png"
#define MIME_CSS "text/css"
#define MIME_JS "applications/javascript"

// Petition Structure

typedef struct HttpRequest {
    char method[MAX_METHOD];
    char path[MAX_PATH];
    char version[MAX_VERSION];
    char* body;
    int content_length;
} HttpRequest;

// Function Prototypes
const char* get_mime_type(const char* filename);
void parse_request(char* raw_request, HttpRequest* request);
void handle_request(const HttpRequest* request, char* response_buffer, size_t* response_len);

void handle_get(const HttpRequest* request, char* response_buffer, size_t* response_len);
void handle_head(const HttpRequest* request, char* response_buffer, size_t* response_len);
void handle_post(const HttpRequest* request, char* response_buffer, size_t* response_len);
void handle_put(const HttpRequest* request, char* response_buffer, size_t* response_len);
void handle_delete(const HttpRequest* request, char* response_buffer, size_t* response_len);

#endif