/* ********************************
 *
 */
/** @file tcp_utils.h */ /*
                          *
                          ********************************/

#include "tcp_utils.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "thpool.h"

struct tcp_server_pool {     // Represents a pool of connected descriptors
    int maxfd;               // Largest descriptor in read_set
    fd_set read_set;         // Set of all active descriptors
    fd_set ready_set;        // Subset of descriptors ready for reading
    int nready;              // Number of ready descriptors from select
    int maxi;                // High water index into client array
    int connfd[FD_SETSIZE];  // Set of active descriptors
    pthread_mutex_t mutex[FD_SETSIZE];
    struct tcp_buffer *read_buf[FD_SETSIZE];
    struct tcp_buffer *write_buf[FD_SETSIZE];
};

typedef struct tcp_server_ {
    void (*add_client)(int id);
    int (*handle_client)(int id, tcp_buffer *write_buf, char *msg, int len);
    void (*clear_client)(int id);
    int port;
    int listenfd;
    struct tcp_server_pool pool;
    threadpool thpool;
} tcp_server_;

typedef struct tcp_client_ {
    int sockfd;
    struct tcp_buffer *read_buf;
    struct tcp_buffer *write_buf;
} tcp_client_;

/* Initialize a pool */
void init_pool(int listenfd, struct tcp_server_pool *p) {
    p->maxi = 1;
    for (int i = 0; i < FD_SETSIZE; i++) p->connfd[i] = -1;
    for (int i = 0; i < FD_SETSIZE; i++) pthread_mutex_init(&p->mutex[i], NULL);

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

/* Add a new connection to the pool */
void add_conn(int connfd, struct tcp_server_pool *p, void (*add_client)(int)) {
    int i;
    p->nready--;
    // find an available slot
    for (i = 0; i < FD_SETSIZE; i++) {
        if (p->connfd[i] < 0) {
            p->connfd[i] = connfd;
            p->read_buf[i] = init_buffer();
            p->write_buf[i] = init_buffer();
            if (add_client) add_client(i);
            printf("New client: %d\n", connfd);
            FD_SET(connfd, &p->read_set);
            if (connfd > p->maxfd) p->maxfd = connfd;
            if (i > p->maxi) p->maxi = i;
            break;
        }
    }
    if (i == FD_SETSIZE) printf("Too many clients");
}

/* Arguments for handle_read */
typedef struct handle_read_args {
    tcp_server_ *server;
    int i;
} handle_read_args;

/* Handle read, running in a thread */
void handle_read(void *arg_p) {
    handle_read_args *arg = (handle_read_args *)arg_p;
    tcp_server_ *server = arg->server;
    int i = arg->i;
    struct tcp_server_pool *p = &server->pool;
    int connfd = p->connfd[i];
    free(arg);

    struct tcp_buffer *read_buf = p->read_buf[i];
    struct tcp_buffer *write_buf = p->write_buf[i];

    // read all data from the socket
    int count = buffer_input(read_buf, connfd);
    int close_flag = 0;

    // handle
    if (count > 0) {
        printf("Server received %d bytes on fd %d\n", count, connfd);

        while (1) {  // handle all messages in the buffer
            int readable = read_buf->write_index - read_buf->read_index;
            char *s = &read_buf->buf[read_buf->read_index];
            // the first 4 bytes is the length of the message
            if (readable < 4) break;
            // network long to host long
            int len = ntohl(*(int *)s);
            // if the message is complete
            if (readable >= len + 4) {
                if (server->handle_client(i, write_buf, s + 4, len) < 0)
                    close_flag = 1;
                recycle_read(read_buf, len + 4);
            } else
                break;
        }
    }

    // write
    buffer_output(write_buf, connfd);

    if (count < 0 || close_flag) {
        printf("client %d exited\n", connfd);
        free(p->read_buf[i]);
        free(p->write_buf[i]);
        if (server->clear_client) server->clear_client(i);
        close(connfd);
        FD_CLR(connfd, &p->read_set);
        p->connfd[i] = -1;
    }

    // locked in server_loop, before the task is added
    // so unlock here
    pthread_mutex_unlock(&p->mutex[i]);
}

/* Initialize a server */
tcp_server_ *server_init(int port, int num_threads, void (*add_client)(int id),
                         int (*handle_client)(int id, tcp_buffer *write_buf,
                                              char *msg, int len),
                         void (*clear_client)(int id)) {
    tcp_server_ *server = malloc(sizeof(tcp_server_));
    server->port = port;
    server->add_client = add_client;
    server->handle_client = handle_client;
    server->clear_client = clear_client;

    if (!handle_client) {
        fprintf(stderr, "handle_client() cannot be NULL\n");
        exit(EXIT_FAILURE);
    }

    // create listen socket
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    server->listenfd = listenfd;

    // set SO_REUSEADDR, avoid bind error
    int val = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    // bind the socket to ANY localhost address
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server->port);

    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    // start listening
    if (listen(listenfd, 10) < 0) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    init_pool(listenfd, &server->pool);
    server->thpool = thpool_init(num_threads);

    printf("Start listening on port %d...\n", server->port);
    return server;
}

