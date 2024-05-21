#include "header.h"

// Copy the contents of the source file to the destination file by using fork

int main(int argc, char *argv[]) {
    pid_t ForkPid;
    ForkPid = fork();
    switch (ForkPid) {
        case -1:
            fprintf(stderr, "Error: Fail to fork.\n");
            return -1;
        case 0:
            execl("./MyCopy", argv[0], argv[1], argv[2], NULL);
            break;
        default:
            wait(NULL);
            break;
    }

    if (argc != 3) {
        printf("Usage: %s <source> <destination>\n", argv[0]);
        return -1;
    }

    return 0;
}