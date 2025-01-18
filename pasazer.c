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

static void *createAndSendPassenger(void *arg);

int fifoSend;
int N = 4;
int semID, msgID, shmID, msgIDdyspozytor, shmAmountofPeople, shmIOPassenger, shmPeopleInID;
int *memory, *tableOfFlights, *memoryAmountPeople, *tableOfPeople, *IOPassenger, *memoryPeopleIn;
int numberOfPlanes;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void fifoSendAirplane(int numberOfPlanes);

int main() {
    key_t kluczA, kluczB, kluczC, kluczD, kluczF, kluczG, kluczL;

//-------------------------------------------------------------------------- klucz semaforów A
    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }

    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);
    waitSemafor(semID, 1, 0); //brak jeżeli nie ma mainp
    int liczba_watkow = 10000; // Liczba wątków do utworzenia
    pthread_t watki[liczba_watkow];

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
    if ((kluczD = ftok(".", 'E')) == -1) {
        printf("Blad ftok (E)\n");
        exit(2);
    }
    msgctl(msgIDdyspozytor, IPC_RMID, NULL);
    msgIDdyspozytor = msgget(kluczD, IPC_CREAT | 0666);
    if (msgIDdyspozytor == -1) {
        printf("blad kolejki komunikatow pasazerow\n");
        exit(1);
    }
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
    IOPassenger = (int *)shmat(shmIOPassenger, NULL, 0);
    if (IOPassenger == (void *)-1) {
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
    memoryPeopleIn = (int *)shmat(shmPeopleInID, NULL, 0);
    if (memoryPeopleIn == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    //----------------------------------------------------
    signalSemafor(semID, 0);
    signalSemafor(semID, 0);
    waitSemafor(semID, 3, 0);

    printf("ilosc samolotow pasazer %d\n", memory[MAXAIRPLANES]);

    numberOfPlanes = memory[MAXAIRPLANES];

    tableOfFlights = malloc(numberOfPlanes * sizeof(int));

    signalSemafor(semID, 0);
    fifoSendAirplane(numberOfPlanes);

    fifoSend = open(FIFO_NAME, O_WRONLY);
    if (fifoSend == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    printf("jest git-------------------------------------------");

    waitSemafor(semID, 2, 0);
    printf("jest git-------------------------------------------");

    for (int i = 0; i < liczba_watkow; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&watki[i], NULL, createAndSendPassenger, id) != 0) {
            perror("Błąd przy tworzeniu wątku");
            free(id);
            return 1;
        }
        sleep(randNumber(1));
    }
    for (int j = 0; j < liczba_watkow; j++) {
        pthread_join(watki[j], NULL);
    }

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
    free(arg);

    passenger->id = id;
    passenger->baggage_weight = randNumber(100);
    passenger->gender = randGender();
    passenger->is_vip = randRare();
    passenger->is_equipped = randRare();
    passenger->frustration = randNumber(20);
    passenger->peoplePass = 0;
    passenger->airplaneNumber = getAirplane(numberOfPlanes);
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
//            pthread_mutex_unlock(&mutex);
            break;
        } else {
            passenger->baggage_weight = randNumber(passenger->baggage_weight);
        }
//        pthread_mutex_unlock(&mutex);

        sleep(100);
    }
    printf("pasazer %d  czeka\n", passenger->id);

//    printf("table : %d", tableOfFlights[passenger->airplaneNumber]);

    while (1) {
        for (int i = 0; i < MAXAIRPLANES; i++) {
            printf("%d, ", tableOfPeople[i]);
        }
        if(passenger->is_vip == 0){
            usleep(10000);
            if (IOPassenger[passenger->airplaneNumber] == 0){
                continue;
            }
        }
        if (pthread_mutex_lock(&mutex) != 0) {
            perror("Mutex lock failed");
            exit(EXIT_FAILURE);
        }
        if (memoryPeopleIn[passenger->airplaneNumber] < memoryAmountPeople[passenger->airplaneNumber]) {
            memoryPeopleIn[passenger->airplaneNumber]++;
            if (write(tableOfFlights[passenger->airplaneNumber], passenger, sizeof(struct passenger)) == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            pthread_mutex_unlock(&mutex);
            break;
        } else {
            pthread_mutex_unlock(&mutex);
            printf("samolot odlatuje");
            continue;
        }
        pthread_mutex_unlock(&mutex);
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

