#include "funkcje.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXAIRPLANES 10

int semID, msgIDdyspozytor, shmID;
int numberOfPlanes;
int *memory;
int N = 4;


// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Funkcja wątku
void *controller(void *arg) {
    int i = *(int *)arg;
    struct depoPassenger message;
    free(arg); // Pamięć już niepotrzebna

    message.mtype = 1; // Typ komunikatu
    message.data = i;  // Dane do wysłania

    while (1) {
        // Zablokowanie mutexu przed wysłaniem wiadomości
        pthread_mutex_lock(&mutex);

        // Wysłanie komunikatu
        if (msgsnd(msgIDdyspozytor, &message, sizeof(message.data), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }

        // Odblokowanie mutexu po wysłaniu wiadomości
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    key_t kluczA, kluczD, kluczC;

    // Inicjalizacja klucza A
    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }

    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);

    // Inicjalizacja klucza D
    if ((kluczD = ftok(".", 'E')) == -1) {
        printf("Blad ftok (E)\n");
        exit(2);
    }

    // Tworzenie kolejki komunikatów
    msgctl(msgIDdyspozytor, IPC_RMID, NULL);
    msgIDdyspozytor = msgget(kluczD, IPC_CREAT | 0666);
    if (msgIDdyspozytor == -1) {
        printf("Blad kolejki komunikatow pasazerow\n");
        exit(1);
    }

    // Inicjalizacja klucza C
    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }

    shmID = shmget(kluczC, (MAXAIRPLANES+1) * sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }

    memory = (int *)shmat(shmID, NULL, 0);
    if (memory == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    waitSemafor(semID, 3, 0);

    printf("Ilosc samolotow dyspozytor %d\n", memory[MAXAIRPLANES]);
    numberOfPlanes = memory[MAXAIRPLANES];

    pthread_t watki[numberOfPlanes];

    // Tworzenie wątków
    for (int i = 0; i < numberOfPlanes; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            return 1;
        }
        *arg = i;
        pthread_create(&watki[i], NULL, controller, arg);
    }

    // Czekanie na zakończenie wątków
    for (int j = 0; j < numberOfPlanes; j++) {
        pthread_join(watki[j], NULL);
    }

    // Zniszczenie mutexu przed zakończeniem programu
    pthread_mutex_destroy(&mutex);

    return 0;
}
