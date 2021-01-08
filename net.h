#ifndef _NET_H_
#define _NET_H_

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "constants.h"

// Send whole buffer to socket
int send_all_to_socket(int sock_fd, char *buf, int buf_len, int *send_len);

// Read each line from fd to parse the request
typedef struct r_buf
{
    char buf[MAX_READ_BUF];
    unsigned int r_pos, w_pos; // for keeping track of positions when read line
    unsigned int current_fd;
} read_buffer;
int read_line_socket(read_buffer *, char *, unsigned int);

// Get ip
char *get_host_ip();

#endif