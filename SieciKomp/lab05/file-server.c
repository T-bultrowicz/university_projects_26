#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <endian.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>

#include "err.h"
#include "common.h"
#include "protocol.h"

#define QUEUE_LENGTH    5
#define BUFFER_SIZE     1024
#define HEADER_SIZE     6

char const save_path[] = "./tmp/";
char const dir_name[] = "tmp/";
atomic_size_t sum_of_files = 0;

void *handle_connection(void *client_fd_ptr) {
    client_data cl = *(client_data *) client_fd_ptr;
    free(client_fd_ptr);

    data_header dh;
    if (readn(cl.client_fd, &dh, HEADER_SIZE) != HEADER_SIZE)
        syserr("Wrong header size sent!\n");
    dh.file_length = ntohl(dh.file_length);
    dh.name_length = ntohs(dh.name_length);

    
    char name[dh.name_length + 7];
    memcpy(name, save_path, 6);
    name[dh.name_length + 6] = '\0';

    char * buffor = malloc(BUFFER_SIZE + 1);
    if (!buffor) {
        fprintf(stderr, "[Thread %lu] malloc error!\n", pthread_self());
        free((void*)cl.client_ip);
        close(cl.client_fd);
        return NULL;
    }
    buffor[BUFFER_SIZE] = '\0';

    if (readn(cl.client_fd, name + 6, dh.name_length) != dh.name_length) {
        fprintf(stderr, "[Thread %lu] Did not read enough bytes from user!\n", pthread_self());
        free((void*)cl.client_ip);
        close(cl.client_fd);
        return NULL;
    }

    printf("new client [%s:%" PRIu16 "] size=[%u] file=[%s]\n", cl.client_ip, cl.client_port, dh.file_length, name + 6);

    // SLEEPING FUNCTION
    sleep(1);

    int file_fd = open(name, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
    if (file_fd < 0) {
        fprintf(stderr, "[Thread %lu] Did not manage to create a file!\n", pthread_self());
        free((void*)cl.client_ip);
        close(cl.client_fd);
        return NULL;
    }

    uint32_t read_bytes = 0;
    while (read_bytes < dh.file_length) {
        ssize_t left = dh.file_length - read_bytes;
        ssize_t expected = left > BUFFER_SIZE ? BUFFER_SIZE : left;
        if (readn(cl.client_fd, buffor, expected) != expected) {
            fprintf(stderr, "[Thread %lu] Did not read enough bytes from user!\n", pthread_self());
            free((void*)cl.client_ip);
            close(cl.client_fd);
            return NULL;
        }

        read_bytes += expected;
        writen(file_fd, buffor, expected);
    }

    size_t prev = atomic_fetch_add(&sum_of_files, (size_t) dh.file_length);
    printf("new client [%s:%" PRIu16 "] has sent its file of size=[%u]\n total size of uploaded files: [%lu]\n", 
        cl.client_ip, cl.client_port, dh.file_length, prev + dh.file_length);

    free((void*)cl.client_ip);
    free(buffor);
    close(file_fd);
    close(cl.client_fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fatal("usage: %s <port>", argv[0]);
    }

    uint16_t port = read_port(argv[1]);

    // Create a folder
    if (mkdir(dir_name, 0755) < 0) {
        if (errno != EEXIST)
            fatal("Couldn't create a save directory!\n");
    }

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        syserr("cannot create a socket");
    }

    // Bind the socket to a concrete address.
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // Listening on all interfaces.
    server_address.sin_port = htons(port);

    if (bind(socket_fd, (struct sockaddr *) &server_address, (socklen_t) sizeof server_address) < 0) {
        syserr("bind");
    }

    // Switch the socket to listening.
    if (listen(socket_fd, QUEUE_LENGTH) < 0) {
        syserr("listen");
    }

    // Find out what port the server is actually listening on.
    socklen_t lenght = (socklen_t) sizeof server_address;
    if (getsockname(socket_fd, (struct sockaddr *) &server_address, &lenght) < 0) {
        syserr("getsockname");
    }

    printf("parent is listening on port %" PRIu16 "\n", ntohs(server_address.sin_port));

    for (;;) {
        struct sockaddr_in client_address;
        int client_fd = accept(socket_fd, (struct sockaddr *) &client_address,
                               &((socklen_t) {sizeof client_address}));
        if (client_fd < 0) {
            syserr("accept");
        }

        client_data * cl = malloc(sizeof(client_data));
        if (!cl)
            fatal("malloc\n");
        
        cl->client_ip = strdup(inet_ntoa(client_address.sin_addr));
        cl->client_port = ntohs(client_address.sin_port);
        printf("accepted connection from %s:%" PRIu16 "\n", cl->client_ip, cl->client_port);
        cl->client_fd = client_fd;

        pthread_t thread;
        if (pthread_create(&thread, 0, handle_connection, cl) != 0) {
            syserr("pthread_create"); // TODO: Punkt 5. Ćwiczenie 3.
        }
        else if (pthread_detach(thread) != 0) {
            syserr("pthread_detach");
        }
    }
    close(socket_fd);
    exit(0);
}
