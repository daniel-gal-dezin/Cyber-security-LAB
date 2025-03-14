#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "LineParser.h"
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

// Add this helper function after the includes
cmdLine* copyCommand(cmdLine* cmd) {
    if (!cmd) return NULL;
    
    cmdLine* newCmd = (cmdLine*)malloc(sizeof(cmdLine));
    if (!newCmd) return NULL;
    memset(newCmd, 0, sizeof(cmdLine));  // Initialize structure to zero
    
    // Copy arguments
   
    newCmd->argCount = cmd->argCount;
    
    // Copy redirections
    newCmd->inputRedirect = cmd->inputRedirect ? strdup(cmd->inputRedirect) : NULL;
    newCmd->outputRedirect = cmd->outputRedirect ? strdup(cmd->outputRedirect) : NULL;
    
    newCmd->blocking = cmd->blocking;
    newCmd->next = NULL;
    
    return newCmd;
}

// Forward declarations
struct process;
void updateProcessList(struct process **process_list);
void freeProcessList(struct process *process_list);
void addProcess(struct process **process_list, cmdLine* cmd, pid_t pid);
void printProcessList(struct process **process_list);

// Move struct definition before first usage
typedef struct process {
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid;                           /* the process id that is running the command*/
    int status;                          /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;                /* next process in chain */
} process;

// Then declare global variable
process* process_list = NULL;

#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0

#define HISTLEN 10

typedef struct history_node {
    char* cmd_str;
    struct history_node* next;
} history_node;

typedef struct history_list {
    history_node* head;
    history_node* tail;
    int size;
} history_list;

history_list* hist = NULL;

// Update addProcess function
void addProcess(process** process_list, cmdLine* cmd, pid_t pid) {
    process* newProcess = malloc(sizeof(process));
    if (newProcess == NULL) {
        perror("malloc failed");
        return;
    }
    
    // Create deep copy of command
    newProcess->cmd = copyCommand(cmd);
    if (newProcess->cmd == NULL) {
        free(newProcess);
        perror("command copy failed");
        return;
    }
    
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = *process_list;
    *process_list = newProcess;
}

void printProcessList(process** process_list) {
    // Update status of all processes
    updateProcessList(process_list);
    process* curr = *process_list;
    process* prev = NULL;
    printf("INDEX\tPID\tSTATUS\t\tCOMMAND\n");
    int index = 0;
    
    while (curr != NULL) {
        // Check process status
        int status;
        pid_t result = waitpid(curr->pid, &status, WNOHANG);
        
        // Update status
        if (result == curr->pid) {
            curr->status = TERMINATED;
        }
        
        // Convert status to string
        char* status_str;
        switch (curr->status) {
            case TERMINATED:
                status_str = "Terminated";
                break;
            case RUNNING:
                status_str = "Running";
                break;
            case SUSPENDED:
                status_str = "Suspended";
                break;
            default:
                status_str = "Unknown";
        }
        
        // Print process info
        printf("%d\t%d\t%s\t", index, curr->pid, status_str);
        
        // Print command with arguments
        for (int i = 0; curr->cmd->arguments[i] != NULL; i++) {
            printf("%s ", curr->cmd->arguments[i]);
        }
        printf("\n");
        
        // Remove if terminated
        if (curr->status == TERMINATED) {
            process* to_remove = curr;
            if (prev == NULL) {
                *process_list = curr->next;
                curr = curr->next;
            } else {
                prev->next = curr->next;
                curr = curr->next;
            }
            freeCmdLines(to_remove->cmd);
            free(to_remove);
        } else {
            prev = curr;
            curr = curr->next;
        }
        index++;
    }
}

void freeProcessList(process* process_list) {
    process* current = process_list;
    while (current != NULL) {
        process* next = current->next;
        freeCmdLines(current->cmd);
        free(current);
        current = next;
    }
}

void updateProcessStatus(process* process_list, int pid, int status) {
    process* current = process_list;
    while (current != NULL) {
        if (current->pid == pid) {
            current->status = status;
            return;
        }
        current = current->next;
    }
}

