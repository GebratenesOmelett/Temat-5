#include "funkcje.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXAIRPLANES 10

int semID, msgIDdyspozytor, shmID, shmAmountofPeople, shmIOPassenger;
int numberOfPlanes;
int *memory, *memoryAmountPeople, *IOPassenger;
int N = 4;


// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Funkcja wątku
void *controller(void *arg) {
    int i = *(int *)arg;
    printf("utworzone wątek controller %d\n", i);
    free(arg); // Pamięć już niepotrzebna

    while (1) {
        usleep(rand() % 10000000 + 2000000);
        printf("zmieniam %d na otwarte-------------------", i);

        pthread_mutex_lock(&mutex);

        IOPassenger[i] = 1;
        usleep(10000000);
        IOPassenger[i] = 0;
        pthread_mutex_unlock(&mutex);
        printf("zmieniam %d na zamykanie-------------------", i);

    }
    return NULL;
}

int main() {
    key_t kluczA, kluczD, kluczC, kluczF, kluczG;

    //---------------------------------------------------- Inicjalizacja klucza A
    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }

    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);


    //---------------------------------------------------- Inicjalizacja kolejke wiadomości E
    if ((kluczD = ftok(".", 'E')) == -1) {
        printf("Blad ftok (E)\n");
        exit(2);
    }
    msgIDdyspozytor = msgget(kluczD, IPC_CREAT | 0666);
    if (msgIDdyspozytor == -1) {
        printf("Blad kolejki komunikatow pasazerow\n");
        exit(1);
    }

    //---------------------------------------------------- Inicjalizacja pamięć dzieloną D
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
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną F
    if ((kluczF = ftok(".", 'F')) == -1) {
        printf("Blad ftok (F)\n");
        exit(2);
    }

    shmAmountofPeople = shmget(kluczF, (MAXAIRPLANES) * sizeof(int), IPC_CREAT | 0666);
    if (shmAmountofPeople == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    memoryAmountPeople = (int *)shmat(shmAmountofPeople, NULL, 0);
    if (memoryAmountPeople == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną G
    if ((kluczG = ftok(".", 'G')) == -1) {
        printf("Blad ftok (G)\n");
        exit(2);
    }

    shmIOPassenger = shmget(kluczG, (MAXAIRPLANES) * sizeof(int), IPC_CREAT | 0666);
    if (shmIOPassenger == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    IOPassenger = (int *)shmat(shmIOPassenger, NULL, 0);
    if (IOPassenger == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    //----------------------------------------------------
    waitSemafor(semID, 3, 0);
    waitSemafor(semID, 0, 0);
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

//     Zniszczenie mutexu przed zakończeniem programu
    pthread_mutex_destroy(&mutex);

    return 0;
}
