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

#define SIGNALFLY SIGUSR1
#define SIGNALKILL SIGUSR2

int numberOfPlanes, maxPeople, multiplyAirplane;
int semID, msgID, shmID, shmAmountofPeople, shmPeopleInID, semInAirplane;
int N = 4;
int *tableOfFlights, *memoryAmountPeople, *memory, *tableOfPeople, *memoryPeopleIn;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Define the mutex
// Mutex do synchronizacji zapisu


static void createFIFOs(int numberOfPlanes);
static void readFromFifo(int numberOfPlanes);
static void *airplaneControl(void *arg);
static void cleanupResources();

void handleSignalFly(int sig){
    printf("Odebrano sygnał %d (SIGUSR1): Wykonuję akcję A.\n", sig);
}
void handleSignalKill(int sig) {
    printf("Odebrano sygnał %d (SIGUSR2): Zatrzymuję program i czyszczę zasoby.\n", sig);
    cleanupResources(); // Sprzątanie zasobów
}

void *airplaneControl(void *arg) {
    int i = *((int *) arg);
    free(arg);
    printf("samolot %d o pojemności %d\n", i, memoryAmountPeople[i]);
    waitSemafor(semInAirplane, i, 0);

    printf("samolot %d z wagą %d o pojemności %d\n", i, memory[i], memoryAmountPeople[i]);

    printf("table of flight : %d\n", tableOfFlights[i]);
    while (1) {
        struct passenger p;
//        pthread_mutex_lock(&mutex); // Critical section start

        if (memoryPeopleIn[i] < memoryAmountPeople[i] || (rand() % 100 + 1) == 1) {
            ssize_t bytesRead = read(tableOfFlights[i], &p, sizeof(struct passenger));
            tableOfPeople[i]++;
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
            printf("samolot %d wrócił", i);

        }
//        pthread_mutex_unlock(&mutex); // Critical section end

    }


    return NULL;
}

int main() {
//############################## Obsługa sygnału
    struct sigaction saFly, saKill;

    saFly.sa_handler = handleSignalFly;
    saFly.sa_flags = 0;
    sigemptyset(&saFly.sa_mask);
    if(sigaction(SIGNALFLY, &saFly, NULL) == -1){
        perror("Błąd SIGNALFLY");
        return 1;
    }

    saKill.sa_handler = handleSignalKill;
    saKill.sa_flags = 0;
    sigemptyset(&saKill.sa_mask);
    if(sigaction(SIGNALKILL, &saKill, NULL) == -1){
        perror("Błąd SIGNALKILL");
        return 1;
    }

//###############################

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
    for (int i = 0; i < MAXAIRPLANES; i++) {
        inicjalizujSemafor(semInAirplane, i, 0);
    }
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
        printf("Blad ftok (F)\n");
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
    for (int i = 0; i < numberOfPlanes; i++) {
        printf("%d, ", tableOfFlights[i]);
    }
    pthread_t watki[numberOfPlanes];
    for (int i = 0; i < numberOfPlanes; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            return 1;
        }
        *arg = (i);
        pthread_create(&watki[i], NULL, airplaneControl, arg);
    }
    for (int j = 0; j < numberOfPlanes; j++) {
        printf("%d,", memoryPeopleIn[j]);
    }
    printf("\n");

    for (int i = 0; i < numberOfPlanes; i++) {
        memory[i] = rand() % 100 + 1;
        memoryAmountPeople[i] = rand() % 100 + 20;
        signalSemafor(semInAirplane, i);
    }
    for (int j = 0; j < numberOfPlanes; j++) {
        printf("%d,", memoryAmountPeople[j]);
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
void cleanupResources() {
    printf("Czyszczenie zasobów...\n");

    // Odłączenie pamięci dzielonej
    if (memory != (void *) -1) {
        shmdt(memory);
    }
    if (memoryAmountPeople != (void *) -1) {
        shmdt(memoryAmountPeople);
    }
    if (memoryPeopleIn != (void *) -1) {
        shmdt(memoryPeopleIn);
    }

    // Usunięcie pamięci dzielonej
    if (shmID != -1) {
        if (shmctl(shmID, IPC_RMID, NULL) == -1) {
            perror("Błąd przy usuwaniu pamięci dzielonej (shmID)");
        }
    }
    if (shmAmountofPeople != -1) {
        if (shmctl(shmAmountofPeople, IPC_RMID, NULL) == -1) {
            perror("Błąd przy usuwaniu pamięci dzielonej (shmAmountofPeople)");
        }
    }
    if (shmPeopleInID != -1) {
        if (shmctl(shmPeopleInID, IPC_RMID, NULL) == -1) {
            perror("Błąd przy usuwaniu pamięci dzielonej (shmPeopleInID)");
        }
    }

    // Usunięcie semaforów
    if (semID != -1) {
        if (semctl(semID, 0, IPC_RMID) == -1) {
            perror("Błąd przy usuwaniu semaforów (semID)");
        }
    }
    if (semInAirplane != -1) {
        if (semctl(semInAirplane, 0, IPC_RMID) == -1) {
            perror("Błąd przy usuwaniu semaforów (semInAirplane)");
        }
    }

    // Zamykanie i usuwanie FIFO
    if (tableOfFlights) {
        char fifoName[20];
        for (int i = 0; i < numberOfPlanes; i++) {
            if (tableOfFlights[i] != -1) {
                close(tableOfFlights[i]);
            }
            snprintf(fifoName, sizeof(fifoName), "%s%d", PREFIX, i);
            if (unlink(fifoName) == -1) {
                perror("Błąd przy usuwaniu FIFO");
            } else {
                printf("FIFO %s zostało usunięte.\n", fifoName);
            }
        }
        free(tableOfFlights); // Zwolnienie pamięci
    }

    // Zniszczenie mutexu
    if (pthread_mutex_destroy(&mutex) != 0) {
        perror("Błąd przy niszczeniu mutexu");
    }

    printf("Zasoby zostały wyczyszczone.\n");
}

