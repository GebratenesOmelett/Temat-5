#include <stdio.h>
#include <pthread.h>
#include "funkcje.h"
#include <sys/shm.h>
#define MAXAIRPLANES 10

int numberOfPlanes;
int semID, msgID, shmID;
int *memory;
int N = 4;

void *airplaneControl(void *arg) {
    int i = *(int *)arg;
    memory[i] = rand() % 100 + 1;

    printf("samolot %d z wagą %d\n", i, memory[i]);
    free(arg);
    return NULL;
}

void createFIFOs(int numberOfPlanes) {
    char fifoName[256];
    for (int i = 0; i < numberOfPlanes; i++) {
        snprintf(fifoName, sizeof(fifoName), "fifoplane%d", i); // Tworzymy unikalną nazwę FIFO
        createNewFifo(fifoName);
        printf("FIFO %s created successfully.\n", fifoName);
    }
}

int main() {
    key_t kluczA, kluczB, kluczC;

    numberOfPlanes = randNumber(MAXAIRPLANES);

    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }
    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);
    waitSemafor(semID, 2, 0); //brak jeżeli nie ma mainp

    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }
    shmID = shmget(kluczC, (MAXAIRPLANES+1) * sizeof(int), IPC_CREAT|0666);
    if(shmID == -1){
        printf("blad pamieci dzielodznej samolotu\n");
        exit(1);
    }
    memory = (int*)shmat(shmID, NULL, 0);
    memory[MAXAIRPLANES] = numberOfPlanes;
    printf("ilosc samolotow to salomoty %d\n", numberOfPlanes);
    signalSemafor(semID, 3);

    createFIFOs(numberOfPlanes);

    if (numberOfPlanes <= 0) {
        fprintf(stderr, "Invalid number of planes: %d\n", numberOfPlanes);
        return 1;
    }

    pthread_t watki[numberOfPlanes];
    for (int i = 0; i < numberOfPlanes; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            return 1;
        }
        *arg = i;
        pthread_create(&watki[i], NULL, airplaneControl, arg);
    }

    for (int j = 0; j < numberOfPlanes; j++) {
        pthread_join(watki[j], NULL);
    }

    return 0;
}
