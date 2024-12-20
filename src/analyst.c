#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include "../include/utils.h"

void handle_sigcont(int sig) {
    remove("LNG");
    raise(SIGSTOP);
}

int main(int argc, char **argv) {
    FILE *file;
        
    if (argc == 2) // no-analyst option
        signal(SIGCONT, handle_sigcont);

    raise(SIGSTOP);

    sem_t *sem = sem_open("/sem_atend", O_RDWR);
        
    if (sem != SEM_FAILED)
        sem_wait(sem);
    else
        perror("analyst sem_failed");
   
    while (1) {
        
        if ((file = fopen("LNG", "rb")) == NULL) {
            perror("analyst - LNG");
            exit(EXIT_FAILURE);
        }
        
        int itens = 0;
        pid_t pids[N_WAKE_ANALYST];
        
        if ((itens = fread(pids, sizeof(pid_t), N_WAKE_ANALYST, file)) < 0) {
            exit(EXIT_FAILURE);
        }
        
        fclose(file);
        for (int i = 0; i < itens; ++i) {
            printf("%d\n", pids[i]);
        }

        remove("LNG");

        if (sem != SEM_FAILED)
            sem_post(sem);

        raise(SIGSTOP);
    }
}

