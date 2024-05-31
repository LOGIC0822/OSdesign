/* ********************************
 * Description:  A simple TCP server and client library
 ********************************/

#ifndef _TCP_UTILS_
#define _TCP_UTILS_

#define TCP_BUF_SIZE 4096

#include "tcp_buffer.h"

typedef struct tcp_server_ *tcp_server;
typedef struct tcp_client_ *tcp_client;
/* Initialize a server */
tcp_server server_init(int port, int num_threads, void (*add_client)(int id),
                       int (*handle_client)(int id, tcp_buffer *write_buf,
                                            char *msg, int len),
                       void (*clear_client)(int id));

/* Start the server loop, never returns */
void server_loop(tcp_server server);

/* Initialize a client */
tcp_client client_init(const char *hostname, int port);

/* Send a message to the server */
void client_send(tcp_client client, const char *msg, int len);

/* Receive a message from the server */
int client_recv(tcp_client client, char *buf, int max_len);

/* Destroy the client */
void client_destroy(tcp_client client);

#endif
