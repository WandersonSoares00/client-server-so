#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>

int main() {
    FILE *file;
    char str[5];
    int count;
    
    while (1) {
        raise(SIGSTOP);

        sem_t *sem = sem_open("/sem_atend", O_RDWR);
        
        if (sem != SEM_FAILED)
            sem_wait(sem);
        
        if ((file = fopen("LNG.txt", "r")) == NULL) {
            perror("analyst - LNG.txt");
            exit(EXIT_FAILURE);
        }
        
        count = 0;
        while (fgets(str, 5, file) != NULL && count <= 10) {
            fputs(str, stdout);
            ++count;
        }
        
        fclose(file);

        remove("LNG.txt");

        if (sem != SEM_FAILED)
            sem_post(sem);
    }
}

