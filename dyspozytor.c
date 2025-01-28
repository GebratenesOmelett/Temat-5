#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdatomic.h>
#include "funkcje.h"

int shmIOPassenger, semSemforyPassengerID, semSemforyAirplaneID, passenegerIsOverID, PassenegerInAirportID,
        airplaneIsOverID, semID, shmPidAirplaneID, airplanesInAirportID, lotniskoPidID, allPassengersPidID, airplanesPidsID;
int *IOPassenger, *PidAirplane, *airplanesInAirport, *PassenegerInAirport, *allPassengersPid, *airplanesPids, *
        lotniskoPid, *passenegerIsOver, *airplaneIsOver;
pthread_t threadSignalPassengers, threadSignalAirplanes, threadKillPassengers;
volatile atomic_int startKill = 0;

static void cleanup();
static void *SignalPassengers(void *arg);
static void *SignalAirplanes(void *arg);
static void *signalPassengersKill(void *arg);
#define SIGNALFLY SIGUSR1
#define SIGNALKILL SIGUSR2
#define MAXAIRPLANES 10
#define MAXPASSENGERS 100

int main() {
    // Inicjalizacja semaforów
    key_t key = ftok(".", 'A');
    if (key == -1) {
        perror("Błąd ftok dla semaforów");
        exit(EXIT_FAILURE);
    }

    semID = alokujSemafor(key, 4, 0);
    if (semID == -1) {
        perror("Błąd alokacji semaforów");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja semaforów dla pasażerów
    key_t keySemforyPassenger = ftok(".", 'R');
    if (keySemforyPassenger == -1) {
        perror("Błąd ftok dla semaforów pasażerów");
        exit(EXIT_FAILURE);
    }

    semSemforyPassengerID = alokujSemafor(keySemforyPassenger, 1, 0);
    if (semSemforyPassengerID == -1) {
        perror("Błąd alokacji semaforów pasażerów");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja semaforów dla samolotów
    key_t keySemforyAirport = ftok(".", 'K');
    if (keySemforyAirport == -1) {
        perror("Błąd ftok dla semaforów samolotów");
        exit(EXIT_FAILURE);
    }

    semSemforyAirplaneID = alokujSemafor(keySemforyAirport, 1, 0);
    if (semSemforyAirplaneID == -1) {
        perror("Błąd alokacji semaforów samolotów");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja pamięci dzielonej: bramka wejścia dla pasażerów
    key_t kluczG = ftok(".", 'G');
    if (kluczG == -1) {
        perror("Błąd ftok dla pamięci dzielonej bramki wejścia");
        exit(EXIT_FAILURE);
    }
    shmIOPassenger = shmget(kluczG, sizeof(int), IPC_CREAT | 0666);
    if (shmIOPassenger == -1) {
        perror("Błąd tworzenia pamięci dzielonej bramki wejścia");
        exit(EXIT_FAILURE);
    }
    IOPassenger = (int *) shmat(shmIOPassenger, NULL, 0);
    if (IOPassenger == (void *) -1) {
        perror("Błąd dołączania pamięci dzielonej bramki wejścia");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja pamięci dzielonej: PID samolotu
    key_t kluczP = ftok(".", 'P');
    if (kluczP == -1) {
        perror("Błąd ftok dla pamięci dzielonej PID samolotu");
        exit(EXIT_FAILURE);
    }
    shmPidAirplaneID = shmget(kluczP, sizeof(int), IPC_CREAT | 0666);
    if (shmPidAirplaneID == -1) {
        perror("Błąd tworzenia pamięci dzielonej PID samolotu");
        exit(EXIT_FAILURE);
    }
    PidAirplane = (int *) shmat(shmPidAirplaneID, NULL, 0);
    if (PidAirplane == (void *) -1) {
        perror("Błąd dołączania pamięci dzielonej PID samolotu");
        exit(EXIT_FAILURE);
    }

    // Inicjalizacja pamięci dzielonej: liczba samolotów
    key_t kluczN = ftok(".", 'N');
    if (kluczN == -1) {
        perror("Błąd ftok dla pamięci dzielonej liczby samolotów");
        exit(EXIT_FAILURE);
    }
    airplanesInAirportID = shmget(kluczN, sizeof(int), IPC_CREAT | 0666);
    if (airplanesInAirportID == -1) {
        perror("Błąd tworzenia pamięci dzielonej liczby samolotów");
        exit(EXIT_FAILURE);
    }
    airplanesInAirport = (int *) shmat(airplanesInAirportID, NULL, 0);

    // Inicjalizacja pamięci dzielonej: liczba pasażerów na lotnisku
    key_t kluczO = ftok(".", 'O');
    if (kluczO == -1) {
        perror("Błąd ftok dla pamięci dzielonej liczby pasażerów");
        exit(EXIT_FAILURE);
    }
    PassenegerInAirportID = shmget(kluczO, 2 * sizeof(int), IPC_CREAT | 0666);
    if (PassenegerInAirportID == -1) {
        perror("Błąd tworzenia pamięci dzielonej liczby pasażerów");
        exit(EXIT_FAILURE);
    }
    PassenegerInAirport = (int *) shmat(PassenegerInAirportID, NULL, 0);

    // Inicjalizacja pamięci dzielonej: PID-y pasażerów
    key_t kluczX = ftok(".", 'X');
    if (kluczX == -1) {
        perror("Błąd ftok dla pamięci dzielonej PID-ów pasażerów");
        exit(EXIT_FAILURE);
    }
    allPassengersPidID = shmget(kluczX, MAXPASSENGERS * sizeof(int), IPC_CREAT | 0666);
    if (allPassengersPidID == -1) {
        perror("Błąd tworzenia pamięci dzielonej PID-ów pasażerów");
        exit(EXIT_FAILURE);
    }
    allPassengersPid = (int *) shmat(allPassengersPidID, NULL, 0);

    // Inicjalizacja pamięci dzielonej: PID-y samolotów
    key_t kluczU = ftok(".", 'U');
    if (kluczU == -1) {
        perror("Błąd ftok dla pamięci dzielonej PID-ów samolotów");
        exit(EXIT_FAILURE);
    }
    airplanesPidsID = shmget(kluczU, MAXAIRPLANES * sizeof(int), IPC_CREAT | 0666);
    if (airplanesPidsID == -1) {
        perror("Błąd tworzenia pamięci dzielonej PID-ów samolotów");
        exit(EXIT_FAILURE);
    }
    airplanesPids = (int *) shmat(airplanesPidsID, NULL, 0);

    // Inicjalizacja pamięci dzielonej: PID lotniska
    key_t kluczW = ftok(".", 'W');
    if (kluczW == -1) {
        perror("Błąd ftok dla pamięci dzielonej PID lotniska");
        exit(EXIT_FAILURE);
    }
    lotniskoPidID = shmget(kluczW, sizeof(int), IPC_CREAT | 0666);
    if (lotniskoPidID == -1) {
        perror("Błąd tworzenia pamięci dzielonej PID lotniska");
        exit(EXIT_FAILURE);
    }
    lotniskoPid = (int *) shmat(lotniskoPidID, NULL, 0);
    //Inicjalizacja pamięć dzieloną V, informacja o zamknieciu lotniska
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
    // Inicjalizacja pamięć dzieloną Y, informacja o zamknieciu samolotu
    key_t kluczY = ftok(".", 'Y');
    if (kluczY == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    airplaneIsOverID = shmget(kluczY, sizeof(int), IPC_CREAT | 0666);
    if (airplaneIsOverID == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    airplaneIsOver = (int *) shmat(airplaneIsOverID, NULL, 0);
    airplaneIsOver[0] = 0;
    passenegerIsOver[0] = 0;

    // Sygnalizacja gotowości semaforów
    signalSemafor(semSemforyPassengerID, 0);
    signalSemafor(semSemforyAirplaneID, 0);
    waitSemafor(semID, 0, 0);
    // Uruchomienie wątków
    pthread_create(&threadSignalAirplanes, NULL, SignalAirplanes, NULL);
    pthread_create(&threadSignalPassengers, NULL, SignalPassengers, NULL);
    pthread_create(&threadKillPassengers, NULL, signalPassengersKill, NULL);

    // Oczekiwanie na zakończenie wątków
    pthread_join(threadSignalAirplanes, NULL);
    pthread_join(threadSignalPassengers, NULL);
    pthread_join(threadKillPassengers, NULL);

    printf("Dyspozytor kończy swoją pracę.\033[0;33m\n");
    cleanup();
    return 0;
}
// Funkcja wątku otwierającego bramkę dla pasażerów
void *SignalPassengers(void *arg) {
    while (startKill == 0) {
        usleep(rand() % 10000000 + 2000000); // Losowe opóźnienie między otwarciami bramki
        printf("Otwieram bramkę dla pasażerów. PID[%d]\033[0;33m\n", getpid());
        IOPassenger[0] = 1; // Otwarcie bramki
        usleep(10000000); // Czas otwarcia bramki
        printf("Zamykam bramkę dla pasażerów. PID[%d]\033[0;33m\n", getpid());

        IOPassenger[0] = 0; // Zamknięcie bramki
    }
    return NULL;
}
// Funkcja wątku wysyłającego sygnały do samolotów
void *SignalAirplanes(void *arg) {
    while (startKill == 0) {
        sleep((rand() % 60) + 20); // Losowe opóźnienie między sygnałami
        kill(PidAirplane[0], SIGUSR1); // Wysłanie sygnału do samolotu
    }
    return NULL;
}
// Funkcja wątku wysyłającego sygnały kończące do pasażerów i samolotów
void *signalPassengersKill(void *arg) {
    sleep((rand() % 200) + 50);
    printf("Rozpoczynanie procedury kończenia pracy pasażerów i samolotów.\033[0;33m\n");
    passenegerIsOver[0] = 1;
    airplaneIsOver[0] = 1;

    // Wysłanie sygnału do lotniska
    kill(lotniskoPid[0], SIGUSR2);
    // Wysłanie sygnału do wszystkich pasażerów
    for (int i = 0; i < PassenegerInAirport[1]; i++) {
        kill(allPassengersPid[i], SIGUSR2);
    }
    // Wysłanie sygnału do wszystkich samolotów
    for (int i = 0; i < airplanesInAirport[0]; i++) {
        kill(airplanesPids[i], SIGUSR2);
    }
    atomic_store(&startKill, 1); // Bezpieczne zakończenie pętli głównej
}

void cleanup() {
    // Usunięcie pamięci dzielonej
    shmdt(IOPassenger) == -1 || shmctl(shmIOPassenger, IPC_RMID, NULL);
    shmdt(PassenegerInAirport) == -1 || shmctl(PassenegerInAirportID, IPC_RMID, NULL);
    shmdt(allPassengersPid) == -1 || shmctl(allPassengersPidID, IPC_RMID, NULL);
    shmdt(airplanesPids) == -1 || shmctl(airplanesPidsID, IPC_RMID, NULL);

    // Usunięcie semaforów
    zwolnijSemafor(semID,0);
    zwolnijSemafor(semSemforyPassengerID,0);
    zwolnijSemafor(semSemforyAirplaneID,0);

    printf("Wszystkie zasoby zostały zwolnione.\n");
}
