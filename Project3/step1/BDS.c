#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp_utils.h"

typedef struct {
    char *filename;
    int fd;
    int num_cylinders;
    int sectors_per_cylinder;
    double track_to_track_delay;
    char *diskfile;
    int port;
    // FILE *log_fp;
} SimDisk;

// Block size in bytes
#define SECTOR_SIZE 256
#define BUFFER_SIZE 128
#define BASIC_SERVER_PORT 10356

SimDisk disk;
int current_cylinder = 0;

int notin(char c, const char *delim) {
    for (int i = 0; i < strlen(delim); i++) {
        if (c == delim[i]) {
            return 0;
        }
    }
    return 1;
}

char **strspace(
    char *token, const char *delim,
    int count) {  // To ensure the space in data can be read correctly
    // length is the length of the last token
    char **tokens = (char **)malloc(count * sizeof(char *));
    for (int i = 0; i < count; i++) {
        tokens[i] = (char *)malloc(
            300 * sizeof(char));  // Assume max token length is 256
    }
    int i = 0;
    int c = 0;
    while (c < count && token[i] != '\0') {
        if (!notin(token[i], delim)) {
            while (!notin(token[i], delim) && token[i] != '\0') {
                i++;
            }
        } else {
            int j = 0;
            if (c + 1 < count) {
                while (notin(token[i], delim) && token[i] != '\0') {
                    tokens[c][j] = token[i];
                    i++;
                    j++;
                }
                tokens[c][j] = '\0';
                c++;
            } else
                break;
        }
        int length = atoi(tokens[c - 1]);
        memcpy(tokens[c], token + i, length);
        tokens[c][length] = '\0';
    }
    return tokens;
}

// return a negative value to exit
int cmd_i(tcp_buffer *write_buf, char *args, int len) {
    static char buf[64];
    sprintf(buf, "%d %d", disk.num_cylinders, disk.sectors_per_cylinder);

    // send to buffer, including the null terminator
    // printf("I\n");
    send_to_buffer(write_buf, buf, strlen(buf) + 1);
    return 0;
}

int cmd_r(tcp_buffer *write_buf, char *args, int len) {
    // printf("R\n");
    char *p = strtok(args, " \r\n\t");
    int cylinder = atoi(p);
    p = strtok(NULL, " \r\n\t");
    int sector = atoi(p);
    printf("R %d %d\n", cylinder, sector);

    if (cylinder < 0 || cylinder >= disk.num_cylinders || sector < 0 ||
        sector >= disk.sectors_per_cylinder) {
        send_to_buffer(write_buf, "Read data from disk failed!", 28);
        return 0;
    }

    int delta = abs(cylinder - current_cylinder);
    usleep(delta * disk.track_to_track_delay * 1000);  // s
    current_cylinder = cylinder;

    char *sector_data =
        disk.diskfile +
        (cylinder * disk.sectors_per_cylinder + sector) * SECTOR_SIZE;
    send_to_buffer(write_buf, sector_data, SECTOR_SIZE);
    return 0;
}

int cmd_w(tcp_buffer *write_buf, char *args, int len) {
    printf("%s\n", args);
    char **tokens = strspace(args, " \n\t\r", 4);
    char *data = tokens[3];
    int cylinder = atoi(tokens[0]), sector = atoi(tokens[1]),
        length = atoi(tokens[2]);
    printf("W %d %d %d %s \n", cylinder, sector, length, data);
    // printf("data: %s\n", data);

    if (cylinder < 0 || cylinder >= disk.num_cylinders || sector < 0 ||
        sector >= disk.sectors_per_cylinder || length > SECTOR_SIZE) {
        // printf("Instruction error!\n");
        send_to_buffer(write_buf, "No", 3);
        return 0;
    }

    int delta = abs(cylinder - current_cylinder);
    usleep(delta * disk.track_to_track_delay * 1000);  // s
    current_cylinder = cylinder;

    char *sector_data =
        disk.diskfile +
        (cylinder * disk.sectors_per_cylinder + sector) * SECTOR_SIZE;
    memcpy(sector_data, data, length);

    send_to_buffer(write_buf, "Yes", 4);

    return 0;
}

