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
    cont = 0; // Safely update the value of startFly
    atomic_fetch_sub(&PassenegerInAirport[0], 1); // Zmniejszenie liczby pasażerów
    signalSemafor(semID, 1);
    signalSemafor(semID, 2);
    signalSemafor(semSemforyPassengerID, 0);
    cleanup();
    exit(0);
}


int main(void) {
    //#######################################
    struct sigaction saKill;

    saKill.sa_handler = handleSignalKill;
    saKill.sa_flags = 0;
    sigemptyset(&saKill.sa_mask);
    if (sigaction(SIGNALKILL, &saKill, NULL) == -1) {
        perror("Błąd SIGINT");
        _exit(1);
    }
    //#######################################
    // Przygotowanie struktury pasażera i wypełnianie przykładowymi danymi
    struct passenger passenger;
    struct messagePassenger message;

    passenger.id = getpid();
    passenger.baggage_weight = randNumber(100, passenger.id);
    passenger.gender = randGender(passenger.id);
    passenger.is_vip = randRare(passenger.id);
    passenger.is_equipped = randRare(passenger.id);
    passenger.frustration = randNumber(20, passenger.id);
    passenger.peoplePass = 0;

    // Pobranie klucza do semaforów A
    key_t key = ftok(".", 'A');
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Dołączenie do istniejących semaforów (4 semafory)
    semID = alokujSemafor(key, 4, 0);
    if (semID == -1) {
        printf("[PID %d] Wychodzi z lotniska\033[0;31m\n", getpid());
        cleanup();
    }
    // Semafory R, do wewnętrznego porozumiewania się pasażerów

    key_t keySemforyPassenger = ftok(".", 'R');
    if (keySemforyPassenger == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    semSemforyPassengerID = alokujSemafor(keySemforyPassenger, 1, 0);
    if (semSemforyPassengerID == -1) {
        printf("[PID %d] Wychodzi z lotniska\n", getpid());
        return 0;
    }

    // Klucz do kolejki komunikatów potwierdzenie
    key_t kluczB = ftok(".", 'B');
    if (kluczB == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    msgID = msgget(kluczB, IPC_CREAT | 0600);
    if (msgID == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    // Klucz do kolejki komunikatów wysanie do kolejki
    key_t kluczF = ftok(".", 'F');
    if (kluczF == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    msgLotniskoID = msgget(kluczF, IPC_CREAT | 0600);
    if (msgLotniskoID == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    // Klucz do kolejki komunikatów wysanie do samolotu
    key_t kluczH = ftok(".", 'H');
    if (kluczH == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    msgSamolotID = msgget(kluczH, IPC_CREAT | 0600);
    if (msgSamolotID == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    // Pamięć dzielona: ilość ludzi
    key_t kluczD = ftok(".", 'D');
    if (kluczD == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    shmIDAmountOfPeople = shmget(kluczD, sizeof(int), IPC_CREAT | 0600);
    if (shmIDAmountOfPeople == -1) {
        perror("shmget - AmountOfPeople");
        exit(EXIT_FAILURE);
    }
    memoryAmountPeople = (int *) shmat(shmIDAmountOfPeople, NULL, 0);

    // Pamięć dzielona: ludzie na pokładzie
    key_t kluczE = ftok(".", 'E');
    if (kluczE == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    shmPeopleInID = shmget(kluczE, sizeof(int), IPC_CREAT | 0600);
    if (shmPeopleInID == -1) {
        perror("shmget - PeopleIn");
        exit(EXIT_FAILURE);
    }
    memoryPeopleIn = (int *) shmat(shmPeopleInID, NULL, 0);
    if (memoryPeopleIn == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną G, otwiera i zamyka bramke wejscia.
    key_t kluczG = ftok(".", 'G');
    if (kluczG == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    shmIOPassenger = shmget(kluczG, sizeof(int), IPC_CREAT | 0600);
    if (shmIOPassenger == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    IOPassenger = (int *) shmat(shmIOPassenger, NULL, 0);
    if (IOPassenger == (void *) -1) {
        perror("shmat");
        exit(1);
    }
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną 0, ilosc pasazerow na lotnisku - 1, ilosc pasazerow ogolna - 0;
    key_t kluczO = ftok(".", 'O');
    if (kluczO == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    PassenegerInAirportID = shmget(kluczO, 2 * sizeof(int), IPC_CREAT | 0600);
    if (PassenegerInAirportID == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    PassenegerInAirport = (int *) shmat(PassenegerInAirportID, NULL, 0);
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną X, pidy pasazerow
    key_t kluczX = ftok(".", 'X');
    if (kluczX == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    allPassengersPidID = shmget(kluczX, MAXPASSENGERS * sizeof(int), IPC_CREAT | 0600);
    if (allPassengersPidID == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    allPassengersPid = (int *) shmat(allPassengersPidID, NULL, 0);
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną X, informacja o zamknieciu
    key_t kluczV = ftok(".", 'V');
    if (kluczV == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    passenegerIsOverID = shmget(kluczV, sizeof(int), IPC_CREAT | 0600);
    if (passenegerIsOverID == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    passenegerIsOver = (int *) shmat(passenegerIsOverID, NULL, 0);

    safewaitsemafor(semSemforyPassengerID, 0, 0);
    usleep((rand() % 1000000) + 100000);
    struct messagePassengerAirport messagepassengerairport;
    struct messagePassengerAirport messagepassengerairplane;
    struct messagePassenger messFromAirplane;
    // Pierwszy etap: wysłanie danych pasażera do FIFO_NAME
    allPassengersPid[PassenegerInAirport[1]] = passenger.id;
    atomic_fetch_add(&PassenegerInAirport[0], 1);
    atomic_fetch_add(&PassenegerInAirport[1], 1);
    signalSemafor(semSemforyPassengerID, 0);

    while (passenegerIsOver[0] == 0) {
        safewaitsemafor(semID, 1, 0);
        if (passenegerIsOver[0] == 1) {
            break;
        }
        printf("[PID %d] Wchodzi na odprawe\033[0;31m\n", passenger.id);

        messagepassengerairport.mtype = 1; // Ustaw typ komunikatu (np. 1 dla nowych pasażerów)
        messagepassengerairport.passenger = passenger;

        // Wysyłanie komunikatu
        if (msgsnd(msgLotniskoID, &messagepassengerairport, sizeof(struct passenger), 0) == -1) {
            perror("msgsnd - sendPassengerToAirport");
            exit(EXIT_FAILURE);
        }

        // Odbieramy komunikat z kolejki (odpowiedź lotniska?)
        ssize_t ret = msgrcv(msgID, &message, sizeof(message.mvalue), passenger.id, 0);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                // Inny błąd
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
        }
        // Komunikat decyduje: 1 - przechodzimy dalej, 2 - koniec (przedmiot niebezpieczny), 0 - zmiana
        if (message.mvalue == 1) {
            goto AirplaneTicketing;
        }
        if (message.mvalue == 2) {
            // Wyrzucenie pasażera
            printf("[PID %d] Przedmiot niebezpieczny, koniec.\033[0;31m\n", passenger.id);
            atomic_fetch_sub(&PassenegerInAirport[0], 1); // Zmniejszenie liczby pasażerów
            return 0;
        }

        // Otrzymaliśmy np. wartość 0, zmieniamy wagę i kontynuujemy pętlę
        passenger.baggage_weight = randNumber(passenger.baggage_weight, (rand() % 100) + 1);
        continue;

        AirplaneTicketing:

        printf("[PID %d] Oczekuję na wejście, aktualnie %d z maks. %d\033[0;31m\n",
               passenger.id, memoryPeopleIn[0], memoryAmountPeople[0]);

        while (passenegerIsOver[0] == 0) {
            safewaitsemafor(semID, 2, 0);
            if (passenegerIsOver[0] == 1) {
                break;
            }
            messagepassengerairplane.mtype = 2; // Ustaw typ komunikatu (np. 1 dla nowych pasażerów)
            messagepassengerairplane.passenger = passenger;

            if (passenger.is_vip == 0) {
                if (IOPassenger[0] == 0) {
                    signalSemafor(semID, 2);
                    continue;
                }
            }

            // Sprawdzamy, czy jest jeszcze miejsce
            if (memoryPeopleIn[0] < memoryAmountPeople[0]) {
                memoryPeopleIn[0]++;
                if (msgsnd(msgSamolotID, &messagepassengerairplane, sizeof(struct passenger), 0) == -1) {
                    perror("msgsnd - sendPassengerToAirport");
                    exit(EXIT_FAILURE);
                }
                // Wysyłamy dane pasażera do drugiego FIFO
                printf("Pasażer wchodzi do samolotu\033[0;31m\n");

                atomic_fetch_sub(&PassenegerInAirport[0], 1); // Zmniejszenie liczby pasażerów
                return 0;
            }

            // Jeśli nie ma miejsca
            printf("[PID %d] Samolot odleciał lub brak miejsca, czekam...\033[0;31m\n", passenger.id);
            // Można tu dodać sleep, aby pasażer nie próbował natychmiast pętli
        }
    }
    atomic_fetch_sub(&PassenegerInAirport[0], 1); // Zmniejszenie liczby pasażerów
    printf("[PID %d] Wychodzi z lotniska\033[0;31m\n", getpid());
    return 0;
}

void cleanup() {
    // Odłączenie pamięci współdzielonej
    if (memoryAmountPeople != NULL) shmdt(memoryAmountPeople);
    if (memoryPeopleIn != NULL) shmdt(memoryPeopleIn);
    if (IOPassenger != NULL) shmdt(IOPassenger);
    if (PassenegerInAirport != NULL) shmdt(PassenegerInAirport);
    if (allPassengersPid != NULL) shmdt(allPassengersPid);
    if (passenegerIsOver != NULL) shmdt(passenegerIsOver);

    // Usunięcie pamięci współdzielonej
    shmctl(shmIDAmountOfPeople, IPC_RMID, NULL);
    shmctl(shmPeopleInID, IPC_RMID, NULL);
    shmctl(shmIOPassenger, IPC_RMID, NULL);
    shmctl(PassenegerInAirportID, IPC_RMID, NULL);
    shmctl(allPassengersPidID, IPC_RMID, NULL);
    shmctl(passenegerIsOverID, IPC_RMID, NULL);

    // Usunięcie semaforów
    semctl(semID, 0, IPC_RMID);
    semctl(semSemforyPassengerID, 0, IPC_RMID);

    // Usunięcie kolejek komunikatów
    msgctl(msgID, IPC_RMID, NULL);
    msgctl(msgLotniskoID, IPC_RMID, NULL);
    msgctl(msgSamolotID, IPC_RMID, NULL);
    exit(0);

}