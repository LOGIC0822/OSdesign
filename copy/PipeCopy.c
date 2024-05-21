// implement by children process

#include "header.h"

// Copy the contents of the source file to the destination file by using pipe

int main(int argc, char *argv[]) {
    int myPipe[2];
    int buffer_size = 1024;
    clock_t start, end;
    double elapsed;

    if (pipe(myPipe) == -1) {
        fprintf(stderr, "Error: Fail to create pipe.\n");
        return -1;
    }

    pid_t writer = fork();
    if (writer == -1) {
        fprintf(stderr, "Error: Fail to create child process.\n");
        return -1;
    } else if (writer == 0) {
        // Child process
        close(myPipe[0]);  // Close the read end of the pipe
        char *buffer1[buffer_size];

        // Open the source file
        FILE *sourceFile = fopen(argv[1], "r");
        if (sourceFile == NULL) {
            fprintf(stderr, "Error: Fail to open source file.\n");
            return -1;
        }

        // Read from the source file and write to the pipe

        size_t bytesRead;

        while (bytesRead = fread(buffer1, 1, buffer_size, sourceFile)) {
            write(myPipe[1], &buffer1, bytesRead);
        }

        // Close the write end of the pipe and the source file
        close(myPipe[1]);
        fclose(sourceFile);
        return 0;
    } else {
        start = clock();
        close(myPipe[1]);
        wait(NULL);
    }

    pid_t reader = fork();

    if (reader == -1) {
        fprintf(stderr, "Error: Fail to create child process.\n");
        return -1;
    } else if (reader == 0) {
        // Child process
        close(myPipe[1]);  // Close the write end of the pipe
        char *buffer2[buffer_size];

        // Open the destination file
        FILE *destFile = fopen(argv[2], "w+");
        if (destFile == NULL) {
            fprintf(stderr, "Error: Fail to open destination file.\n");
            return -1;
        }

        // Read from the pipe and write to the destination file

        size_t bytesRead;
        while ((bytesRead = read(myPipe[0], &buffer2, buffer_size))) {
            fwrite(buffer2, 1, bytesRead, destFile);
        }

        // Close the read end of the pipe and the destination file
        close(myPipe[0]);
        fclose(destFile);
        return 0;
    } else {
        start = clock();
        close(myPipe[0]);
        wait(NULL);
    }

    if (argc != 3) {
        printf("Usage: %s <source> <destination>\n", argv[0]);
        return -1;
    }

    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("Time used: %f second(s)\n", elapsed);
    printf("File copied successfully.\n");
    return 0;
}

// implement by father and child process

// #include "header.h"

// int main(int argc, char *argv[]) {
//     int myPipe[2];
//     int buffer_size = 1024;
//     clock_t start, end;
//     double elapsed;

//     if (pipe(myPipe) == -1) {
//         fprintf(stderr, "Error: Fail to create pipe.\n");
//         return -1;
//     }

//     pid_t pid = fork();

//     if (pid == -1) {
//         fprintf(stderr, "Error: Fail to create child process.\n");
//         return -1;
//     } else if (pid == 0) {
//         start = clock();
//         // Child process
//         close(myPipe[0]);  // Close the read end of the pipe
//         char *buffer1[buffer_size];

//         // Open the source file
//         FILE *sourceFile = fopen(argv[1], "r");
//         if (sourceFile == NULL) {
//             fprintf(stderr, "Error: Fail to open source file.\n");
//             return -1;
//         }

//         // Read from the source file and write to the pipe

//         size_t bytesRead;

//         while (bytesRead = fread(buffer1, 1, buffer_size, sourceFile)) {
//             write(myPipe[1], &buffer1, bytesRead);
//         }

//         // Close the write end of the pipe and the source file
//         close(myPipe[1]);
//         fclose(sourceFile);
//     } else {
//         start = clock();
//         close(myPipe[1]);  // Close the write end of the pipe
//         char *buffer2[buffer_size];

//         // Open the destination file
//         FILE *destFile = fopen(argv[2], "w+");
//         if (destFile == NULL) {
//             fprintf(stderr, "Error: Fail to open destination file.\n");
//             return -1;
//         }

//         // Read from the pipe and write to the destination file
//         size_t bytesRead;
//         while (bytesRead = read(myPipe[0], &buffer2, buffer_size)) {
//             fwrite(buffer2, 1, bytesRead, destFile);
//         }

//         // Close the read end of the pipe and the destination file
//         close(myPipe[0]);
//         fclose(destFile);
//     }

//     if (argc != 3) {
//         printf("Usage: %s <source> <destination>\n", argv[0]);
//         return -1;
//     }

//     printf("File copied successfully.\n");
//     end = clock();
//     elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
//     printf("Time used: %f millisecond\n", elapsed);

//     return 0;
// }