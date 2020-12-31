#ifndef HTTP_H
#define HTTP_H

#include <sys/queue.h>
#include"constants.h"

enum http_method{
    OPTIONS,
    GET,
    HEAD,
    POST,
    PUT, 
    DELETE, 
    TRACE,
    CONNECT, 
    UNKNOWN
};

enum http_version{
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_INVALID
};

typedef enum http_method http_method;
typedef enum http_version http_version;

// Struct to store other metadata of http request
typedef struct http_metadata_item
{ 
    const char *key; 
    const char *value;
    // Pointers to previous and next entries
    TAILQ_ENTRY(http_metadata_item) entries;
}http_metadata_item; 

// Struct to store raw request from client
typedef struct http_request
{
    http_method method; 
    http_version version;
    const char *search_path;
    TAILQ_HEAD(METADATA_HEAD, http_metadata_item) metadata_head; 
} http_request;

// Free struct http_request
void http_request_free(http_request*);

// Read raw request from client and store in struct http_request
// and store the request in the second argument
http_request *http_read_request(int, char**);

// http respond code
typedef enum http_response_code{
    MOVED_PERMANENTLY,
    BAD_REQUEST,
    FORBIDDEN,
    NOT_FOUND,
    METHOD_NOT_ALLOWED, 
    OK,
}http_response_code;

// Custom respond builder
typedef struct http_custom_response{
    char *http_header;
    int header_size;
    char *http_html_content;
    int content_size;
}http_custom_response;
http_custom_response *http_response_build(int);
void http_response_free(http_custom_response *);

// Read each line from fd to parse the request
typedef struct r_buf{
    char buf[MAX_READ_BUF];
    unsigned int r_pos, w_pos; // for keeping track of positions when read line
    unsigned int current_fd;
}read_buffer;
int read_line_socket(read_buffer *, char *, unsigned int);

int send_error_response(int error_code, int client_fd);

#endif