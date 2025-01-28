#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "funkcje.h"
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/shm.h>

#define SIGNALFLY SIGUSR1
#define SIGNALKILL SIGUSR2
#define MAXPASSENGERS 500
#define MAXAIRPLANES 10

int airplaneIsOverID, semSemforyAirplaneID, semID, shmID, msgSamolotID, shmIDAmountOfPeople, shmPeopleInID,
        shmPidAirplaneID, airplanesInAirportID;
int *memory, *memoryAmountPeople, *memoryPeopleIn, *PidAirplane, *airplanesInAirport, *airplanesPids, *airplaneIsOver;
int count = 0, peopleInPid[MAXPASSENGERS];
volatile atomic_int startFly = 0;

static void cleanup();
//Odbieranie sygnału leć
void handleSignalFly(int sig) {
    printf("Odebrano sygnał %d Wykonuję akcję startu.\033[0;32m\n", sig);
    fflush(stdout);
    atomic_store(&startFly, 1); // Bezpieczna aktualizacja flagi startu
}
//Odbieranie sygnału zakończ
void handleSignalKill(int sig) {
    fflush(stdout);
    signalSemafor(semSemforyAirplaneID, 0);
    signalSemafor(semID, 3);
    if (count == 0) {
        printf("Samolot [%d] kończy loty\033[0;32m\n", getpid());
        atomic_fetch_sub(&airplanesInAirport[0], 1); // Zmniejszenie liczby samolotów na lotnisku
        if (airplanesInAirport[0] == 0) {
            cleanup();
        }
        exit(0);
    }
}

