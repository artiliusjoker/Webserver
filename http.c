#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include"constants.h"
#include"net.h"
#include"file.h"
#include"mime.h"
#include "http.h"

const int http_methods__arr_len = UNKNOWN - OPTIONS;
const char *http_methods_array[] = {
    "OPTIONS", 
    "GET", 
    "HEAD", 
    "POST", 
    "PUT", 
    "DELETE", 
    "TRACE", 
    "CONNECT",
    "UNKNOWN"
};

static void http_request_print(http_request *req);

http_request *http_read_request(int sockfd, char** request_result)
{
	
    // Init buffer
    char buffer[MAX_LINE_BUF];
    read_buffer *rbuf;
    rbuf = calloc(1, sizeof(*rbuf));
    rbuf->current_fd = sockfd;
    
    // First line -> get METHOD, URL, VERSION
    int retVal = read_line_socket(rbuf, buffer, MAX_LINE_BUF);
    if(retVal <= 0)
    {
        free(rbuf);
        return NULL;
    }

    // Init request
    http_request *new_request = (http_request *) malloc(sizeof(http_request));
    new_request->method = 0; 
    TAILQ_INIT(&new_request->metadata_head);

    char* token = NULL;
    char* copy, *pointer_copy;

    // copy first line into string
    *request_result = (char *) malloc(strlen(buffer) + 1);
    strcpy(*request_result, buffer);

    copy = strdup(buffer);
    pointer_copy = copy;
    int index = 0;
    while ((token = strsep(&copy, " \r\n")) != NULL) {
        if(index == 3) break;
        switch (index) {
            case 0: {
                int found = 0;
                for (int i = 0; i <= http_methods__arr_len; i++) {
                    if (strcmp(token, http_methods_array[i]) == 0) {
                        found = 1;
                        new_request->method = i;
                        break;
                    }
                }
                if (found == 0) {
                    new_request->method = http_methods__arr_len;
                    free(copy);
                    return new_request;
                }
                ++index;
                break;
            }
            case 1:
                new_request->search_path = strdup(token);
                ++index;
                break;
            case 2:
            {
                if(strcmp(token, "HTTP/1.0") == 0) 
                {
                    new_request->version = HTTP_VERSION_1_0;
                }     
                else if(strcmp(token, "HTTP/1.1") == 0) 
                {
                    new_request->version = HTTP_VERSION_1_1;
                } 
                else 
                {
                    new_request->version = HTTP_VERSION_INVALID;
                }
                ++index;
                break;
            }
            case 3:
                break;
        }
    }
    // Done first line

    // Clean up
    free(pointer_copy);

    // Rest : other metadata
    do
    {
        read_line_socket(rbuf, buffer, MAX_LINE_BUF);
        // copy first line into string
        int buf_len = strlen(buffer);
        int curr_len = strlen(*request_result);
        *request_result = (char *) realloc(*request_result, (buf_len + curr_len) + 1);
        strcat(*request_result, buffer);
        if(buffer[0] == '\r' && buffer[1] == '\n')
		{
			// The end of the HTTP header
			break;
		}
        copy = strdup(buffer);
        char *key = strdup(strtok(copy, ":"));
        char *value = strdup(strtok(NULL, "\r"));
        free(copy);
        // remove whitespaces
        char *p, *hold;
        p = strdup(value);
        hold = p;
        while(*p == ' ') p++;
        free(value);
        value = strdup(p);
        free(hold);
        // create the http_metadata_item object
        struct http_metadata_item *item = malloc(sizeof(*item));
        item->key = key;
        item->value = value;
        // add the new item to the list of metadatas     
        TAILQ_INSERT_TAIL(&new_request->metadata_head, item, entries);

    } while (1);
    free(rbuf);
    //http_request_print(new_request);
    return new_request;
}

void http_request_free(http_request *req)
{
    free((char*)req->search_path);
    struct http_metadata_item *item; 
    TAILQ_FOREACH(item, &req->metadata_head, entries) {
        free((char*)item->key);
        free((char*)item->value); 
        free(item);
    }
    free(req);
    req = NULL;
}

