#include "header.h"

// write a shell-like server to handle the command from the client

#define BASIC_SERVER_PORT 8080
#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

// const char *prompt = ">";
const char *exit_command = "exit";
const char *pipe_command = "|";
const char *cd_command = "cd";

// analyze the command line
int parseLine(char *line, char *command_array[]) {
    char *p;
    int count = 0;
    p = strtok(line, " \n\t\r");
    while (p != NULL) {
        command_array[count] = p;
        count++;
        p = strtok(NULL, " \n\r\t");
    }
    command_array[count] = NULL;
    return count;
}

// execute the command
int execute_command(char **args, int num_args) {
    int pipe_index = -1;
    for (int i = 0; i < num_args; i++) {
        if (strcmp(args[i], pipe_command) == 0) {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index == -1) {
        pid_t pid;

        if (args[0] == NULL) {
            return 1;
        }

        pid = fork();
        if (pid == 0) {
            // child process
            if (execvp(args[0], args) == -1) {
                perror("Error");
                // write(STDOUT_FILENO, "Error: command not found\n", 25);
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // error forking
            perror("fork");
            return 1;
        } else {
            // parent process
            int r;
            wait(&r);
        }
    }

    else {
        if (pipe_index == 0 || pipe_index == num_args - 1) {
            perror("Error: Invalid pipe command\n");
            return 1;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return 1;
        }

        pid_t pid1 = fork();
        if (pid1 == 0) {
            // child process
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            args[pipe_index] = NULL;
            if (execvp(args[0], args) == -1) {
                perror("lsh");
            }
            exit(EXIT_FAILURE);
        } else if (pid1 < 0) {
            // error forking
            perror("fork");
            return 1;
        } else {
            // parent process
            wait(NULL);

            pid_t pid2 = fork();
            if (pid2 == 0) {
                // child process
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                execute_command(args + pipe_index + 1,
                                num_args - pipe_index - 1);
                exit(EXIT_FAILURE);
            } else if (pid2 < 0) {
                // error forking
                perror("fork");
                return 1;
            } else {
                // parent process
                close(pipefd[0]);
                close(pipefd[1]);
                wait(NULL);
                return 1;
            }
        }
    }

    return 1;
}

int getInfo(char *path) {
    char tmp[MAX_COMMAND_LENGTH];
    int length;

    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        strcpy(path, pw->pw_name);
    }
    length = strlen(path);
    strcat(path, "@");
    length += 1;

    gethostname(tmp, MAX_COMMAND_LENGTH);
    strcat(path, tmp);
    length += strlen(tmp);
    strcat(path, ":");
    length += 1;

    getcwd(tmp, MAX_COMMAND_LENGTH);
    strcat(path, tmp);
    length += strlen(tmp);
    strcat(path, "> ");
    length += 2;
    strcat(path, "\0");

    return length;
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        perror("Usage: ./shell <port>\n");
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int cli_sockfd;
    if (sockfd < 0) {
        perror("ERROR opening socket.\n");
        exit(2);
    }
    struct sockaddr_in serv_addr;

    // Initialize the server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(argc > 1 ? atoi(argv[1]) : BASIC_SERVER_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding.\n");
        exit(2);
    }

    // Listen for incoming connections
    listen(sockfd, 5);

    while (1) {
        cli_sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (cli_sockfd < 0) {
            perror("ERROR on accept.\n");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork.\n");
            close(cli_sockfd);
        }

        if (pid == 0) {
            close(sockfd);

            int status = 1;
            char *args[MAX_ARGS];
            int nread;
            char buffer[MAX_COMMAND_LENGTH];
            int num_args;
            char path[MAX_COMMAND_LENGTH];
            int length;

            // redirect the stdin, stdout, stderr to the client socket
            int new_stdout = dup(STDOUT_FILENO);
            dup2(cli_sockfd, STDOUT_FILENO);
            dup2(cli_sockfd, STDERR_FILENO);
            dup2(cli_sockfd, STDIN_FILENO);

            do {
                // printf("> ");
                length = getInfo(path);
                write(cli_sockfd, path, length);
                nread = read(cli_sockfd, buffer, MAX_COMMAND_LENGTH);
                if (nread <= 0) {
                    perror("ERROR reading from socket.\n");
                    break;
                }
                buffer[nread] = '\0';

                for (int i = 0; i < nread; i++) {
                    if (buffer[i] == '|') {
                        if (i > 0 && buffer[i - 1] != ' ') {
                            memmove(buffer + i + 1, buffer + i, nread - i);
                            buffer[i] = ' ';
                            nread++;
                            i++;
                        }
                        if (i < nread - 1 && buffer[i + 1] != ' ') {
                            memmove(buffer + i + 1, buffer + i, nread - i);
                            buffer[i + 1] = ' ';
                            nread++;
                            i++;
                        }
                    }
                }

                num_args = parseLine(buffer, args);  // analyze the command line

                if (args[0] == NULL) {
                    continue;
                }

                else if (strcmp(args[0], exit_command) == 0) {
                    break;
                }

                else if (strcmp(args[0], cd_command) == 0) {
                    if (num_args < 2) {
                        fprintf(stderr, "cd: expected argument to \"cd\"\n");
                    } else {
                        if (chdir(args[1]) != 0) {
                            perror("cd");
                        }
                    }
                    continue;
                }

                else {
                    status =
                        execute_command(args, num_args);  // execute the command
                }

                // free(buffer);
                // free(args);

            } while (status);
            close(cli_sockfd);
            exit(0);

        } else {
            close(cli_sockfd);
        }
    }
    return 0;
}