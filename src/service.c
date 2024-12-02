#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../include/client.h"
#include "../include/darray.h"
#include "../include/utils.h"

void servicer(Darray *clients_array, pid_t analyst_pid) {
  int cont = 0;

  while (1) {
    // Esperar os semáfores sem_atend e sem_block
    sem_t *sem1 = sem_open("/sem_atend", O_RDWR);

    sem_t *sem2 = sem_open("/sem_block", O_RDWR);

    if (sem1 != SEM_FAILED) sem_wait(sem1);

    if (sem2 != SEM_FAILED) sem_wait(sem2);

    Client *client = (Client *)darray_get_front(clients_array);
    if (client == NULL) {
      sem_post(sem2);
      continue;
    }

    darray_pop_front(clients_array);
    ++cont;
    sem_post(sem2);

    time_t current_time = time(NULL);
    double waiting_time = difftime(current_time, client->t_coming);

    // Retorna a satistafação do cliente
    const char *satisfaction =
        (waiting_time <= client->priority) ? "Satisfied" : "Unsatisfied";
    printf("%s\n", satisfaction);

    // Acordar cliente
    kill(client->pid, SIGCONT);

    // Escrever PID no LNG
    FILE *file = fopen("LNG.txt", "a");
    if (file != NULL) {
      fprintf(file, "Client: %d\n", client->pid);
      fclose(file);
    } else {
      perror("Error to open file");
      exit(EXIT_FAILURE);
    }

    // Acordar Analista
    if (cont == 10) {
      kill(analyst_pid, SIGCONT);
      cont = 0;
    }
  }
}

void reception(int n_clients, Darray *queue) {
  sem_t *sem1, *sem2;
  if ((sem1 = sem_open("/sem_atend", O_CREAT)) == SEM_FAILED) {
    perror("sem_open failure");
    exit(EXIT_FAILURE);
  }
  if ((sem2 = sem_open("/sem_block", O_CREAT)) == SEM_FAILED) {
    perror("sem_open failure");
    exit(EXIT_FAILURE);
  }

  if (n_clients == 0) {
  }

  for (int i = 0; i < n_clients; ++i) {
    pid_t client_pid = fork();
    if (client_pid == 0) {
      execv("./client.c", "./client.c");
      perror("client");
      exit(EXIT_FAILURE);
    } else {
      Client *client = malloc(sizeof(Client));
      client->pid = client_pid;
      client->t_coming = time(NULL);

      int demanda_fd = open("demanda.txt", O_RDONLY);
      if (demanda_fd == -1) {
        perror("client");
        exit(EXIT_FAILURE);
      }

      int time;
      if (read(demanda_fd, &time, sizeof(time)) != sizeof(time)) {
        perror("client");
        close(demanda_fd);
        exit(EXIT_FAILURE);
      }

      close(demanda_fd);
      remove("demanda.txt");
      client->t_service = time;
      darray_push_back(queue, client);
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 3) exit(EXIT_FAILURE);

  int n_clients = atoi(argv[1]);
  int time = atoi(argv[2]);
  Darray *clients_queue = darray_init(n_clients, NULL);

  darray_free(clients_queue);
  return EXIT_SUCCESS;
}