static void http_request_print(http_request *req)
{
    printf("[HTTP_REQUEST] \n"); 

    switch (req->version) {
      case HTTP_VERSION_1_0:
        printf("version:\tHTTP/1.0\n");
        break;
      case HTTP_VERSION_1_1:
        printf("version:\tHTTP/1.1\n");
        break;
      case HTTP_VERSION_INVALID:
        printf("version:\tInvalid\n");
        break;
    }

    printf("method:\t\t%s\n", 
            http_methods_array[req->method]);
    printf("path:\t\t%s\n", 
            req->search_path); 
    printf("[Metadata] \n"); 
    struct http_metadata_item *item; 
    TAILQ_FOREACH(item, &req->metadata_head, entries) {
        printf("%s: %s\n", item->key, item->value); 
    }
}

// Tuple contains : code and name of http status
typedef struct http_status_tuple{
    char status_code[4];
    char status_name[20]; 
}http_status_tuple;

http_status_tuple * create_response_tuple(int status_code)
{
    http_status_tuple * tuple = (http_status_tuple *) malloc(sizeof(*tuple));
    switch (status_code)
    {
    case OK:
    {
        strcpy(tuple->status_code, "200");
        strcpy(tuple->status_name, "Ok");
        break;
    }       
    case BAD_REQUEST:
    {
        strcpy(tuple->status_code, "400");
        strcpy(tuple->status_name, "Bad Request");
        break;
    }
    case MOVED_PERMANENTLY:
    {
        strcpy(tuple->status_code, "301");
        strcpy(tuple->status_name, "Moved Permanently");
        break;
    }
    case FORBIDDEN:
    {
        strcpy(tuple->status_code, "403");
        strcpy(tuple->status_name, "Forbidden");
        break;
    }
    case NOT_FOUND:
    {
        strcpy(tuple->status_code, "404");
        strcpy(tuple->status_name, "Not Found");
        break;
    }
    case METHOD_NOT_ALLOWED:
    {
        strcpy(tuple->status_code, "405");
        strcpy(tuple->status_name, "Method Not Allowed");
        break;
    }   
    default:
        free(tuple);
        tuple = NULL;
        break;
    }
    return tuple;
}

/* 
    HTTP response builder, header and body
*/
http_custom_response *http_response_build(int status_code, char* file_name)
{
    // Get file
    struct file_data *filedata; 
    char mime_type[20];

    // Get file in byte array
    filedata = file_load(file_name);

    // Http status code
    http_status_tuple * tuple;
    tuple = create_response_tuple(status_code);

    // File not exists in root, send 404 response
    if (filedata == NULL) {
        //http_custom_response * error_response;
        //error_response = http_error_response_builder(NOT_FOUND);
        free(tuple);
        tuple = create_response_tuple(NOT_FOUND);
        filedata = file_load("404.html");
    }
    // MIME type
    strcpy(mime_type, mime_type_get(filedata->filepath));

    // Create response with body
    http_custom_response *new_response = (http_custom_response *) calloc(1, sizeof(*new_response));
    new_response->http_header = (char *) calloc(MAX_HTTP_HDR_SIZE, 1);
    new_response->body_content = NULL;
    new_response->header_size = 0;
    new_response->total_size = 0;

    // current system date
    char date_buf[50];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S %Z", &tm);

    // current machine ip
    char * host_ip = get_host_ip();

    // Create header
    int hdr_size = snprintf(new_response->http_header, MAX_HTTP_HDR_SIZE, 
                                                            "HTTP/1.1 %s %s\r\n"
                                                            "Date: %s\r\n" // today
                                                            "Set-Cookie: SERVERID=%s\r\n" // Cookie 
                                                            "Cache-Control: no-cache, private\r\n" // no cache
                                                            "Content-Length: %d\r\n" // body is empty
                                                            "Content-Type: %s\r\n" // mime type of file                                              
                                                            "Connection: keep-alive\r\n" // notify the clients to close connection
                                                            "\r\n", tuple->status_code, tuple->status_name, date_buf, host_ip, filedata->size, mime_type);
    free(host_ip);
    new_response->header_size = hdr_size;
    new_response->body_content = filedata;
    new_response->total_size = hdr_size + filedata->size;
    free(tuple);
    return new_response;
}

void http_response_free(http_custom_response * response)
{
    if((response)->http_header)
        free((response)->http_header);

    if((response)->body_content)
        file_free(response->body_content);

    if(response){
        free(response);
        response = NULL;
    }
}