void updateProcessList(process **process_list) {
    process* current = *process_list;
    while (current != NULL) {
        int status;
        pid_t result = waitpid(current->pid, &status, WNOHANG);
        
        if (result == -1) {
            // Process no longer exists
            current->status = TERMINATED;
        } else if (result > 0) {
            // Process terminated
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                current->status = TERMINATED;
            } else if (WIFSTOPPED(status)) {
                current->status = SUSPENDED;
            } else if (WIFCONTINUED(status)) {
                current->status = RUNNING;
            }
        }
        current = current->next;
    }
}

void stopProcess(pid_t pid) {
    if (kill(pid, SIGSTOP) == -1) {
        perror("stop failed");
    }
}

void wakeProcess(pid_t pid) {
    if (kill(pid, SIGCONT) == -1) {
        perror("wake failed");
    }
}

void termProcess(pid_t pid) {
    if (kill(pid, SIGINT) == -1) {
        perror("terminate failed");
    }
}

void executePiped(cmdLine *cmd1, cmdLine *cmd2) {
    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    if (cmd1->outputRedirect) {
        fprintf(stderr, "Error: Output redirection for the left-hand-side process is not allowed.\n");
        return;
    }

    if (cmd2->inputRedirect) {
        fprintf(stderr, "Error: Input redirection for the right-hand-side process is not allowed.\n");
        return;
    }

    // First child (cmd1)

    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        return;
    }

    if (pid1 == 0) {  // First child
        // Redirect stdout to pipe write end
        close(pipefd[0]);  // Close read end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipefd[1]);

        // Execute first command
        if (execvp(cmd1->arguments[0], cmd1->arguments) == -1) {
            perror("execvp");
            _exit(EXIT_FAILURE);
        }
    }

    // Second child (cmd2)
    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        return;
    }
    close(pipefd[1]);  // Close write end of parent

    if (pid2 == 0) {  // Second child
        // Redirect stdin from pipe read end
        close(pipefd[1]);  // Close write end
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(pipefd[0]);

        // Execute second command
        if (execvp(cmd2->arguments[0], cmd2->arguments) == -1) {
            perror("execvp");
            _exit(EXIT_FAILURE);
        }
    }

    // Parent process
    close(pipefd[0]);
    
    // After first fork in parent
    addProcess(&process_list, cmd1, pid1);
    
    // After second fork in parent
    addProcess(&process_list, cmd2, pid2);

    // Wait for both children
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void execute(cmdLine *pCmdLine) {
    // Check if command contains pipe
    if (pCmdLine->next != NULL) {
        executePiped(pCmdLine, pCmdLine->next);
        return;
    }

    // Handle built-in commands
    if (strcmp(pCmdLine->arguments[0], "stop") == 0) {
        if (pCmdLine->arguments[1]) {
            stopProcess(atoi(pCmdLine->arguments[1]));
        }
        return;
    } else if (strcmp(pCmdLine->arguments[0], "wake") == 0) {
        if (pCmdLine->arguments[1]) {
            wakeProcess(atoi(pCmdLine->arguments[1]));
        }
        return;
    } else if (strcmp(pCmdLine->arguments[0], "term") == 0) {
        if (pCmdLine->arguments[1]) {
            termProcess(atoi(pCmdLine->arguments[1]));
        }
        return;
    } else if (strcmp(pCmdLine->arguments[0], "quit") == 0) {
        exit(0);
    }
    if (strcmp(pCmdLine->arguments[0], "procs") == 0) {
        printProcessList(&process_list);
        return;
    }

    // Fork for external commands
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) { // Child process
        // Handle input redirection
        if (pCmdLine->inputRedirect) {
            int input_fd = open(pCmdLine->inputRedirect, O_RDONLY);
            if (input_fd == -1) {
                perror("Error opening input file");
                _exit(EXIT_FAILURE);
            }
            if (dup2(input_fd, STDIN_FILENO) == -1) {
                perror("Error redirecting input");
                close(input_fd);
                _exit(EXIT_FAILURE);
            }
            close(input_fd);
        }

        // Handle output redirection
        if (pCmdLine->outputRedirect) {
            int output_fd = open(pCmdLine->outputRedirect, 
                               O_WRONLY | O_CREAT | O_TRUNC,
                               0644);
            if (output_fd == -1) {
                perror("Error opening output file");
                _exit(EXIT_FAILURE);
            }
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                perror("Error redirecting output");
                close(output_fd);
                _exit(EXIT_FAILURE);
            }
            close(output_fd);
        }

        // Execute the command
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
            perror("execvp failed");
            _exit(EXIT_FAILURE);
        }
    } 
    else { // Parent process
        addProcess(&process_list, pCmdLine, pid);
        if (pCmdLine->blocking) {
            // Wait only if it's not a background process
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid failed");
            }
        }
    }
}

