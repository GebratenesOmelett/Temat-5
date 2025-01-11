#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "funkcje.h"
#include <malloc.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/shm.h>

#define FIFO_NAME "passengerFifo"
#define MAXAIRPLANES 10

static void *createAndSendPassenger(void *arg);

int fifoSend;
int N = 4;
int semID, msgID, shmID;

int main() {
    key_t kluczA, kluczB, kluczC;
    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }

    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);
    waitSemafor(semID, 1, 0); //brak jeżeli nie ma mainp
    int liczba_watkow = 10000; // Liczba wątków do utworzenia
    pthread_t watki[liczba_watkow];

    createNewFifo(FIFO_NAME);

    if ((kluczB = ftok(".", 'C')) == -1) {
        printf("Blad ftok (C)\n");
        exit(2);
    }
    msgctl(msgID, IPC_RMID, NULL);
    msgID= msgget(kluczB, IPC_CREAT  | 0666);
    if(msgID == -1){
        printf("blad kolejki komunikatow pasazerow\n");
        exit(1);
    }

    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }
    shmID = shmget(kluczC, MAXAIRPLANES * sizeof(int), IPC_CREAT|IPC_EXCL|06666);
    if(shmID == -1){
        printf("blad pamieci dzielodznej pasazerow\n");
        exit(1);
    }
    signalSemafor(semID, 0);
    fifoSend = open(FIFO_NAME, O_WRONLY);
    if (fifoSend == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    int id = 0;
    while (1) {
        id = i + 1; // Przypisanie wartości ID
        if (pthread_create(&watki[i], NULL, createAndSendPassenger, &id) != 0) {
            perror("Błąd przy tworzeniu wątku");
            return 1;
        }
        sleep(randNumber(2));
        i++;
    }
    for (int j = 0; j < liczba_watkow; j++) {
        pthread_join(watki[j], NULL);
    }

    close(fifoSend);
    printf("Proces pasazer zakończył pracę.\n");
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
//    sleep(passenger->id % 10000); // Symulacja pracy wątku
//    printf("Wątek %d rozpoczął pracę!\n", passenger->id);
//    print_passenger(passenger);

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
        if (message.mvalue == 1){
            break;
        } else{
            passenger->baggage_weight = randNumber(100);
        }
        sleep(randNumber(5));
    }
    printf("pasazer %d  czeka\n", passenger->id);
    free(passenger);
    return NULL;
}



