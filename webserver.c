#include"webserver.h"
int listening_fd;

static void SIGINT_parent(int signum)
{
    pid_t wait_retVal;
    int status;
    fprintf(stdout, "\nShutting down all processes\n");

    int killall = 0 - getpid();
    kill(killall, SIGINT);

    while ((wait_retVal = wait(&status)) > 0);
    fprintf(stdout, "Program terminated by CRL-C\n");
    
    close(listening_fd);
    
    exit(EXIT_SUCCESS);
}

static void SIGINT_child(int signum)
{
    fprintf(stdout, "Shut down process %d\n", getpid());
    exit(EXIT_SUCCESS);
}

static void handle_client(int client_fd)
{
    int server_fd, retVal;
    http_request *client_request = NULL;
    char * request_in_string;

    // Read client's request
    client_request = http_read_request(client_fd, &request_in_string);
    if(client_request == NULL)
    {
        fprintf(stderr, "Handling : cannot read client's request ! \n");
        return;
    }
    
    // Build the http response
    http_custom_response* response = http_response_build(OK, client_request->search_path);
    printf("%s", response->http_header);

    if(response != NULL)
    {
        // Send the http header
        send_all_to_socket(client_fd, response->http_header, response->header_size + 1, NULL);
        // Send the file
        send_all_to_socket(client_fd, response->body_content->data, response->body_content->size, NULL);
    }
    // // Send all in an array
    // char response_buffer[MAX_RESPONSE_SIZE];
    // snprintf(response_buffer, MAX_RESPONSE_SIZE, "%s%s", response->http_header, response->body_content->data);
    // // memcpy(response_buffer, response->http_header, response->header_size);
    // // memcpy(response_buffer + response->header_size, response->body_content->data, response->body_content->size);
    // // Send the response to client
    // send_all_to_socket(client_fd, response_buffer, response->total_size, NULL);
    
    // Free
    http_request_free(client_request);
    http_response_free(response);
    free(request_in_string);
}

static void start_server(char *port)
{
    signal(SIGINT, SIGINT_parent);
    struct addrinfo result, *list;
    int fd;

    // Allocate mem for result
    memset(&result, 0, sizeof(result));
    result.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    result.ai_socktype = SOCK_STREAM; /* Datagram socket */
    result.ai_flags = AI_PASSIVE;

    int retVal = getaddrinfo(NULL, port, &result, &list);
    if (retVal != 0) 
    {
        fprintf(stderr, "Getaddrinfo: %s\n", gai_strerror(retVal));
        exit(EXIT_FAILURE);
    }
    struct addrinfo *iterator = list;
    if(iterator == NULL)
    {
        fprintf(stderr, "Server: cannot create socket fd !\n");
        return;
    }
    // Go through the list to create socket fd
    for(iterator; iterator != NULL; iterator = iterator->ai_next)
    {
        if((fd = socket(iterator->ai_family, iterator->ai_socktype,
                        iterator->ai_protocol)) < 0)
        {
            continue;
        }
        if(bind(fd, iterator->ai_addr, iterator->ai_addrlen) < 0)
        {
            close(fd);
            continue;
        }
        // Prepare for connections
        if(listen(fd, MAX_REQUESTS_QUEUE_SIZE) < 0)
        {
            close(fd);
            continue;
        }
        break;
    }
    freeaddrinfo(list);
    listening_fd = fd;


    int accept_fd;
    // Fork process's child to handle clients
    printf("Web server is listening on port %s, make connections by browsers or curl cmd\n", port);
    // Program loop
    while(1)
    {
        accept_fd = accept(fd, NULL, NULL);
        if(accept_fd == -1)
        {
            perror("Server: ");
            continue;
        }
        printf("Received connection\n");
        // Create thread to handle the new request
        pid_t child_pid = fork();
        
        // Parent process
        // if(child_pid > 0)
        // {
        //     break;
        // }

        // Child process
        if(child_pid == 0)
        {
            signal(SIGINT, SIGINT_child);
            handle_client(accept_fd);
            // Child process exits
            close(accept_fd);
            exit(EXIT_SUCCESS);
        }
    }
}

int main(int argc, char *argv[])
{
    start_server(PORT);
    return 0;
}