void init_history() {
    hist = (history_list*)malloc(sizeof(history_list));
    hist->head = NULL;
    hist->tail = NULL;
    hist->size = 0;
}

void add_to_history(const char* cmd) {
    if (!cmd || strcmp(cmd, "") == 0) return;
    
    // Create new node
    history_node* new_node = (history_node*)malloc(sizeof(history_node));
    new_node->cmd_str = strdup(cmd);
    new_node->next = NULL;
    
    // If list is empty
    if (hist->size == 0) {
        hist->head = new_node;
        hist->tail = new_node;
        hist->size = 1;
        return;
    }
    
    // Add to tail
    hist->tail->next = new_node;
    hist->tail = new_node;
    hist->size++;
    
    // Remove from head if over HISTLEN
    if (hist->size > HISTLEN) {
        history_node* temp = hist->head;
        hist->head = hist->head->next;
        free(temp->cmd_str);
        free(temp);
        hist->size--;
    }
}

void print_history() {
    history_node* current = hist->head;
    int index = 1;
    
    while (current != NULL) {
        printf("%d %s\n", index++, current->cmd_str);
        current = current->next;
    }
}

char* get_command_from_history(int n) {
    if (n < 1 || n > hist->size) {
        printf("Error: Invalid history index\n");
        return NULL;
    }
    
    history_node* current = hist->head;
    for (int i = 1; i < n; i++) {
        current = current->next;
    }
    
    return strdup(current->cmd_str);
}

void free_history() {
    history_node* current = hist->head;
    while (current != NULL) {
        history_node* next = current->next;
        free(current->cmd_str);
        free(current);
        current = next;
    }
    free(hist);
}

int main() {
    char cwd[PATH_MAX];
    char input[2048];
    cmdLine *parsedLine;

    init_history();

    while (1) {
        // Display the current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s> ", cwd);
        } else {
            perror("getcwd() error");
            return 1;
        }

        // Read a line from the user
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("fgets() error");
            continue;
        }

        // Remove newline character from input
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "history") == 0) {
            print_history();
            continue;
        }

        if (strcmp(input, "!!") == 0) {
            if (hist->size == 0) {
                printf("No commands in history\n");
                continue;
            }
            strcpy(input, hist->tail->cmd_str);
        } else if (input[0] == '!' && isdigit(input[1])) {
            int n = atoi(input + 1);
            char* cmd = get_command_from_history(n);
            if (cmd) {
                strcpy(input, cmd);
                free(cmd);
            } else {
                continue;
            }
        }

        // Parse the input
        parsedLine = parseCmdLines(input);
        if (parsedLine == NULL) {
            continue;
        }

        // Check for "quit" command
        if (strcmp(parsedLine->arguments[0], "quit") == 0) {
            freeCmdLines(parsedLine);
            break;
        }

        // Check for "cd" command
        if (strcmp(parsedLine->arguments[0], "cd") == 0) {
            if (parsedLine->argCount < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                if (chdir(parsedLine->arguments[1]) != 0) {
                    perror("chdir() error");
                }
            }
            freeCmdLines(parsedLine);
            continue;
        }

        // Execute the command
        execute(parsedLine);

        if (strcmp(input, "history") != 0) {
            add_to_history(input);
        }

        // Release resources
        freeCmdLines(parsedLine);
    }

    // Before return, cleanup process list
    freeProcessList(process_list);
    free_history();
    return 0;
}