int cmd_e(tcp_buffer *write_buf, char *args, int len) {
    // printf("E\n");
    send_to_buffer(write_buf, "Bye!", 5);
    return -1;
}

static struct {
    const char *name;
    int (*handler)(tcp_buffer *write_buf, char *, int);
} cmd_table[] = {
    {"I", cmd_i},
    {"R", cmd_r},
    {"W", cmd_w},
    {"E", cmd_e},
};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

void add_client(int id) {
    // some code that are executed when a new client is connected
    // you don't need this in step1
}

int handle_client(int id, tcp_buffer *write_buf, char *msg, int len) {
    // printf("msg: %s\n", msg);
    char *p = strtok(msg, " \r\n\t");
    int ret = 1;
    if (p != NULL) {
        for (int i = 0; i < NCMD; i++)
            if (strcmp(p, cmd_table[i].name) == 0) {
                ret = cmd_table[i].handler(write_buf, p + strlen(p) + 1,
                                           len - strlen(p) - 1);
                break;
            }
    } else {
        ret = 1;
    }
    if (ret == 1) {
        static char unk[] = "Unknown command";
        send_to_buffer(write_buf, unk, sizeof(unk));
    }
    if (ret < 0) {
        return -1;
    }
}

void clear_client(int id) {
    // some code that are executed when a client is disconnected
    // you don't need this in step2
    // free(disk.filename);

    // // Unmap the disk storage file from memory
    // if (munmap(disk.diskfile, disk.num_cylinders * disk.sectors_per_cylinder
    // * SECTOR_SIZE) == -1)
    // {
    //     perror("Error unmapping disk storage file");
    // }

    // // Close the disk storage file
    // if (close(disk.fd) == -1)
    // {
    //     perror("Error closing disk storage file");
    // }
}

void Init(SimDisk *disk, char *filename, int num_cylinders,
          int sectors_per_cylinder, double track_to_track_delay, int port) {
    disk->filename = filename;
    disk->num_cylinders = num_cylinders;
    disk->sectors_per_cylinder = sectors_per_cylinder;
    disk->track_to_track_delay = track_to_track_delay;
    disk->port = port;
}

int main(int argc, char *argv[]) {
    if (argc != 6 && argc != 5) {
        fprintf(stderr,
                "Usage: %s <disk file name> <cylinders> <sector per cylinder> "
                "<track-to-track delay> <port>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
    // args
    Init(&disk, argv[1], atoi(argv[2]), atoi(argv[3]), atof(argv[4]),
         (argc == 5) ? BASIC_SERVER_PORT : atoi(argv[5]));

    // open file
    disk.fd = open(disk.filename, O_RDWR | O_CREAT, 0666);
    if (disk.fd < 0) {
        printf("Error: Could not open file '%s'.\n", disk.filename);
        exit(-1);
    }

    // Calculate the disk storage size
    long filesize =
        disk.num_cylinders * disk.sectors_per_cylinder * SECTOR_SIZE;

    int result = lseek(disk.fd, filesize - 1, SEEK_SET);
    if (result == -1) {
        perror("Error calling lseek() to 'stretch' the file");
        close(disk.fd);
        exit(-1);
    }

    result = write(disk.fd, "", 1);
    if (result != 1) {
        perror("Error writing last byte of the file");
        close(disk.fd);
        exit(-1);
    }

    // Map the disk storage file to memory
    disk.diskfile = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, disk.fd, 0);
    if (disk.diskfile == MAP_FAILED) {
        close(disk.fd);
        printf("Error: Could not map file.\n");
        exit(-1);
    }

    // command

    tcp_server server =
        server_init(disk.port, 1, add_client, handle_client, clear_client);
    server_loop(server);
}
