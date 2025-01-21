#include "funkcje.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <malloc.h>

#define FIFO_NAME "passengerFifo"
#define MAXAIRPLANES 10
#define PREFIX "fifoplane"
#define PASSENGERSAMOUNT 10000
#define START 1
#define STOP 0


#define SG1 SIGUSR1
#define SG2 SIGUSR2


static void *createAndSendPassenger(void *arg);

static void cleanupResources();

int fifoSend;
int N = 4;
int semID, msgID, shmID, shmAmountofPeople, shmIOPassenger, shmPeopleInID;
int *memory, *tableOfFlights, *memoryAmountPeople, *IOPassenger, *memoryPeopleIn;
int numberOfPlanes, liczba_watkow, startStop;
pthread_t watki[PASSENGERSAMOUNT];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void fifoSendAirplane(int numberOfPlanes);

void handleSignalKill(int sig) {
    printf("Odebrano sygnał %d (SIGUSR2): Zatrzymuję program i czyszczę zasoby pasazer.\n", sig);
    startStop = 0; // Ustawienie flagi zatrzymania
    cleanupResources(); // Sprzątanie zasobów
}


int main() {
    //############################## Obsługa sygnału
    struct sigaction sa;
    sa.sa_handler = handleSignalKill;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("[pasazer] Error setting signal handler");
        return 1;
    }

    printf("[pasazer] PID: %d - Waiting for SIGUSR2...\n", getpid());

//###############################
    key_t kluczA, kluczB, kluczC, kluczF, kluczG, kluczL;
    liczba_watkow = PASSENGERSAMOUNT; // Liczba wątków do utworzenia

    startStop = START;
//-------------------------------------------------------------------------- klucz semaforów A
    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }

    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);
    waitSemafor(semID, 1, 0); //brak jeżeli nie ma mainp


    createNewFifo(FIFO_NAME);
//-------------------------------------------------------------------------- kolejka wiadomości C
    if ((kluczB = ftok(".", 'C')) == -1) {
        printf("Blad ftok (C)\n");
        exit(2);
    }
    msgctl(msgID, IPC_RMID, NULL);
    msgID = msgget(kluczB, IPC_CREAT | 0666);
    if (msgID == -1) {
        printf("blad kolejki komunikatow pasazerow\n");
        exit(1);
    }

//-------------------------------------------------------------------------- kolejka wiadomości E

//-------------------------------------------------------------------------- pamięć dzielona D
    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }
    shmID = shmget(kluczC, (MAXAIRPLANES + 1) * sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        printf("blad pamieci dzielodznej pasazerow\n");
        exit(1);
    }
    memory = (int *) shmat(shmID, NULL, 0);
    if (memory == (void *) -1) {
        perror("shmat");
        exit(1);
    }
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
    IOPassenger = (int *) shmat(shmIOPassenger, NULL, 0);
    if (IOPassenger == (void *) -1) {
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
    //----------------------------------------------------
    signalSemafor(semID, 0);
    waitSemafor(semID, 3, 0);
    signalSemafor(semID, 0);


    printf("ilosc samolotow pasazer %d\n", memory[MAXAIRPLANES]);

    numberOfPlanes = memory[MAXAIRPLANES];

    tableOfFlights = malloc(numberOfPlanes * sizeof(int));

    signalSemafor(semID, 2);
    fifoSendAirplane(numberOfPlanes);

    fifoSend = open(FIFO_NAME, O_WRONLY);
    if (fifoSend == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    waitSemafor(semID, 2, 0);
    printf("jest git------------------------------------------- pasazerow:");

    for (int i = 0; i < liczba_watkow; i++) {
        if (!startStop) {
            break;
        }
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&watki[i], NULL, createAndSendPassenger, id) != 0) {
            perror("Błąd przy tworzeniu wątku");
            free(id);
            return 1;
        }
        sleep(randNumber(2));
    }
    for (int j = 0; j < liczba_watkow; j++) {
        pthread_join(watki[j], NULL);
    }

    printf("kończe");
    close(fifoSend);
    free(tableOfFlights);
    pthread_mutex_destroy(&mutex);
    return 0;
}

