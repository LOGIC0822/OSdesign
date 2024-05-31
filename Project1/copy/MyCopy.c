#include "header.h"

// Copy the contents of the source file to the destination file

int copy_file(const char *target, const char *dest) {
    FILE *source_file, *dest_file;
    size_t buffer_size = 1024;
    char *buffer[buffer_size];

    // Open the source file in read mode
    source_file = fopen(target, "r");
    if (source_file == NULL) {
        printf("Error: Couldn't open file '%s'.\n", target);
        return -1;
    }

    // Open the destination file in write mode
    dest_file = fopen(dest, "w+");
    if (dest_file == NULL) {
        printf("Error: Couldn't open file'%s'.\n", dest);
        fclose(source_file);
        return -1;
    }

    size_t read_bytes;
    // Copy the contents of the source file to the destination file
    while (read_bytes = fread(buffer, 1, buffer_size, source_file))

        fwrite(buffer, 1, read_bytes, dest_file);

    // Close the files
    fclose(source_file);
    fclose(dest_file);

    printf("File copied successfully.\n");
    return 666;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <source> <destination>\n", argv[0]);
        return -1;
    }

    clock_t start, end;
    double elapsed;
    start = clock();

    copy_file(argv[1], argv[2]);

    end = clock();
    elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("Time used: %f second(s)\n", elapsed);
    return 0;
}
