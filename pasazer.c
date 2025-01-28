#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdatomic.h>
#include "funkcje.h"
#include <setjmp.h>

#define SIGNALFLY SIGUSR1
#define SIGNALKILL SIGUSR2
#define MAXPASSENGERS 100

int *memoryPeopleIn, *memoryAmountPeople, *IOPassenger, *allPassengersPid, *passenegerIsOver;
int semID, msgID, msgLotniskoID, msgSamolotID, shmIDAmountOfPeople, shmPeopleInID, shmIOPassenger,
        semSemforyPassengerID, PassenegerInAirportID, allPassengersPidID, passenegerIsOverID;
volatile int cont = 1;
int *PassenegerInAirport;

static void cleanup();

void handleSignalKill(int sig) {
    printf("[PID %d] Wychodzi z lotniska\033[0;31m\n", getpid());
    cont = 0; // Bezpieczne zaktualizowanie flagi
    atomic_fetch_sub(&PassenegerInAirport[0], 1); // Zmniejszenie liczby pasażerów
    signalSemafor(semID, 1);
    signalSemafor(semID, 2);
    signalSemafor(semSemforyPassengerID, 0);
    cleanup();
    exit(0);
}

int main(void) {
    // Konfiguracja handlera sygnału
    struct sigaction saKill;
    saKill.sa_handler = handleSignalKill;
    saKill.sa_flags = 0;
    sigemptyset(&saKill.sa_mask);
    if (sigaction(SIGNALKILL, &saKill, NULL) == -1) {
        perror("Błąd konfiguracji SIGKILL");
        _exit(1);
    }

    // Inicjalizacja struktury pasażera
    struct passenger passenger;
    struct messagePassenger message;

    passenger.id = getpid();
    passenger.baggage_weight = randNumber(100, passenger.id);
    passenger.gender = randGender(passenger.id);
    passenger.is_vip = randRare(passenger.id);
    passenger.is_equipped = randRare(passenger.id);
    passenger.frustration = randNumber(20, passenger.id);
    passenger.peoplePass = 0;

    // Pobieranie klucza dla semaforów
    key_t key = ftok(".", 'A');
    if (key == -1) {
        perror("Błąd ftok dla klucza 'A'");
        exit(EXIT_FAILURE);
    }

    // Łączenie z semaforami
    semID = alokujSemafor(key, 4, 0);
    if (semID == -1) {
        printf("[PID %d] Wychodzi z lotniska\033[0;31m\n", getpid());
        cleanup();
    }

    // Semafor dla koordynacji pasażerów
    key_t keySemforyPassenger = ftok(".", 'R');
    if (keySemforyPassenger == -1) {
        perror("Błąd ftok dla klucza 'R'");
        exit(EXIT_FAILURE);
    }

    semSemforyPassengerID = alokujSemafor(keySemforyPassenger, 1, 0);
    if (semSemforyPassengerID == -1) {
        printf("[PID %d] Wychodzi z lotniska\n", getpid());
        return 0;
    }

    // Kolejka komunikatów - potwierdzenia
    key_t kluczB = ftok(".", 'B');
    if (kluczB == -1) {
        perror("Błąd ftok dla klucza 'B'");
        exit(EXIT_FAILURE);
    }
    msgID = msgget(kluczB, IPC_CREAT | 0666);
    if (msgID == -1) {
        perror("Błąd tworzenia kolejki komunikatów");
        exit(EXIT_FAILURE);
    }

    // Kolejka komunikatów - lotnisko
    key_t kluczF = ftok(".", 'F');
    if (kluczF == -1) {
        perror("Błąd ftok dla klucza 'F'");
        exit(EXIT_FAILURE);
    }
    msgLotniskoID = msgget(kluczF, IPC_CREAT | 0666);
    if (msgLotniskoID == -1) {
        perror("Błąd tworzenia kolejki lotniska");
        exit(EXIT_FAILURE);
    }

    // Kolejka komunikatów - samolot
    key_t kluczH = ftok(".", 'H');
    if (kluczH == -1) {
        perror("Błąd ftok dla klucza 'H'");
        exit(EXIT_FAILURE);
    }
    msgSamolotID = msgget(kluczH, IPC_CREAT | 0666);
    if (msgSamolotID == -1) {
        perror("Błąd tworzenia kolejki samolotu");
        exit(EXIT_FAILURE);
    }

    // Pamięć współdzielona - limit pasażerów
    key_t kluczD = ftok(".", 'D');
    if (kluczD == -1) {
        perror("Błąd ftok dla klucza 'D'");
        exit(EXIT_FAILURE);
    }
    shmIDAmountOfPeople = shmget(kluczD, sizeof(int), IPC_CREAT | 0666);
    if (shmIDAmountOfPeople == -1) {
        perror("Błąd tworzenia pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }
    memoryAmountPeople = (int *) shmat(shmIDAmountOfPeople, NULL, 0);

    // Pamięć współdzielona - pasażerowie w samolocie
    key_t kluczE = ftok(".", 'E');
    if (kluczE == -1) {
        perror("Błąd ftok dla klucza 'E'");
        exit(EXIT_FAILURE);
    }
    shmPeopleInID = shmget(kluczE, sizeof(int), IPC_CREAT | 0666);
    if (shmPeopleInID == -1) {
        perror("Błąd tworzenia pamięci pasażerów");
        exit(EXIT_FAILURE);
    }
    memoryPeopleIn = (int *) shmat(shmPeopleInID, NULL, 0);

    // Pamięć współdzielona - kontrola bramki
    key_t kluczG = ftok(".", 'G');
    if (kluczG == -1) {
        perror("Błąd ftok dla klucza 'G'");
        exit(EXIT_FAILURE);
    }
    shmIOPassenger = shmget(kluczG, sizeof(int), IPC_CREAT | 0666);
    if (shmIOPassenger == -1) {
        perror("Błąd tworzenia pamięci bramki");
        exit(EXIT_FAILURE);
    }
    IOPassenger = (int *) shmat(shmIOPassenger, NULL, 0);

    // Pamięć współdzielona - liczba pasażerów
    key_t kluczO = ftok(".", 'O');
    if (kluczO == -1) {
        perror("Błąd ftok dla klucza 'O'");
        exit(EXIT_FAILURE);
    }
    PassenegerInAirportID = shmget(kluczO, 2 * sizeof(int), IPC_CREAT | 0666);
    if (PassenegerInAirportID == -1) {
        perror("Błąd tworzenia pamięci licznika");
        exit(EXIT_FAILURE);
    }
    PassenegerInAirport = (int *) shmat(PassenegerInAirportID, NULL, 0);

    // Pamięć współdzielona - PIDy pasażerów
    key_t kluczX = ftok(".", 'X');
    if (kluczX == -1) {
        perror("Błąd ftok dla klucza 'X'");
        exit(EXIT_FAILURE);
    }
    allPassengersPidID = shmget(kluczX, MAXPASSENGERS * sizeof(int), IPC_CREAT | 0666);
    if (allPassengersPidID == -1) {
        perror("Błąd tworzenia pamięci PIDów");
        exit(EXIT_FAILURE);
    }
    allPassengersPid = (int *) shmat(allPassengersPidID, NULL, 0);

    // Pamięć współdzielona - flaga zamknięcia
    key_t kluczV = ftok(".", 'V');
    if (kluczV == -1) {
        perror("Błąd ftok dla klucza 'V'");
        exit(EXIT_FAILURE);
    }
    passenegerIsOverID = shmget(kluczV, sizeof(int), IPC_CREAT | 0666);
    if (passenegerIsOverID == -1) {
        perror("Błąd tworzenia pamięci flagi");
        exit(EXIT_FAILURE);
    }
    passenegerIsOver = (int *) shmat(passenegerIsOverID, NULL, 0);

    safewaitsemafor(semSemforyPassengerID, 0, 0);
    struct messagePassengerAirport messagepassengerairport;
    struct messagePassengerAirport messagepassengerairplane;
    struct messagePassenger messFromAirplane;

    // Rejestracja pasażera w systemie
    allPassengersPid[PassenegerInAirport[1]] = passenger.id;
    atomic_fetch_add(&PassenegerInAirport[0], 1);
    atomic_fetch_add(&PassenegerInAirport[1], 1);
    signalSemafor(semSemforyPassengerID, 0);

    // Główna pętla przetwarzania
    while (passenegerIsOver[0] == 0) {
        safewaitsemafor(semID, 1, 0);
        if (passenegerIsOver[0] == 1) break;

        printf("[PID %d] Przechodzi kontrolę bezpieczeństwa\033[0;31m\n", passenger.id);
        messagepassengerairport.mtype = 1;
        messagepassengerairport.passenger = passenger;

        if (msgsnd(msgLotniskoID, &messagepassengerairport, sizeof(struct passenger), 0) == -1) {
            perror("Błąd wysyłania do lotniska");
            exit(EXIT_FAILURE);
        }

        ssize_t ret = msgrcv(msgID, &message, sizeof(message.mvalue), passenger.id, 0);
        if (ret == -1) {
            if (errno == EINTR) continue;
            perror("Błąd odbierania potwierdzenia");
            exit(EXIT_FAILURE);
        }

        if (message.mvalue == 1) goto WejscieDoSamolotu;
        if (message.mvalue == 2) {
            printf("[PID %d] Wykryto niebezpieczny przedmiot!\033[0;31m\n", passenger.id);
            atomic_fetch_sub(&PassenegerInAirport[0], 1);
            return 0;
        }

        passenger.baggage_weight = randNumber(passenger.baggage_weight, (rand() % 100) + 1);
        continue;

        WejscieDoSamolotu:
        printf("[PID %d] Oczekuje na wejście (%d/%d miejsc)\033[0;31m\n",
               passenger.id, memoryPeopleIn[0], memoryAmountPeople[0]);

        while (passenegerIsOver[0] == 0) {
            safewaitsemafor(semID, 2, 0);
            if (passenegerIsOver[0] == 1) break;

            messagepassengerairplane.mtype = 2;
            messagepassengerairplane.passenger = passenger;

            if (passenger.is_vip == 0 && IOPassenger[0] == 0) {
                signalSemafor(semID, 2);
                continue;
            }

            if (memoryPeopleIn[0] < memoryAmountPeople[0]) {
                memoryPeopleIn[0]++;
                if (msgsnd(msgSamolotID, &messagepassengerairplane, sizeof(struct passenger), 0) == -1) {
                    perror("Błąd wysyłania do samolotu");
                    exit(EXIT_FAILURE);
                }
                printf("Pasażer wszedł do samolotu\033[0;31m\n");
                atomic_fetch_sub(&PassenegerInAirport[0], 1);
                return 0;
            }

            printf("[PID %d] Brak miejsc, oczekuję...\033[0;31m\n", passenger.id);
        }
    }
    atomic_fetch_sub(&PassenegerInAirport[0], 1);
    printf("[PID %d] Wychodzi z lotniska\033[0;31m\n", getpid());
    return 0;
}

void cleanup() {
    // Zwolnienie zasobów
    if (memoryAmountPeople) shmdt(memoryAmountPeople);
    if (memoryPeopleIn) shmdt(memoryPeopleIn);
    if (IOPassenger) shmdt(IOPassenger);
    if (PassenegerInAirport) shmdt(PassenegerInAirport);
    if (allPassengersPid) shmdt(allPassengersPid);
    if (passenegerIsOver) shmdt(passenegerIsOver);

    shmctl(shmIDAmountOfPeople, IPC_RMID, NULL);
    shmctl(shmPeopleInID, IPC_RMID, NULL);
    shmctl(shmIOPassenger, IPC_RMID, NULL);
    shmctl(PassenegerInAirportID, IPC_RMID, NULL);
    shmctl(allPassengersPidID, IPC_RMID, NULL);
    shmctl(passenegerIsOverID, IPC_RMID, NULL);

    semctl(semID, 0, IPC_RMID);
    semctl(semSemforyPassengerID, 0, IPC_RMID);

    msgctl(msgID, IPC_RMID, NULL);
    msgctl(msgLotniskoID, IPC_RMID, NULL);
    msgctl(msgSamolotID, IPC_RMID, NULL);
    exit(0);
}