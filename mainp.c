#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include "funkcje.h"

#define MAXAIRPLANES 10
#define MAXPASSENGERS 100

#define SIGNALFLY SIGUSR1
#define SIGNALKILL SIGUSR2

int createAndInitializeSemaphores(int semaphoreCount);

int createAndInitializeSemaphorePassenger(int semaphoreCount);

int createAndInitializeSemaphoreAirplane(int semaphoreCount);

int createAndInitializeSemaphoreEnd(int semaphoreCount);

void cleanup(int semID, int semSemforyPassengerID, int semSemforyAirplaneID, int SemaforyendID);

void *zbieraj_procesy(void *arg);

int *passenegerIsOver;
int passenegerIsOverID;

int main(void) {
    pthread_t thread;

    // Inicjalizacja pamięci dzielonej V, informacja o zamknięciu lotniska
    key_t kluczV = ftok(".", 'V');
    if (kluczV == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    passenegerIsOverID = shmget(kluczV, sizeof(int), IPC_CREAT | 0600);
    if (passenegerIsOverID == -1) {
        printf("Błąd pamięci dzielonej pasażerów\n");
        exit(1);
    }
    passenegerIsOver = (int *) shmat(passenegerIsOverID, NULL, 0);

    int semID = createAndInitializeSemaphores(4);
    int semSemforyAirplaneID = createAndInitializeSemaphoreAirplane(1);
    int semSemforyPassengerID = createAndInitializeSemaphorePassenger(1);
    int SemaforyendID = createAndInitializeSemaphoreEnd(1);


    printf("Main: tworzenie procesu lotnisko...\n");
    pid_t pid_lotnisko = fork();
    if (pid_lotnisko == 0) {
        execl("./lotnisko", "lotnisko", NULL);
        perror("Nie udało się uruchomić lotniska");
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
        if (passenegerIsOver[0] == 0) {
            pid_t pid_pasazer = fork();
            if (pid_pasazer == 0) {
                execl("./pasazer", "pasazer", NULL);
                perror("Nie udało się uruchomić pasażera");
                exit(EXIT_FAILURE);
            }
        } else {
            break;
        }
    }
    printf("Main: tworzenie procesu dyspozytora...\n");
    pid_t pid_dyspozytor = fork();
    if (pid_dyspozytor == 0) {
        execl("./dyspozytor", "dyspozytor", NULL);
        perror("Nie udało się uruchomić dyspozytora");
        exit(EXIT_FAILURE);
    }
    printf("Main: tworzenie wątku zbierającego procesy...\n");
    if (pthread_create(&thread, NULL, zbieraj_procesy, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    pthread_join(thread, NULL);

    safewaitsemafor(SemaforyendID, 0, 0);
    safewaitsemafor(SemaforyendID, 0, 0);
    printf("Main: wszystkie procesy zakończyły działanie. Czyszczenie zasobów...\n");
    cleanup(semID, semSemforyPassengerID, semSemforyAirplaneID, SemaforyendID);
    printf("Main: zasoby zostały wyczyszczone.\n");
    return 0;
}

// Funkcja wątku zbierającego procesy potomne
void *zbieraj_procesy(void *arg) {
    (void) arg; // Nieużywany argument
    while (1) {
        pid_t pid = wait(NULL);
        if (pid == -1) {
            if (errno == ECHILD) {
                break; // Nie ma więcej procesów potomnych
            } else {
                perror("wait");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

void cleanup(int semID, int semSemforyPassengerID, int semSemforyAirplaneID, int SemaforyendID) {
    zwolnijSemafor(semID, 0);
    zwolnijSemafor(semSemforyPassengerID, 0);
    zwolnijSemafor(semSemforyAirplaneID, 0);
    zwolnijSemafor(SemaforyendID, 0);

    shmdt(passenegerIsOver);
    shmctl(passenegerIsOverID, IPC_RMID, NULL);
    printf("Cleanup zakończony.\n");
}

// Pozostałe funkcje pomocnicze bez zmian
int createAndInitializeSemaphores(int semaphoreCount) {
    key_t key;

    if ((key = ftok(".", 'A')) == -1) {
        perror("Błąd ftok");
        _exit(2);
    }

    const int semID = alokujSemafor(key, semaphoreCount, IPC_CREAT | 0600);
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

    const int semSemforyPassengerID = alokujSemafor(keySemforyPassenger, 1, IPC_CREAT | 0600);
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

    const int semSemforyAirplaneID = alokujSemafor(keySemforyAirport, 1, IPC_CREAT | 0600);
    if (semSemforyAirplaneID == -1) {
        perror("alokujSemafor airplane");
        exit(EXIT_FAILURE);
    }
    inicjalizujSemafor(semSemforyAirplaneID, 0, 0);
    return semSemforyAirplaneID;
}

int createAndInitializeSemaphoreEnd(int semaphoreCount) {
    key_t keySemaforyend = ftok(".", 'T');
    if (keySemaforyend == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    const int SemaforyendID = alokujSemafor(keySemaforyend, 1, IPC_CREAT | 0600);
    if (SemaforyendID == -1) {
        perror("alokujSemafor airplane");
        exit(EXIT_FAILURE);
    }
    inicjalizujSemafor(SemaforyendID, 0, 0);
    return SemaforyendID;
}