/* Start the server loop, never returns */
void server_loop(tcp_server_ *server) {
    while (1) {
        server->pool.ready_set = server->pool.read_set;
        // wait for a client to be ready
        server->pool.nready = select(server->pool.maxfd + 1,
                                     &server->pool.ready_set, NULL, NULL, NULL);
        // if listenfd is ready, a new client is connecting
        if (FD_ISSET(server->listenfd, &server->pool.ready_set)) {
            // handle new client
            int connfd = accept(server->listenfd, NULL, NULL);
            if (connfd < 0) {
                perror("accept()");
                exit(EXIT_FAILURE);
            }
            int flag = fcntl(connfd, F_GETFL, 0);
            fcntl(connfd, F_SETFL, flag | O_NONBLOCK);
            flag = fcntl(connfd, F_GETFL, 0);
            if (!(flag & O_NONBLOCK)) {
                fprintf(stderr, "set nonblock error\n");
            }
            add_conn(connfd, &server->pool, server->add_client);
        }

        struct tcp_server_pool *p = &server->pool;

        // handle all readable clients
        for (int i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
            int connfd = p->connfd[i];
            if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
                // make sure only one thread is handling this client
                // if the mutex is locked, skip
                if (pthread_mutex_trylock(&p->mutex[i]) == 0) {
                    p->nready--;
                    handle_read_args *arg = malloc(sizeof(handle_read_args));
                    arg->server = server;
                    arg->i = i;
                    // add the task to the threadpool
                    thpool_add_work(server->thpool, handle_read, (void *)arg);
                }
            }
        }
    }
    // no break in the loop, so never reach here
    close(server->listenfd);
}

/* Initialize a client */
tcp_client_ *client_init(const char *hostname, int port) {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr;
    struct hostent *host;
    serv_addr.sin_family = AF_INET;
    host = gethostbyname(hostname);
    if (host == NULL) {
        perror("gethostbyname()");
        exit(EXIT_FAILURE);
    }
    memcpy(&serv_addr.sin_addr.s_addr, host->h_addr, host->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    tcp_client_ *client = malloc(sizeof(tcp_client_));
    client->sockfd = sockfd;
    client->read_buf = init_buffer();
    client->write_buf = init_buffer();
    return client;
}

/* Send a message to the server */
void client_send(tcp_client_ *client, const char *msg, int len) {
    send_to_buffer(client->write_buf, msg, len);
    buffer_output(client->write_buf, client->sockfd);
}

/* Receive a message from the server */
int client_recv(tcp_client_ *client, char *buf, int max_len) {
    tcp_buffer *read_buf = client->read_buf;
    // read all data from the socket
    while (1) {
        int count = buffer_input(read_buf, client->sockfd);
        if (count <= 0) {
            printf("Connection closed\n");
            return 0;
        }
        int readable = read_buf->write_index - read_buf->read_index;
        char *s = &read_buf->buf[read_buf->read_index];
        // the first 4 bytes is the length of the message
        if (readable < 4) continue;
        // network long to host long
        int len = ntohl(*(int *)s);
        // if the message is complete
        if (readable >= len + 4) {
            if (len > max_len) {
                fprintf(stderr, "client_recv: buffer too small\n");
                exit(EXIT_FAILURE);
            }
            // copy the message to buf
            memcpy(buf, s + 4, len);
            recycle_read(read_buf, len + 4);
            return len;
        } else
            continue;
    }
}

/* Destroy the client */
void client_destroy(tcp_client_ *client) {
    close(client->sockfd);
    free(client->read_buf);
    free(client->write_buf);
    free(client);
}
