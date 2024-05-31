#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp_utils.h"

#define BASIC_SERVER_PORT 10356

int main(int argc, char *argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s <Port>", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = (argc == 2) ? atoi(argv[1]) : BASIC_SERVER_PORT;
    tcp_client client = client_init("localhost", port);
    static char buf[4096];
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (feof(stdin)) break;
        client_send(client, buf, strlen(buf) + 1);
        int n = client_recv(client, buf, sizeof(buf));
        buf[n] = 0;
        printf("%s\n", buf);
        if (strcmp(buf, "Bye!") == 0) break;
    }
    client_destroy(client);
}
