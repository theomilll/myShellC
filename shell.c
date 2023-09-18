#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 100
#define MAX_COMMANDS 10

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

void executeCommand(char **parsedArgs, enum ExecStyle style) {
    int pipefd[2];
    pid_t pid1, pid2;

    // Check for pipe symbol
    int hasPipe = 0;
    char *parsedArgs2[MAX_ARG_SIZE];
    for (int i = 0; parsedArgs[i] != NULL; i++) {
        if (strcmp(parsedArgs[i], "|") == 0) {
            hasPipe = 1;
            parsedArgs[i] = NULL;  // Split the commands at the pipe symbol

            // Prepare for the second command
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

    // If there is a pipe
    if (hasPipe) {
        // Create pipe
        if (pipe(pipefd) < 0) {
            perror("pipe");
            return;
        }

        // Fork first process
        if ((pid1 = fork()) == 0) {
            // Redirect stdout to pipe
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            execvp(parsedArgs[0], parsedArgs);
            perror(parsedArgs[0]);
            exit(1);
        }

        // Fork second process
        if ((pid2 = fork()) == 0) {
            // Redirect stdin from pipe
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[1]);
            execvp(parsedArgs2[0], parsedArgs2);
            perror(parsedArgs2[0]);
            exit(1);
        }

        // Parent closes unused file descriptors and waits for children
        close(pipefd[0]);
        close(pipefd[1]);
        wait(NULL);
        wait(NULL);

    } else {
        // Check for input and output redirection here
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

        pid_t pid, wpid;
        int status;

        pid = fork();
        if (pid == 0) {
            // Child process

            // Handle input redirection if specified
            if (inputFile) {
                freopen(inputFile, "r", stdin);
            }

            // Handle output redirection if specified
            if (outputFile) {
                freopen(outputFile, "w", stdout);
            }

            if (execvp(parsedArgs[0], parsedArgs) < 0) {
                printf("Could not execute command: %s\n", parsedArgs[0]);
            }
            exit(0);
        } else {
            // Parent process
            if (style == SEQUENTIAL) {
                waitpid(pid, &status, 0);  // Wait for child to exit
            }
        }
        // printf("Debug: Command to execute: %s\n", parsedArgs[0]);

    // if (pid == 0) {
    //     // Child process

    //     // Debug print to see if inputFile and outputFile are correctly set
    //     if (inputFile) {
    //         printf("Debug: Input file: %s\n", inputFile);
    //     }
    //     if (outputFile) {
    //         printf("Debug: Output file: %s\n", outputFile);
    //     }

    //     if (execvp(parsedArgs[0], parsedArgs) < 0) {
    //         printf("Could not execute command: %s\n", parsedArgs[0]);
    //     }
    //     exit(0);
    // }
    }
}

int main() {
    // Define the history array and counter
    #define MAX_HISTORY 100
    char *commandHistory[MAX_HISTORY];
    int historyCount = 0;
    
    char input[MAX_INPUT_SIZE];
    char *parsedArgs[MAX_ARG_SIZE];
    char *commands[MAX_COMMANDS];
    enum ExecStyle style = SEQUENTIAL;  // Default style is sequential
    pid_t wpid;  // Declare wpid here

    while (1) {
        // Print the shell prompt
        printf("\nmyShell> ");
        
        // Take input from user
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            perror("Failed to read input");
            exit(EXIT_FAILURE);
        }
        
        // Remove trailing newline character from input
        input[strcspn(input, "\n")] = 0;
        
        // Add the command to history
        if (historyCount < MAX_HISTORY) {
            commandHistory[historyCount++] = strdup(input);
        } else {
            // Implement logic to remove the oldest command and add the newest
            // if the history is full.
        }

        // Split input into multiple commands separated by ;
        splitCommands(input, commands);

        for (int i = 0; commands[i] != NULL; i++) {
            // Parse each command to separate arguments and options
            parseInput(commands[i], parsedArgs);
            
            // Handle !! command
            if (strcmp(parsedArgs[0], "!!") == 0) {
                if (historyCount > 0) {
                    // Execute the last command from history
                    char *lastCommand = commandHistory[historyCount - 1];
                    printf("Executing last command: %s\n", lastCommand);  // Debug print

                    // Parse and execute this lastCommand
                    parseInput(lastCommand, parsedArgs);
                    executeCommand(parsedArgs, style);
                } else {
                    printf("No commands in history.\n");
                }
                continue;  // Skip to next iteration of loop
            }
            // Check for style change commands
            if (strcmp(parsedArgs[0], "style") == 0 && parsedArgs[1] != NULL) {
                // /* Your existing code here */
            }
            
            // Execute each command based on the style (Sequential/Parallel)
            executeCommand(parsedArgs, style);
        }
        
        // If it's parallel, wait for all child processes to complete
        if (style == PARALLEL) {
            while ((wpid = wait(NULL)) > 0);
        }
    }
    return 0;
}

