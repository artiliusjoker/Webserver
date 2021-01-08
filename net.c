#include <sys/types.h>
#include <ifaddrs.h>
#include "net.h"

//from Beej's guide
int send_all_to_socket(int sock_fd, char *buf, int buf_len, int *send_len)
{
    int total = 0;           // how many bytes we've sent
    int bytesleft = buf_len; // how many we have left to send
    int n;

    while (total < buf_len)
    {
        n = send(sock_fd, buf + total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    if (send_len != NULL)
    {
        *send_len = total; // return number actually sent here
    }
    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

// Read a single line of http request from the socket
// Socket info is stored in read_buffer
int read_line_socket(read_buffer *r_buf, char *dst, unsigned int size)
{
    unsigned int i = 0;
    ssize_t bytes_recv;

    while (i < size)
    {
        if (r_buf->r_pos == r_buf->w_pos)
        {
            size_t wpos = r_buf->w_pos % MAX_READ_BUF;

            if ((bytes_recv = recv(r_buf->current_fd, r_buf->buf + wpos, (MAX_READ_BUF - wpos), 0)) < 0)
            {
                return -1;
            }
            else if (bytes_recv == 0)
            {
                return 0;
            }
            r_buf->w_pos += bytes_recv;
        }
        dst[i++] = r_buf->buf[r_buf->r_pos++ % MAX_READ_BUF];
        if (dst[i - 1] == '\n')
        {
            break;
        }
    }
    // Line too big
    if (i == size)
    {
        return -1;
    }
    // String terminate char
    dst[i] = '\0';
    return i;
}

char *get_host_ip()
{
    struct ifaddrs *addrs;
    getifaddrs(&addrs);
    struct ifaddrs *tmp = addrs;

    char * result = NULL;
    while (tmp)
    {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            result = strdup(inet_ntoa(pAddr->sin_addr));
            if(strcmp("127.0.0.1", result) != 0)
            {
                break;
            }
        }
        tmp = tmp->ifa_next;
        free(result);
        result = NULL;
    }
    freeifaddrs(addrs);
    return result;
}