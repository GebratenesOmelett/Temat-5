#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "funkcje.h"

#define MAXAIRPLANES 10
#define MAXPASSENGERS 100

#define SIGNALFLY SIGUSR1
#define SIGNALKILL SIGUSR2

int createAndInitializeSemaphores(int semaphoreCount);
int createAndInitializeSemaphorePassenger(int semaphoreCount);
int createAndInitializeSemaphoreAirplane(int semaphoreCount);
void cleanup(int semID, int semSemforyPassengerID, int semSemforyAirplaneID);
int *passenegerIsOver;
int passenegerIsOverID;

int main(void) {
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną V, informacja o zamknieciu lotniska
    key_t kluczV = ftok(".", 'V');
    if (kluczV == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    passenegerIsOverID = shmget(kluczV, sizeof(int), IPC_CREAT | 0666);
    if (passenegerIsOverID == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    passenegerIsOver = (int *) shmat(passenegerIsOverID, NULL, 0);

    int semID = createAndInitializeSemaphores(4);
    int semSemforyAirplaneID = createAndInitializeSemaphoreAirplane(1);
    int semSemforyPassengerID = createAndInitializeSemaphorePassenger(1);

    printf("Main: tworzenie procesu dyspozytora...\n");
    pid_t pid_dyspozytor = fork();
    if (pid_dyspozytor == 0) {
        execl("./dyspozytor", "dyspozytor", NULL);
        perror("Nie udało się uruchomić dyspozytor");
        exit(EXIT_FAILURE);
    }

    printf("Main: tworzenie procesu lotnisko...\n");
    pid_t pid_lotnisko = fork();
    if (pid_lotnisko == 0) {
        execl("./lotnisko", "lotnisko", NULL);
        perror("Nie udało się uruchomić lotnisko");
        exit(EXIT_FAILURE);
    }

    const int NUM_KSAMALOTU_PROCESSES = MAXAIRPLANES;
    for (int i = 0; i < NUM_KSAMALOTU_PROCESSES; i++) {
        pid_t pid_ksamolotu = fork();
        if (pid_ksamolotu == 0) {
            execl("./ksamolotu", "ksamolotu", NULL);
            perror("Nie udało się uruchomić kapitana samolotu");
            exit(EXIT_FAILURE);
        }
    }

    printf("Main: tworzenie procesów pasażerów...\n");
    signalSemafor(semID, 3); // Zwolnij semafor, aby ksamolotu mogli czytać do FIFO

    const int NUM_PASAZER_PROCESSES = MAXPASSENGERS;
    for (int i = 0; i < NUM_PASAZER_PROCESSES; i++) {
        if (passenegerIsOver[0] == 0){
            pid_t pid_pasazer = fork();
            if (pid_pasazer == 0) {
                execl("./pasazer", "pasazer", NULL);
                perror("Nie udało się uruchomić pasazer");
                exit(EXIT_FAILURE);
            }
            usleep((rand() % 1000000) + 100000);
        }
        else {
            break;
        }
    }

    for (int i = 0; i < 1 + 1 + NUM_PASAZER_PROCESSES + NUM_KSAMALOTU_PROCESSES; i++) {
        wait(NULL);
    }

    printf("Main: wszystkie procesy zakończyły działanie. Czyszczenie zasobów...\n");
    cleanup(semID, semSemforyPassengerID, semSemforyAirplaneID);
    printf("Main: zasoby zostały wyczyszczone.\n");
    return 0;
}

void cleanup(int semID, int semSemforyPassengerID, int semSemforyAirplaneID) {
    zwolnijSemafor(semID,0);
    zwolnijSemafor(semSemforyPassengerID,0);
    zwolnijSemafor(semSemforyAirplaneID,0);

    shmdt(passenegerIsOver);
    shmctl(passenegerIsOverID, IPC_RMID, NULL);
    printf("Cleanup zakończony.\n");
}

int createAndInitializeSemaphores(int semaphoreCount) {
    key_t key;

    if ((key = ftok(".", 'A')) == -1) {
        perror("Błąd ftok");
        _exit(2);
    }

    const int semID = alokujSemafor(key, semaphoreCount, IPC_CREAT | 0666);
    for (int i = 0; i < semaphoreCount; i++) {
        inicjalizujSemafor(semID, i, 0);
    }

    printf("Semafory gotowe!\n");
    return semID;
}

int createAndInitializeSemaphorePassenger(int semaphoreCount) {
    key_t keySemforyPassenger = ftok(".", 'R');
    if (keySemforyPassenger == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    const int semSemforyPassengerID = alokujSemafor(keySemforyPassenger, 1, IPC_CREAT | 0666);
    if (semSemforyPassengerID == -1) {
        perror("alokujSemafor passenger");
        exit(EXIT_FAILURE);
    }
    inicjalizujSemafor(semSemforyPassengerID, 0, 0);
    return semSemforyPassengerID;
}

int createAndInitializeSemaphoreAirplane(int semaphoreCount) {
    key_t keySemforyAirport = ftok(".", 'K');
    if (keySemforyAirport == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    const int semSemforyAirplaneID = alokujSemafor(keySemforyAirport, 1, IPC_CREAT | 0666);
    if (semSemforyAirplaneID == -1) {
        perror("alokujSemafor airplane");
        exit(EXIT_FAILURE);
    }
    inicjalizujSemafor(semSemforyAirplaneID, 0, 0);
    return semSemforyAirplaneID;
}
