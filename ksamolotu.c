#include "funkcje.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/shm.h>
#include <unistd.h>
#include <malloc.h>

#define MAXAIRPLANES 10
#define PREFIX "fifoplane"

int numberOfPlanes, maxWeight, maxPeople, multiplyAirplane;
int semID, msgID, shmID, shmAmountofPeople, shmPeopleInID, semInAirplane;
int N = 4;
int *tableOfFlights, *memoryAmountPeople, *memory, *tableOfPeople, *memoryPeopleIn;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Define the mutex
// Mutex do synchronizacji zapisu


static void createFIFOs(int numberOfPlanes);

static void readFromFifo(int numberOfPlanes);

void *airplaneControl(void *arg) {
    int i = *(int *) arg;
    free(arg);
    printf("samolot %d o pojemności %d\n", i, memoryAmountPeople[i]);
    waitSemafor(semInAirplane, i, 0);
    maxWeight = rand() % 100 + 1;
    memory[i] = maxWeight;
    printf("samolot %d z wagą %d o pojemności %d\n", i, memory[i], memoryAmountPeople[i]);


//    pthread_mutex_lock(&mutex); // Critical section start

    while (1) {
        struct passenger p;

        if (memoryPeopleIn[i] < memoryAmountPeople[i] || (rand() % 100 + 1) == 1) {
            tableOfPeople[i]++;
            ssize_t bytesRead = read(tableOfFlights[i], &p, sizeof(struct passenger));
            if (bytesRead == -1) {
                perror("Error reading from FIFO");
                exit(EXIT_FAILURE);
            }

            printf("------------------------------------------------------ samolot : %d", i);
            print_passenger(&p);
        } else {
            printf("samolot %d jest pełny osoby %d\n", i, memoryPeopleIn[i]);
            printf("samolot %d startuje", i);
            memoryPeopleIn[i] = 0;
            signalSemafor(semInAirplane, i);
            usleep(100000);
        }
//        pthread_mutex_unlock(&mutex); // Critical section end

    }


    return NULL;
}

int main() {
    key_t kluczA, kluczC, kluczF, kluczL, kluczM;
    numberOfPlanes = randNumber(MAXAIRPLANES);
    tableOfFlights = malloc(numberOfPlanes * sizeof(int));
    multiplyAirplane = randNumber(5);
//#------------------------------------------------ Alokacja semaforów A
    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }
    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);
    waitSemafor(semID, 0, 0); //brak jeżeli nie ma mainp
//#------------------------------------------------ Alokacja semaforów M
    if ((kluczM = ftok(".", 'M')) == -1) {
        printf("Blad ftok (M)\n");
        exit(2);
    }
    semInAirplane = alokujSemafor(kluczM, MAXAIRPLANES, IPC_CREAT | 0666);
    for (int i = 0; i < MAXAIRPLANES; i++)
        inicjalizujSemafor(semInAirplane, i, 0);
//#------------------------------------------------ Alokacja pamięci dzielonej D
    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }
    shmID = shmget(kluczC, (MAXAIRPLANES + 1) * sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        printf("blad pamieci dzielodznej samolotu\n");
        exit(1);
    }
    memory = (int *) shmat(shmID, NULL, 0);
    memory[MAXAIRPLANES] = numberOfPlanes;
//#------------------------------------------------ Alokacja pamięci dzielonej F
    if ((kluczF = ftok(".", 'F')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }

    shmAmountofPeople = shmget(kluczF, (MAXAIRPLANES) * sizeof(int), IPC_CREAT | 0666);
    if (shmAmountofPeople == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    memoryAmountPeople = (int *) shmat(shmAmountofPeople, NULL, 0);
    if (memoryAmountPeople == (void *) -1) {
        perror("shmat");
        exit(1);
    }
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną L
    if ((kluczL = ftok(".", 'L')) == -1) {
        printf("Blad ftok (L)\n");
        exit(2);
    }

    shmPeopleInID = shmget(kluczL, (MAXAIRPLANES) * sizeof(int), IPC_CREAT | 0666);
    if (shmPeopleInID == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    memoryPeopleIn = (int *) shmat(shmPeopleInID, NULL, 0);
    if (memoryPeopleIn == (void *) -1) {
        perror("shmat");
        exit(1);
    }
//#--------------------------------------------------------

    createFIFOs(numberOfPlanes);
    tableOfPeople = malloc(MAXAIRPLANES * sizeof(int));
    if (numberOfPlanes <= 0) {
        fprintf(stderr, "Invalid number of planes: %d\n", numberOfPlanes);
        return 1;
    }

    printf("ilosc samolotow to salomoty %d\n", numberOfPlanes);
    signalSemafor(semID, 3);
    signalSemafor(semID, 3);
    printf("odblokowa\n");

    readFromFifo(numberOfPlanes);
    for (int i = 0; i < MAXAIRPLANES; i++) {
        printf("%d, ", tableOfFlights[i]);
    }
    pthread_t watki[numberOfPlanes * multiplyAirplane];
//    for (int i = 0; i < numberOfPlanes * multiplyAirplane; i++) {
//        int *arg = malloc(sizeof(int));
//        if (!arg) {
//            perror("malloc");
//            return 1;
//        }
//        *arg = i % numberOfPlanes;
//        pthread_create(&watki[i], NULL, airplaneControl, arg);
//    }
    for (int j = 0; j < MAXAIRPLANES; j++) {
        printf("%d,", memoryPeopleIn[j]);
    }
    printf("\n");
    for (int j = 0; j < MAXAIRPLANES; j++) {
        printf("%d,", memoryAmountPeople[j]);
    }
    for (int i = 0; i < numberOfPlanes; i++) {
        maxPeople = rand() % 100 + 20;
        memoryAmountPeople[i] = maxPeople;
        signalSemafor(semInAirplane, i);
    }
    signalSemafor(semID, 2);
    signalSemafor(semID, 2);

    for (int j = 0; j < numberOfPlanes * multiplyAirplane; j++) {
        if (pthread_join(watki[j], NULL) != 0) {
            perror("pthread_join");
        }
    }

//    printf("oki doki skończone");
    // Zniszczenie mutexu przed zakończeniem programu
    pthread_mutex_destroy(&mutex);

    free(tableOfFlights);
    shmdt(memory);
    shmdt(memoryAmountPeople);
    return 0;
}

void createFIFOs(int numberOfPlanes) {
    char fifoName[20];
    for (int i = 0; i < numberOfPlanes; i++) {
        snprintf(fifoName, sizeof(fifoName), "%s%d", PREFIX, i); // Tworzymy unikalną nazwę FIFO
        createNewFifo(fifoName);
        printf("FIFO %s created successfully.\n", fifoName);
    }
}

void readFromFifo(int numberOfPlanes) {
    char fifoName[20];
    for (int i = 0; i < numberOfPlanes; i++) {
        snprintf(fifoName, sizeof(fifoName), "%s%d", PREFIX, i); // Tworzymy unikalną nazwę FIFO
        printf("daje do odczytu\n");
        tableOfFlights[i] = open(fifoName, O_RDONLY);
        printf("otworzono %d\n", tableOfFlights[i]);
        if (tableOfFlights[i] == -1) {
            perror("open\n------------");
            exit(EXIT_FAILURE);
        }
    }
}
