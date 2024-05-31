#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "tcp_utils.h"

#define BASIC_SERVER_PORT 10356
#define BUFFER_SIZE 1024

const char charset[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789! \t\n\r\0 "
    "@#$%^&*()_+"
    "{}|:<>?[];',./-=`~";
const int charset_size = sizeof(charset) - 1;  // 不包括null终止符

char *generate_random_string(int length) {
    char *random_string = (char *)malloc(length + 1);  // +1 为null终止符
    if (!random_string) return NULL;

    for (int i = 0; i < length; ++i) {
        random_string[i] = charset[rand() % charset_size];
    }
    random_string[length] = '\0';  // 添加null终止符

    return random_string;
}

int main(int argc, char *argv[]) {
    if (argc > 3) {
        fprintf(stderr, "Usage: %s <instructions> <Port>", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = (argc == 3) ? atoi(argv[2]) : BASIC_SERVER_PORT;
    int instructions = atoi(argv[1]);
    srand(time(NULL));
    tcp_client client = client_init("localhost", port);
    static char buf[BUFFER_SIZE];
    client_send(client, "I ", 3 * sizeof(char));
    int n = client_recv(client, buf, BUFFER_SIZE);
    buf[n] = 0;
    // printf("%s\n", buf);
    char *p = strtok(buf, " \r\n\t");
    int cylinders = atoi(p);
    p = strtok(NULL, " \r\n\t");
    int sectors_per_cylinder = atoi(p);
    printf("cylinders: %d, sectors_per_cylinder: %d, instructions: %d\n",
           cylinders, sectors_per_cylinder, instructions);
    while (1) {
        if (instructions == 0) break;
        if (rand() % 2 == 0) {
            int cylinder = rand() % cylinders;
            int sector = rand() % sectors_per_cylinder;
            sprintf(buf, "R %d %d", cylinder, sector);
            instructions--;
        } else {
            int cylinder = rand() % cylinders;
            int sector = rand() % sectors_per_cylinder;
            char *data = generate_random_string(255);
            sprintf(buf, "W %d %d 256 %s", cylinder, sector, data);
            instructions--;
            free(data);
        }
        printf("%s\n", buf);
        client_send(client, buf, strlen(buf) + 1);
        int n = client_recv(client, buf, sizeof(buf));
        buf[n] = 0;
        printf("%s\n", buf);

        if (strcmp(buf, "Bye!") == 0) break;
    }
    client_destroy(client);
}