int main() {
    //############################## Konfiguracja obsługi sygnałów
    struct sigaction saFly, saKill;

    saFly.sa_handler = handleSignalFly;
    saFly.sa_flags = 0;
    sigemptyset(&saFly.sa_mask);
    if (sigaction(SIGNALFLY, &saFly, NULL) == -1) {
        perror("Błąd konfiguracji SIGNALFLY");
        return 1;
    }

    saKill.sa_handler = handleSignalKill;
    saKill.sa_flags = 0;
    sigemptyset(&saKill.sa_mask);
    if (sigaction(SIGNALKILL, &saKill, NULL) == -1) {
        perror("Błąd konfiguracji SIGNALKILL");
        _exit(1);
    }

    int md = randNumber(100, getpid());
    int amountOfPassengers = randNumber(100, getpid());

    key_t kluczA = ftok(".", 'A');
    if (kluczA == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja semaforów
    const int N = 4;
    semID = alokujSemafor(kluczA, N, 0);
    if (semID == -1) {
        perror("Błąd alokacji semafora");
        exit(EXIT_FAILURE);
    }
    // Pamięć dzielona dla wagi samolotu
    key_t kluczC = ftok(".", 'C');
    if (kluczC == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }
    shmID = shmget(kluczC, sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        printf("Błąd pamięci dzielonej samolotu\n");
        exit(1);
    }
    memory = (int *) shmat(shmID, NULL, 0);
    if (memory[0] == 0 || memory[0] < 0) {
        memory[0] = md;
    }
    // Kolejka komunikatów do komunikacji z pasażerami
    key_t kluczH = ftok(".", 'H');
    if (kluczH == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }
    msgSamolotID = msgget(kluczH, IPC_CREAT | 0666);
    if (msgSamolotID == -1) {
        perror("Błąd tworzenia kolejki komunikatów");
        exit(EXIT_FAILURE);
    }
    // Pamięć dzielona dla maksymalnej liczby pasażerów
    key_t kluczD = ftok(".", 'D');
    if (kluczD == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }
    shmIDAmountOfPeople = shmget(kluczD, sizeof(int), IPC_CREAT | 0666);
    if (shmIDAmountOfPeople == -1) {
        printf("Błąd pamięci dzielonej samolotu\n");
        exit(1);
    }
    memoryAmountPeople = (int *) shmat(shmIDAmountOfPeople, NULL, 0);
    if (memoryAmountPeople[0] == 0 || memoryAmountPeople[0] < 0) {
        memoryAmountPeople[0] = amountOfPassengers;
    }
    // Pamięć dzielona dla aktualnej liczby pasażerów na pokładzie
    key_t kluczE = ftok(".", 'E');
    if (kluczE == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }
    shmPeopleInID = shmget(kluczE, sizeof(int), IPC_CREAT | 0666);
    if (shmPeopleInID == -1) {
        printf("Błąd pamięci dzielonej pasażerów\n");
        exit(1);
    }
    memoryPeopleIn = (int *) shmat(shmPeopleInID, NULL, 0);
    if (memoryPeopleIn == (void *) -1) {
        perror("Błąd podłączenia pamięci");
        exit(1);
    }
    // Inicjalizacja pamięci dzielonej dla PID-ów samolotów
    key_t kluczP = ftok(".", 'P');
    if (kluczP == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }
    shmPidAirplaneID = shmget(kluczP, sizeof(int), IPC_CREAT | 0666);
    if (shmPidAirplaneID == -1) {
        printf("Błąd pamięci dzielonej PID-ów samolotów\n");
        exit(1);
    }
    PidAirplane = (int *) shmat(shmPidAirplaneID, NULL, 0);
    if (PidAirplane[0] == 0 || PidAirplane[0] < 0) {
        PidAirplane[0] = getpid();
    }
    // Inicjalizacja pamięci dzielonej dla liczby samolotów na lotnisku
    key_t kluczN = ftok(".", 'N');
    if (kluczN == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }
    airplanesInAirportID = shmget(kluczN, sizeof(int), IPC_CREAT | 0666);
    if (airplanesInAirportID == -1) {
        printf("Błąd pamięci dzielonej dla samolotów\n");
        printf("Samolot kończy %d loty", getpid());
        exit(1);
    }
    airplanesInAirport = (int *) shmat(airplanesInAirportID, NULL, 0);
    // Pamięć dzielona dla PID-ów aktywnych samolotów
    key_t kluczU = ftok(".", 'U');
    if (kluczU == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }
    int airplanesPidsID = shmget(kluczU, MAXAIRPLANES * sizeof(int), IPC_CREAT | 0666);
    if (airplanesPidsID == -1) {
        printf("Błąd pamięci dzielonej PID-ów\n");
        exit(1);
    }
    airplanesPids = (int *) shmat(airplanesPidsID, NULL, 0);
    // Inicjalizacja semaforów dla synchronizacji samolotów
    key_t keySemforyAirport = ftok(".", 'K');
    if (keySemforyAirport == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }

    semSemforyAirplaneID = alokujSemafor(keySemforyAirport, 1, 0);
    if (semSemforyAirplaneID == -1) {
        perror("Błąd alokacji semafora samolotu");
        exit(EXIT_FAILURE);
    }
    // Pamięć dzielona dla flagi zakończenia lotów
    key_t kluczY = ftok(".", 'Y');
    if (kluczY == -1) {
        perror("Błąd generacji klucza");
        exit(EXIT_FAILURE);
    }
    airplaneIsOverID = shmget(kluczY, sizeof(int), IPC_CREAT | 0666);
    if (airplaneIsOverID == -1) {
        printf("Błąd pamięci dzielonej flagi\n");
        exit(1);
    }
    airplaneIsOver = (int *) shmat(airplaneIsOverID, NULL, 0);

    safewaitsemafor(semSemforyAirplaneID, 0, 0);
    airplanesPids[airplanesInAirport[0]] = getpid();
    atomic_fetch_add(&airplanesInAirport[0], 1);
    signalSemafor(semID, 0);
    signalSemafor(semID, 0);
    signalSemafor(semSemforyAirplaneID, 0);


    struct passenger p;
    struct messagePassengerAirport msgFromPassenger;
    while (airplaneIsOver[0] == 0) {
        if (airplaneIsOver[0] == 0) {
            safewaitsemafor(semID, 3, 0);
        }

        memory[0] = md;
        PidAirplane[0] = getpid();
        printf("Samolot [%d] z wagą %d o pojemności %d wjeżdża na pas lotniska\033[0;32m\n", getpid(), memory[0],
               memoryAmountPeople[0]);
        while (airplaneIsOver[0] == 0) {
            if ((memoryPeopleIn[0] < memoryAmountPeople[0] && (rand() % 100 + 1) < 99) && startFly == 0) {
                signalSemafor(semID, 2);
                ssize_t ret = msgrcv(msgSamolotID, &msgFromPassenger, sizeof(struct passenger), 2, 0);


                if (ret == -1) {
                    if (errno == EINTR) {
                        goto fly;
                    } else {
                        perror("Błąd odbioru komunikatu");
                        exit(EXIT_FAILURE);
                    }
                }
                peopleInPid[count] = msgFromPassenger.passenger.id;
                ++count;
                printf("Samolot: wszedł pasażer o PID [%d]\033[0;32m\n", msgFromPassenger.passenger.id);
                continue;
            }
            fly:
            if (startFly == 1) {
                atomic_store(&startFly, 0); // Reset flagi startu
            }
            printf("Samolot [%d] startuje, posiada %d osób na maksymalną ilość %d\033[0;32m\n", getpid(),memoryPeopleIn[0],memoryAmountPeople[0]);
            fflush(stdout);
            memoryPeopleIn[0] = 0;
            signalSemafor(semID, 3);
            sleep((rand() % 20) + 10);
            printf("Samolot [%d] wrócił\033[0;32m\n", getpid());
            for (int i = 0; i < count; i++) {
                printf("Samolot [%d] przewiózł pasażera [%d]\033[0;32m\n",getpid(), peopleInPid[i]);
            }
            fflush(stdout);
            count = 0;
            memset(peopleInPid, 0, sizeof(peopleInPid));
            break;
        }
    }
    atomic_fetch_sub(&airplanesInAirport[0], 1);
    if (airplanesInAirport[0] == 0) {
        cleanup();
    }
    printf("Samolot [%d] kończy loty\033[0;32m\n", getpid());
    return 0;
}

void cleanup() {
    // Odłączanie pamięci dzielonych
    if (memory != NULL) {
        shmdt(memory);
        memory = NULL;
    }
    if (memoryAmountPeople != NULL) {
        shmdt(memoryAmountPeople);
        memoryAmountPeople = NULL;
    }
    if (memoryPeopleIn != NULL) {
        shmdt(memoryPeopleIn);
        memoryPeopleIn = NULL;
    }

    if (airplanesPids != NULL) {
        shmdt(airplanesPids);
        airplanesPids = NULL;
    }

    // Usuwanie kolejki komunikatów
    if (msgSamolotID != -1) {
        msgctl(msgSamolotID, IPC_RMID, NULL);
        msgSamolotID = -1;
    }

    // Usuwanie pamięci dzielonych
    if (shmID != -1) {
        shmctl(shmID, IPC_RMID, NULL);
        shmID = -1;
    }
    if (shmIDAmountOfPeople != -1) {
        shmctl(shmIDAmountOfPeople, IPC_RMID, NULL);
        shmIDAmountOfPeople = -1;
    }
    if (shmPeopleInID != -1) {
        shmctl(shmPeopleInID, IPC_RMID, NULL);
        shmPeopleInID = -1;
    }
    if (shmPidAirplaneID != -1) {
        shmctl(shmPidAirplaneID, IPC_RMID, NULL);
        shmPidAirplaneID = -1;
    }
    if (airplanesInAirportID != -1) {
        shmctl(airplanesInAirportID, IPC_RMID, NULL);
        airplanesInAirportID = -1;
    }
    if (airplaneIsOverID != -1) {
        shmctl(airplaneIsOverID, IPC_RMID, NULL);
        airplaneIsOverID = -1;
    }

}