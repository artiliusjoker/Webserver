#include"net.h"

/**
 * Send an HTTP response
 *
 * header:       "HTTP/1.1 404 NOT FOUND" or "HTTP/1.1 200 OK", etc.
 * content_type: "text/plain", etc.
 * body:         the data to send.
 * 
 * Return the value from the send() function.
 */
int send_response(int fd, char *header, char *content_type, void *body, int content_length)
{
    const int max_response_size = 262144;
    char response[max_response_size];
    int response_length;
    // Build HTTP response and store it in response

    ///////////////////
    // IMPLEMENT ME! //
    ///////////////////

    // Send it all!
    int rv = send(fd, response, response_length, 0);

    if (rv < 0) {
        perror("send");
    }

    return rv;
}

/**
 * Send a 404 response, sent the whole 404 file to client
 */
void send_404(int fd)
{
    char filepath[100];
    struct file_data *filedata; 
    char *mime_type;

    // Fetch the 404.html file
    snprintf(filepath, sizeof filepath, "%s/404.html", SERVER_ROOT);
    filedata = file_load(filepath);

    if (filedata == NULL) {
        // Send 404 header if not exists
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }

    mime_type = mime_type_get(filepath);

    send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);

    file_free(filedata);
}

//from Beej's guide
int send_all_to_socket(int sock_fd, char *buf, int buf_len, int *send_len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = buf_len; // how many we have left to send
    int n;

    while(total < buf_len) {
        n = send(sock_fd, buf + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    if(send_len != NULL)
    {
        *send_len = total; // return number actually sent here
    }
    return n == -1 ? - 1 : 0; // return -1 on failure, 0 on success
}

// Read a single line of http request from the socket
// Socket info is stored in read_buffer
int read_line_socket(read_buffer *r_buf, char *dst, unsigned int size)
{
    unsigned int i = 0;
    ssize_t bytes_recv;
    
    while (i < size) {
        if (r_buf->r_pos == r_buf->w_pos) 
        {
            size_t wpos = r_buf->w_pos % MAX_READ_BUF;
        
            if((bytes_recv = recv(r_buf->current_fd, r_buf->buf + wpos, (MAX_READ_BUF - wpos), 0)) < 0) 
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
    if(i == size) 
    {
        return -1;
    }
    // String terminate char
    dst[i] = '\0';
    return i;
}