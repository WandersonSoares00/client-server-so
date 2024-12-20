#include "../include/utils.h"
#include "../include/service.h"
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>

void dealoc_client(void *client) { free(client); }

int ordered_insert(Client *client, Darray *clients_arr) {
    if (darray_size(clients_arr) == clients_arr->curr_size) {
        if (darray_resize (clients_arr))    return 1;
    }

    int i = clients_arr->last_data - 1;

    while (i >= 0) {
        Client *curr = clients_arr->data[i];
        if (curr->priority < client->priority) {
            clients_arr->data[i + 1] = clients_arr->data[i];
        }
        --i;
    }

    clients_arr->data[i + 1] = client;
    ++clients_arr->last_data;
    return 0;
}

void* input_thread(void* arg) {
    ReceptionArgs *r_args = (ReceptionArgs*)arg;

    while (1) {
        char input;
        input = getchar();

        if (input == 's' || input == 'S') {
            r_args->stop = 1;
            break;
        }

        while (getchar() != '\n');
    }

    return NULL;
}

pid_t invoke_analyst(int lazy) {
    pid_t pid = fork();
    if (pid == 0) {
        if (lazy) {
            char *argv[] = {"./analyst", "", NULL};
            execv(argv[0], argv);
        } else {
            char *argv[] = {"./analyst", NULL};
            execv(argv[0], argv);
        }
        perror("analyst");
        exit(EXIT_FAILURE);
    }
    else {
        if (waitpid(pid, NULL, WUNTRACED) == -1) {
            exit(EXIT_FAILURE);
        }
        return pid;
    }
}

void print_resource_statistics() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    double user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double system_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    
    printf("Tempo executando em user mode: %.6f s\n", user_time);
    printf("Tempo executando em kernel mode: %.6f s\n", system_time);
    printf("Block output operations: %ld times\n", usage.ru_oublock);
    printf("Page faults: %ld times\n", usage.ru_majflt);
    printf("Voluntary context switches: %ld times\n", usage.ru_nvcsw);
    printf("Involuntary context switches: %ld times\n", usage.ru_nivcsw);
    /*
    struct rusage usage_children;
    getrusage(RUSAGE_CHILDREN, &usage_children);

    double children_user_time = usage_children.ru_utime.tv_sec + usage_children.ru_utime.tv_usec / 1e6;
    double children_system_time = usage_children.ru_stime.tv_sec + usage_children.ru_stime.tv_usec / 1e6;

    printf("Uso agregado dos processos filhos:\n");
    printf("Tempo executando em user mode: %.6f s\n", children_user_time);
    printf("Tempo executando em kernel mode: %.6f s\n", children_user_time);
    printf("Block output operations: %ld times\n", usage_children.ru_oublock);
    */
}

void print_help() {
    fprintf(stderr, "Erro: As opções -n e -x são obrigatórias.\n");
}

void parse_arguments(int argc, char *argv[], struct options *opts) {
    int opt;
    int option_index = 0;

    opts->num_clients = -1;
    opts->max_wait_time = -1;
    opts->no_analyst = 0;

    struct option long_options[] = {
        {"n", required_argument, 0, 'n'},
        {"x", required_argument, 0, 'x'},
        {"no-analyst", no_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "n:x:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'n':
                opts->num_clients = atoi(optarg);
                break;
            case 'x':
                opts->max_wait_time = atoi(optarg);
                break;
            case 0:
                if (strcmp(long_options[option_index].name, "no-analyst") == 0) {
                    opts->no_analyst = 1;
                }
                break;
            case '?':
            default:
                print_help();
                exit(EXIT_FAILURE);
        }
    }

    if (opts->num_clients == -1 || opts->max_wait_time == -1) {
        print_help();
        exit(EXIT_FAILURE);
    }
}
