#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>

void handle_sigcont(int sig) { raise(SIGSTOP); }

int main(int argc, char **argv) {
    FILE *file;
    char str[5];
    int count;
    
    if (argc == 2) // no-analyst option
        signal(SIGCONT, handle_sigcont);

    raise(SIGSTOP);

    sem_t *sem = sem_open("/sem_atend", O_RDWR);
        
    if (sem != SEM_FAILED)
        sem_wait(sem);
    else
        perror("analyst sem_failed");
   
    while (1) {
        
        if ((file = fopen("LNG.XXXXXX", "r")) == NULL) {
            perror("analyst - LNG.txt");
            exit(EXIT_FAILURE);
        }
        
        count = 0;
        while (fgets(str, 5, file) != NULL && count <= 10) {
            fputs(str, stdout);
            ++count;
        }
        
        fclose(file);

        remove("LNG.XXXXXX");

        if (sem != SEM_FAILED)
            sem_post(sem);

        raise(SIGSTOP);
    }
}

