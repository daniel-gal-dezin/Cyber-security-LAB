#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid1, pid2;

    // Create pipe
    if(pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>forking…)\n");
    pid1 = fork();
    if(pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "(parent_process>created process with id: %d)\n", pid1);

    if(pid1 == 0) {
        // Child process 1 (ls -l)
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
        
        if(dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipefd[0]);  // Close read end
        close(pipefd[1]);  // Close write end after dup2
        
        char *argv[] = {"ls", "-l", NULL};
        fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
    close(pipefd[1]);  // Close write end in parent

    pid2 = fork();
    if(pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(pid2 == 0) {
        // Child process 2 (tail -n 2)
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe...)\n");
        
        if(dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);  // Close read end after dup2
        
        char *argv[] = {"tail", "-n", "2", NULL};
        fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");
        
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
    close(pipefd[0]);  // Close read end in parent

    fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    fprintf(stderr, "(parent_process>exiting…)\n");
    return 0;
}