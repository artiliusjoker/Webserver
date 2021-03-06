#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "file.h"
#include "constants.h"
/**
 * Loads a file into memory and returns a pointer to the data.
 * 
 * Buffer is not NUL-terminated.
 */
struct file_data *file_load(char *filename)
{
    char *buffer, *p;
    struct stat buf;
    int bytes_read, bytes_remaining, total_bytes = 0;
    char filepath[50];

    int file_name_len = strlen(filename);
    if(filename[0] == '/' && file_name_len == 1)
    {
        snprintf(filepath, sizeof filepath, "%s/%s", SERVER_ROOT, "index.html");
    }
    else
    {
        snprintf(filepath, sizeof filepath, "%s/%s", SERVER_ROOT, filename);
    }
    // Get the file size
    if (stat(filepath, &buf) == -1) {
        return NULL;
    }

    // Make sure it's a regular file
    if (!(buf.st_mode & S_IFREG)) {
        return NULL;
    }

    // Open the file for reading
    FILE *fp = fopen(filepath, "rb");

    if (fp == NULL) {
        return NULL;
    }

    // Allocate that many bytes
    bytes_remaining = buf.st_size;
    p = buffer = malloc(bytes_remaining);

    if (buffer == NULL) {
        return NULL;
    }

    // Read in the entire file
    while (bytes_read = fread(p, 1, bytes_remaining, fp), bytes_read != 0 && bytes_remaining > 0) {
        if (bytes_read == -1) {
            free(buffer);
            return NULL;
        }

        bytes_remaining -= bytes_read;
        p += bytes_read;
        total_bytes += bytes_read;
    }

    // fseek (fp, 0, SEEK_END);
    // fseek (fp, 0, SEEK_SET);
    // int total = 0;
    // if (buffer)
    // {
    //     while (total != bytes_remaining)
    //     {
    //         total += fread(buffer, 1, bytes_remaining, fp);
    //     }
    // }
    // fclose (fp);

    // Allocate the file data struct
    struct file_data *filedata = malloc(sizeof(*filedata));

    if (filedata == NULL) {
        free(buffer);
        return NULL;
    }

    filedata->data = buffer;
    filedata->size = total_bytes;
    strcpy(filedata->filepath, filepath);
    return filedata;
}

/**
 * Frees memory allocated by file_load().
 */
void file_free(struct file_data *filedata)
{
    free(filedata->data);
    free(filedata);
}