// Funkcja, którą będą wykonywać wątki
void *createAndSendPassenger(void *arg) {
    int id = *((int *) arg);
    struct passenger *passenger = (struct passenger *) malloc(sizeof(struct passenger));
    struct messagePassenger message;


    passenger->id = id;
    passenger->baggage_weight = randNumber(100);
    passenger->gender = randGender();
    passenger->is_vip = randRare();
    passenger->is_equipped = randRare();
    passenger->frustration = randNumber(20);
    passenger->peoplePass = 0;
    passenger->airplaneNumber = getAirplane(numberOfPlanes);
    free(arg);
    sleep(randNumber(3));
    while (1) {

        if (write(fifoSend, passenger, sizeof(struct passenger)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        if (msgrcv(msgID, &message, sizeof(message.mvalue), passenger->id, 0) == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        if (message.mvalue == 1) {
            break;
        } else if (message.mvalue == 2) {
            free(passenger);
            return NULL;
        } else {
            passenger->baggage_weight = randNumber(passenger->baggage_weight);
        }
        usleep(1000);
    }
    printf("pasazer %d  czeka\n", passenger->id);

    printf("table : %d", tableOfFlights[passenger->airplaneNumber]);

    while (1) {
        if (passenger->is_vip == 0) {
            printf("pasazer %d  czeka------------------------------------------\n", passenger->id);
            if (IOPassenger[passenger->airplaneNumber] == 0) {
                sleep(3);
                continue;
            }
        }
        if (memoryPeopleIn[passenger->airplaneNumber] < memoryAmountPeople[passenger->airplaneNumber]) {
            memoryPeopleIn[passenger->airplaneNumber]++;
            if (write(tableOfFlights[passenger->airplaneNumber], passenger, sizeof(struct passenger)) == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            break;
        } else {
            printf("samolot odlatuje");
            continue;
        }


    }
    printf("zapisane----------------------------------------------------------\n");
    free(passenger);
    return NULL;
}

void fifoSendAirplane(int numberOfPlanes) {
    char fifoName[20];
    for (int i = 0; i < numberOfPlanes; i++) {
        snprintf(fifoName, sizeof(fifoName), "%s%d", PREFIX, i); // Tworzymy unikalną nazwę FIFO

        tableOfFlights[i] = open(fifoName, O_WRONLY);

        if (tableOfFlights[i] == -1) {
            perror("open : ");
            exit(EXIT_FAILURE);
        }


    }

    return;
}

void cleanupResources() {
    printf("czyszcze pasazera");
    if (fifoSend != -1) {
        close(fifoSend);
    }

    for (int i = 0; i < numberOfPlanes; i++) {
        if (tableOfFlights[i] != -1) {
            close(tableOfFlights[i]);
        }
    }

    // Zwolnienie pamięci
    if (tableOfFlights != NULL) {
        free(tableOfFlights);
    }

    // Odłączenie pamięci dzielonej
    if (memory != (void *) -1) {
        shmdt(memory);
    }
    if (memoryAmountPeople != (void *) -1) {
        shmdt(memoryAmountPeople);
    }
    if (IOPassenger != (void *) -1) {
        shmdt(IOPassenger);
    }
    if (memoryPeopleIn != (void *) -1) {
        shmdt(memoryPeopleIn);
    }

    // Usunięcie pamięci dzielonej
    shmctl(shmID, IPC_RMID, NULL);
    shmctl(shmAmountofPeople, IPC_RMID, NULL);
    shmctl(shmIOPassenger, IPC_RMID, NULL);
    shmctl(shmPeopleInID, IPC_RMID, NULL);

    // Usunięcie kolejki wiadomości
    msgctl(msgID, IPC_RMID, NULL);

    // Usunięcie semaforów
    zwolnijSemafor(semID, 0);

    // Zniszczenie mutexa
    pthread_mutex_destroy(&mutex);

    printf("Zasoby programu zostały wyczyszczone.\n");
}


