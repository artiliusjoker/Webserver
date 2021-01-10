#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "constants.h"
#include "http.h"
#include "file.h"
#include "net.h"

int listening_fd;

static void SIGINT_parent(int signum)
{
    pid_t wait_retVal;
    int status;
    fprintf(stdout, "\nShutting down all processes\n");

    int killall = 0 - getpid();
    kill(killall, SIGINT);

    while ((wait_retVal = wait(&status)) > 0)
        ;

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
    while (1)
    {
        int server_fd, retVal;
        http_request *client_request = NULL;
        char *request_in_string;

        // Read client's request
        client_request = http_read_request(client_fd, &request_in_string);
        if (client_request == NULL)
        {
            return;
        }
        printf("%s\n", request_in_string);
        // Build the http response
        http_custom_response *response = http_response_build(OK, client_request->search_path);

        if (response != NULL)
        {
            // Send the http header
            send_all_to_socket(client_fd, response->http_header, response->header_size, NULL);
            // Send the file
            send_all_to_socket(client_fd, response->body_content->data, response->body_content->size, NULL);
        }

        // Free
        http_request_free(client_request);
        http_response_free(response);
        free(request_in_string);
    }
}

static void start_server(char *port)
{
    signal(SIGINT, SIGINT_parent);
    struct addrinfo result, *list;
    int fd;

    // Allocate mem for result
    memset(&result, 0, sizeof(result));
    result.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    result.ai_socktype = SOCK_STREAM; /* Datagram socket */
    result.ai_flags = AI_PASSIVE;

    int retVal = getaddrinfo(NULL, port, &result, &list);
    if (retVal != 0)
    {
        fprintf(stderr, "Getaddrinfo: %s\n", gai_strerror(retVal));
        exit(EXIT_FAILURE);
    }
    struct addrinfo *iterator = list;
    if (iterator == NULL)
    {
        fprintf(stderr, "Server: cannot create socket fd !\n");
        return;
    }
    // Go through the list to create socket fd
    for (iterator; iterator != NULL; iterator = iterator->ai_next)
    {
        if ((fd = socket(iterator->ai_family, iterator->ai_socktype,
                         iterator->ai_protocol)) < 0)
        {
            continue;
        }
        if (bind(fd, iterator->ai_addr, iterator->ai_addrlen) < 0)
        {
            close(fd);
            continue;
        }
        // Prepare for connections
        if (listen(fd, MAX_REQUESTS_QUEUE_SIZE) < 0)
        {
            close(fd);
            continue;
        }
        break;
    }
    freeaddrinfo(list);
    listening_fd = fd;

    // master file descriptor list
    fd_set master;
    // temp file descriptor list for select()
    fd_set read_fds;
    // maximum file descriptor number
    int fd_max;
    // Incoming fd
    int accept_fd;
    // clear the master and temp sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    // Add the server socket to pool
    FD_SET(fd, &master);
    // Current fd_max is the server socket
    fd_max = fd;
    // Set server socket option to reuse
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("Server-setsockopt()");
        printf("Wait a few seconds for port 80 available again :)\n");
        exit(EXIT_FAILURE);
    }
    int i, j;
    // Main loop
    printf("Web server is listening on port %s, make connections by browsers or curl cmd\n", port);
    for (;;)
    {
        // copy it
        read_fds = master;
        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("Server-select()");
            printf("Wait a few seconds for port 80 available again :)\n");
            exit(EXIT_FAILURE);
        }
        // New connections
        for (i = 0; i <= fd_max; i++)
        {

            if (FD_ISSET(i, &read_fds))
            {
                // New connection is from server socket
                if (i == fd)
                {
                    // handle new connections
                    if ((accept_fd = accept(fd, NULL, NULL)) == -1)
                    {
                        perror("Server-accept()");
                        printf("Wait a few seconds for port 80 available again :)\n");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        // Set timeout of new connections
                        struct timeval tv;
                        tv.tv_sec = 2;
                        setsockopt(accept_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
                        FD_SET(accept_fd, &master);
                        if (accept_fd > fd_max)
                        {
                            // keep track of the maximum
                            fd_max = accept_fd;
                        }
                    }
                    continue;
                }
                handle_client(i);
            }
        }
    }
    // Below code is not using select(), create process for each connection

    // // Fork process's child to handle clients
    // printf("Web server is listening on port %s, make connections by browsers or curl cmd\n", port);
    // // Program loop
    // while (1)
    // {
    //     accept_fd = accept(fd, NULL, NULL);
    //     if (accept_fd == -1)
    //     {
    //         perror("Server");
    //         printf("Restart the program...\n");
    //         break;
    //     }
    //     printf("Received connection\n");
    //     struct timeval tv;
    //     tv.tv_sec = 2;
    //     setsockopt(accept_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    //     // Create thread to handle the new request
    //     pid_t child_pid = fork();

    //     // Parent process
    //     // if(child_pid > 0)
    //     // {
    //     //     break;
    //     // }

    //     // Child process
    //     if (child_pid == 0)
    //     {
    //         signal(SIGINT, SIGINT_child);
    //         handle_client(accept_fd);
    //         // Child process exits
    //         close(accept_fd);
    //         exit(EXIT_SUCCESS);
    //     }
    // }
}

int main(int argc, char *argv[])
{
    // Check superuser to use port 80
    if (getuid() && geteuid())
    {
        printf("Please use root account or user with superuser privileges to open port 80 :)\n");
        exit(EXIT_FAILURE);
    }
    start_server(PORT);
    return 0;
}