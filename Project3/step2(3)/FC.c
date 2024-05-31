#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// extern "C" {

#include "tcp_utils.h"
// }

#define FS_PORT 12356

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 2) {
        fprintf(stderr, "Usage: %s <ServerAddr> <Port>", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = (argc == 3) ? atoi(argv[2]) : FS_PORT;
    char *server_addr = argv[1];
    tcp_client client = client_init(server_addr, port);
    static char buf[4096];
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (feof(stdin)) break;
        client_send(client, buf, strlen(buf) + 1);
        // printf("Sent: %s\n", buf);
        int n = client_recv(client, buf, sizeof(buf));
        if (n < 0) {
            perror("client_recv");
            break;
        }
        buf[n] = 0;
        printf("%s\n", buf);
        if (strcmp(buf, "Bye!") == 0) break;
    }
    client_destroy(client);
}
