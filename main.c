#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 100
#define MAX_COMMANDS 10
#define MAX_HISTORY 100

enum ExecStyle {SEQUENTIAL, PARALLEL};

void parseInput(char *input, char **parsedArgs) {
    int i;
    for (i = 0; i < MAX_ARG_SIZE; i++) {
        parsedArgs[i] = strsep(&input, " ");
        if (parsedArgs[i] == NULL) break;
        if (strlen(parsedArgs[i]) == 0) i--;
    }
}

void splitCommands(char *input, char **commands) {
    int i;
    for (i = 0; i < MAX_COMMANDS; i++) {
        commands[i] = strsep(&input, ";");
        if (commands[i] == NULL) break;
    }
}

void executeCommand(char **parsedArgs, enum ExecStyle style, int background) {
    int pipefd[2];
    pid_t pid1, pid2;

    if (parsedArgs[0] == NULL) {
        printf("Error: No command entered.\n");
        return;
    }

    int hasPipe = 0;
    char *parsedArgs2[MAX_ARG_SIZE];
    for (int i = 0; parsedArgs[i] != NULL; i++) {
        if (strcmp(parsedArgs[i], "|") == 0) {
            hasPipe = 1;
            parsedArgs[i] = NULL;

            if (parsedArgs[i+1] == NULL) {
                printf("Error: No command after pipe symbol.\n");
                return;
            }

            int j = 0;
            i++;
            for (; parsedArgs[i] != NULL; i++, j++) {
                parsedArgs2[j] = parsedArgs[i];
            }
            parsedArgs2[j] = NULL;
            break;
        }
    }

    char *inputFile = NULL;
    char *outputFile = NULL;

    if (hasPipe) {
        if (pipe(pipefd) < 0) {
            perror("pipe");
            return;
        }

        if ((pid1 = fork()) == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            execvp(parsedArgs[0], parsedArgs);
            perror(parsedArgs[0]);
            exit(1);
        }

        if ((pid2 = fork()) == 0) {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[1]);
            execvp(parsedArgs2[0], parsedArgs2);
            perror(parsedArgs2[0]);
            exit(1);
        }

        close(pipefd[0]);
        close(pipefd[1]);
        wait(NULL);
        wait(NULL);

    } else {
        for (int i = 0; parsedArgs[i] != NULL; i++) {
            if (strcmp(parsedArgs[i], "<") == 0) {
                parsedArgs[i] = NULL;
                inputFile = parsedArgs[i + 1];
                break;
            }
            if (strcmp(parsedArgs[i], ">") == 0) {
                parsedArgs[i] = NULL;
                outputFile = parsedArgs[i + 1];
                break;
            }
        }

        pid_t pid;
        int status;

        pid = fork();
        if (pid < 0) {
            perror("Error in fork");
            return;
        }
        if (pid == 0) {
            if (inputFile) {
                freopen(inputFile, "r", stdin);
            }
            if (outputFile) {
                freopen(outputFile, "w", stdout);
            }
            if (execvp(parsedArgs[0], parsedArgs) < 0) {
                printf("Could not execute command: %s\n", parsedArgs[0]);
            }
            if (inputFile && freopen(inputFile, "r", stdin) == NULL) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
            }
            if (outputFile && freopen(outputFile, "w", stdout) == NULL) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
        } else {
            if (style == SEQUENTIAL && !background) {
                waitpid(pid, &status, 0);
            }
        }
        if (background) {
            printf("[1] %d\n", pid);
        }
    }
}

void *threadExecuteCommand(void *arg) {
    char **parsedArgs = (char **) arg;
    executeCommand(parsedArgs, PARALLEL, 0);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    char *commandHistory[MAX_HISTORY];
    int historyCount = 0;
    pthread_t threads[MAX_COMMANDS];

    char input[MAX_INPUT_SIZE];
    char *parsedArgs[MAX_ARG_SIZE];
    char *commands[MAX_COMMANDS];
    enum ExecStyle style = SEQUENTIAL;
    pid_t wpid;

    FILE *batchFile = NULL;

    if (argc == 2) {
        batchFile = fopen(argv[1], "r");
        if (batchFile == NULL) {
            perror("Error opening batch file");
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        if (batchFile) {
            if (fgets(input, MAX_INPUT_SIZE, batchFile) == NULL) {
                fclose(batchFile);
                break;
            }
        } else {
            printf("\ntam4 %s> ", (style == SEQUENTIAL) ? "seq" : "par");
            if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
                perror("Failed to read input");
                exit(EXIT_FAILURE);
            }
        }
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) {
            printf("Error: Empty command.\n");
            continue;
        }

        if (strcmp(input, "exit") == 0) {
            printf("Exiting shell...\n");
            break;
        }

        if (strcmp(input, "!!") == 0) {
            if (historyCount > 0) {
                strcpy(input, commandHistory[historyCount - 1]);
            } else {
                printf("No commands in history.\n");
                continue;
            }
        } else {
            if (historyCount < MAX_HISTORY) {
                commandHistory[historyCount++] = strdup(input);
            }
        }

        splitCommands(input, commands);

        for (int i = 0; commands[i] != NULL; i++) {
            parseInput(commands[i], parsedArgs);

            int background = 0;
            for (int j = 0; parsedArgs[j] != NULL; j++) {
                if (strcmp(parsedArgs[j], "&") == 0) {
                    parsedArgs[j] = NULL;
                    background = 1;
                    break;
                }
            }

            if (strcmp(parsedArgs[0], "style") == 0 && parsedArgs[1] != NULL) {
                if (strcmp(parsedArgs[1], "sequential") == 0) {
                    style = SEQUENTIAL;
                } else if (strcmp(parsedArgs[1], "parallel") == 0) {
                    style = PARALLEL;
                }
                continue;
            }

            if (style == SEQUENTIAL) {
                executeCommand(parsedArgs, style, background);
            } else {
                pthread_create(&threads[i], NULL, threadExecuteCommand, (void *)parsedArgs);
            }
        }

        if (style == PARALLEL) {
            for (int i = 0; commands[i] != NULL; i++) {
                pthread_join(threads[i], NULL);
            }
        }
    }
    exit(0);
    return 0;
}



