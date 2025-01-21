#include "funkcje.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define MAXAIRPLANES 10
#define SG1 SIGUSR1
#define SG2 SIGUSR2

int semID = -1, shmID = -1, shmAmountofPeople = -1, shmIOPassenger = -1, shmPIDID = -1;
int numberOfPlanes;
int *memory = (void *)-1, *memoryAmountPeople = (void *)-1, *IOPassenger = (void *)-1, *memoryPID = (void *)-1;
int N = 4;

// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void cleanupResources();
static void *controller(void *arg);

void handleSignalKill(int sig) {
    printf("Odebrano sygnał %d (SIGUSR2): Zatrzymuję program i czyszczę zasoby dyspozytor.\n", sig);
    cleanupResources(); // Sprzątanie zasobów
    exit(0);
}

int main() {
    // Obsługa sygnału SIGINT
    struct sigaction sa;
    sa.sa_handler = handleSignalKill;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("[dyspozytor] Błąd ustawiania obsługi sygnału");
        return 1;
    }

    printf("[dyspozytor] PID: %d - Czekam na SIGUSR2...\n", getpid());

    key_t kluczA, kluczC, kluczF, kluczG, kluczT;

    // Inicjalizacja semafora
    if ((kluczA = ftok(".", 'A')) == -1) {
        perror("Blad ftok (A)");
        exit(2);
    }
    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);

    // Inicjalizacja pamięci dzielonej D
    if ((kluczC = ftok(".", 'D')) == -1) {
        perror("Blad ftok (D)");
        exit(2);
    }
    shmID = shmget(kluczC, (MAXAIRPLANES + 1) * sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        perror("Blad przy tworzeniu pamieci dzielonej (D)");
        exit(1);
    }
    memory = (int *)shmat(shmID, NULL, 0);
    if (memory == (void *)-1) {
        perror("Blad przy podlaczaniu pamieci dzielonej (D)");
        exit(1);
    }

    // Inicjalizacja pamięci dzielonej F
    if ((kluczF = ftok(".", 'F')) == -1) {
        perror("Blad ftok (F)");
        exit(2);
    }
    shmAmountofPeople = shmget(kluczF, MAXAIRPLANES * sizeof(int), IPC_CREAT | 0666);
    if (shmAmountofPeople == -1) {
        perror("Blad przy tworzeniu pamieci dzielonej (F)");
        exit(1);
    }
    memoryAmountPeople = (int *)shmat(shmAmountofPeople, NULL, 0);
    if (memoryAmountPeople == (void *)-1) {
        perror("Blad przy podlaczaniu pamieci dzielonej (F)");
        exit(1);
    }

    // Inicjalizacja pamięci dzielonej G
    if ((kluczG = ftok(".", 'G')) == -1) {
        perror("Blad ftok (G)");
        exit(2);
    }
    shmIOPassenger = shmget(kluczG, MAXAIRPLANES * sizeof(int), IPC_CREAT | 0666);
    if (shmIOPassenger == -1) {
        perror("Blad przy tworzeniu pamieci dzielonej (G)");
        exit(1);
    }
    IOPassenger = (int *)shmat(shmIOPassenger, NULL, 0);
    if (IOPassenger == (void *)-1) {
        perror("Blad przy podlaczaniu pamieci dzielonej (G)");
        exit(1);
    }

    // Inicjalizacja pamięci dzielonej T
    if ((kluczT = ftok(".", 'T')) == -1) {
        perror("Blad ftok (T)");
        exit(2);
    }
    shmPIDID = shmget(kluczT, sizeof(int), IPC_CREAT | 0666);
    if (shmPIDID == -1) {
        perror("Blad przy tworzeniu pamieci dzielonej (T)");
        exit(1);
    }
    memoryPID = (int *)shmat(shmPIDID, NULL, 0);
    if (memoryPID == (void *)-1) {
        perror("Blad przy podlaczaniu pamieci dzielonej (T)");
        exit(1);
    }

    waitSemafor(semID, 3, 0);
    waitSemafor(semID, 0, 0);

    printf("Ilosc samolotow dyspozytor: %d\n", memory[MAXAIRPLANES]);
    numberOfPlanes = memory[MAXAIRPLANES];

    pthread_t watki[numberOfPlanes];

    // Tworzenie wątków
    for (int i = 0; i < numberOfPlanes; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            cleanupResources();
            return 1;
        }
        *arg = i;
        if (pthread_create(&watki[i], NULL, controller, arg) != 0) {
            perror("Błąd przy tworzeniu wątku");
            free(arg);
            cleanupResources();
            return 1;
        }
    }

    // Oczekiwanie na zakończenie wątków
    for (int j = 0; j < numberOfPlanes; j++) {
        pthread_join(watki[j], NULL);
    }

    // Zniszczenie mutexu przed zakończeniem programu
    pthread_mutex_destroy(&mutex);

    cleanupResources();
    return 0;
}

void *controller(void *arg) {
    int i = *(int *)arg;
    printf("Utworzono wątek controller %d\n", i);
    free(arg);

    while (1) {
        usleep(rand() % 10000000 + 2000000);
        printf("Zmieniam %d na otwarte-------------------\n", i);

        pthread_mutex_lock(&mutex);

        IOPassenger[i] = 1;
        usleep(10000000);
        IOPassenger[i] = 0;

        pthread_mutex_unlock(&mutex);
        printf("Zmieniam %d na zamknięte-------------------\n", i);

        if ((rand() % 100 + 1) == 1) {
            kill(memoryPID[0], SIGUSR1);
        }
    }
    return NULL;
}

void cleanupResources() {
    printf("Czyszczenie zasobów...\n");

    // Odłączenie pamięci dzielonej
    if (memory != (void *)-1 && shmdt(memory) == -1) {
        perror("Blad przy odłączaniu pamieci dzielonej (memory)");
    }
    if (memoryAmountPeople != (void *)-1 && shmdt(memoryAmountPeople) == -1) {
        perror("Blad przy odłączaniu pamieci dzielonej (memoryAmountPeople)");
    }
    if (IOPassenger != (void *)-1 && shmdt(IOPassenger) == -1) {
        perror("Blad przy odłączaniu pamieci dzielonej (IOPassenger)");
    }
    if (memoryPID != (void *)-1 && shmdt(memoryPID) == -1) {
        perror("Blad przy odłączaniu pamieci dzielonej (memoryPID)");
    }

    // Usuwanie pamięci dzielonej
    if (shmID != -1 && shmctl(shmID, IPC_RMID, NULL) == -1) {
        perror("Blad przy usuwaniu pamieci dzielonej (shmID)");
    }
    if (shmAmountofPeople != -1 && shmctl(shmAmountofPeople, IPC_RMID, NULL) == -1) {
        perror("Blad przy usuwaniu pamieci dzielonej (shmAmountofPeople)");
    }
    if (shmIOPassenger != -1 && shmctl(shmIOPassenger, IPC_RMID, NULL) == -1) {
        perror("Blad przy usuwaniu pamieci dzielonej (shmIOPassenger)");
    }
    if (shmPIDID != -1 && shmctl(shmPIDID, IPC_RMID, NULL) == -1) {
        perror("Blad przy usuwaniu pamieci dzielonej (shmPIDID)");
    }

    // Usuwanie semaforów
    if (semID != -1 && semctl(semID, 0, IPC_RMID) == -1) {
        perror("Blad przy usuwaniu semaforów");
    }

    // Zniszczenie mutexu
    if (pthread_mutex_destroy(&mutex) != 0) {
        perror("Blad przy niszczeniu mutexu");
    }

    printf("Zasoby zostały wyczyszczone.\n");
}
