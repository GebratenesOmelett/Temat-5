#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "funkcje.h"
#include <malloc.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#define FIFO_NAME "passenger_fifo"

// Funkcja, którą będą wykonywać wątki
void* createAndSendPassenger(void* arg) {
    struct thread_data* data = (struct thread_data*)arg; // Rzucenie wskaźnika na odpowiedni typ
    int id = data->id;
    int fd = data->fd;
    struct passenger* passenger = (struct passenger*)malloc(sizeof(struct passenger));
    passenger->id = id;
    passenger->baggage_weight = randNumber(100);
    passenger->gender = randGender();
    passenger->is_vip = randVip();
    passenger->frustration = randNumber(20);
    passenger->peoplePass = 0;
//    sleep(passenger->id % 10000); // Symulacja pracy wątku
    printf("Wątek %d rozpoczął pracę!\n", passenger->id);
    print_passenger(passenger);
    if (write(fd, passenger, sizeof(struct passenger)) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    free(passenger);
    return NULL;
}

int main() {
    int liczba_watkow = 10000; // Liczba wątków do utworzenia
    pthread_t watki[liczba_watkow];
    printf("git");
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("mkfifo");
    }
    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    printf("git");
    int i = 0;

    while(1) {
        struct thread_data* data = (struct thread_data*)malloc(sizeof(struct thread_data));
        data->id = i + 1;  // ID wątku
        data->fd = fd;
        if (pthread_create(&watki[i], NULL, createAndSendPassenger, data) != 0) {
            perror("Błąd przy tworzeniu wątku");
            return 1;
        }
        sleep(randNumber(5));
        i++;
    }
    for (int j = 0; j < liczba_watkow; j++) {
        pthread_join(watki[j], NULL);
    }

    close(fd);
    printf("Proces pasazer zakończył pracę.\n");
    return 0;
}