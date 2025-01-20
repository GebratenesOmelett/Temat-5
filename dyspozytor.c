#include "funkcje.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXAIRPLANES 10


#define SG1 SIGUSR1
#define SG2 SIGUSR2


int semID, shmID, shmAmountofPeople, shmIOPassenger;
int numberOfPlanes;
int *memory, *memoryAmountPeople, *IOPassenger;
int N = 4;

// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void cleanupResources();
static void *controller(void *arg);
// Funkcja wątku

void handleSignalKill(int sig) {
    printf("Odebrano sygnał %d (SIGUSR2): Zatrzymuję program i czyszczę zasoby dyspozytor.\n", sig);
    cleanupResources(); // Sprzątanie zasobów
}
void *sendFly(void *arg){
//    printf("utworzone sygnał wylot %d\n", i);
    while (1) {
        usleep(rand() % 10000000 + 2000000);
        printf("wymuszony wylot");

    }
    return NULL;
}

int main() {
    //############################## Obsługa sygnału
    struct sigaction saKill;

    saKill.sa_handler = handleSignalKill;
    saKill.sa_flags = 0;
    sigemptyset(&saKill.sa_mask);
    if(sigaction(SIGUSR2, &saKill, NULL) == -1){
        perror("Błąd SIGNALKILL");
        return 1;
    }

//###############################
    key_t kluczA, kluczC, kluczF, kluczG;

    //---------------------------------------------------- Inicjalizacja klucza A
    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }

    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);


    //---------------------------------------------------- Inicjalizacja kolejke wiadomości E

    //---------------------------------------------------- Inicjalizacja pamięć dzieloną D
    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }

    shmID = shmget(kluczC, (MAXAIRPLANES+1) * sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    memory = (int *)shmat(shmID, NULL, 0);
    if (memory == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    //---------------------------------------------------- Inicjalizacja pamięć dzieloną F
    if ((kluczF = ftok(".", 'F')) == -1) {
        printf("Blad ftok (F)\n");
        exit(2);
    }

    shmAmountofPeople = shmget(kluczF, (MAXAIRPLANES) * sizeof(int), IPC_CREAT | 0666);
    if (shmAmountofPeople == -1) {
        printf("Blad pamieci dzielonej pasazerow\n");
        exit(1);
    }
    memoryAmountPeople = (int *)shmat(shmAmountofPeople, NULL, 0);
    if (memoryAmountPeople == (void *)-1) {
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
    //----------------------------------------------------
    waitSemafor(semID, 3, 0);
    waitSemafor(semID, 0, 0);
    printf("Ilosc samolotow dyspozytor %d\n", memory[MAXAIRPLANES]);
    numberOfPlanes = memory[MAXAIRPLANES];

    pthread_t watki[numberOfPlanes];

    // Tworzenie wątków
    for (int i = 0; i < numberOfPlanes; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            return 1;
        }
        *arg = i;
        pthread_create(&watki[i], NULL, controller, arg);
    }

    // Czekanie na zakończenie wątków
    for (int j = 0; j < numberOfPlanes; j++) {
        pthread_join(watki[j], NULL);
    }

//     Zniszczenie mutexu przed zakończeniem programu
    pthread_mutex_destroy(&mutex);

    return 0;
}

void *controller(void *arg) {
    int i = *(int *)arg;
    printf("utworzone wątek controller %d\n", i);
    free(arg); // Pamięć już niepotrzebna

    while (1) {
        usleep(rand() % 10000000 + 2000000);
        printf("zmieniam %d na otwarte-------------------", i);

        pthread_mutex_lock(&mutex);

        IOPassenger[i] = 1;
        usleep(10000000);
        IOPassenger[i] = 0;
        pthread_mutex_unlock(&mutex);
        printf("zmieniam %d na zamykanie-------------------", i);

    }
    return NULL;
}

void cleanupResources() {
    printf("Czyszczenie zasobów...\n");

    // Odłączenie pamięci dzielonej, jeśli została przydzielona
    if (memory != (void *)-1) {
        if (shmdt(memory) == -1) {
            perror("Błąd przy odłączaniu pamięci dzielonej (memory)");
        }
    }
    if (memoryAmountPeople != (void *)-1) {
        if (shmdt(memoryAmountPeople) == -1) {
            perror("Błąd przy odłączaniu pamięci dzielonej (memoryAmountPeople)");
        }
    }
    if (IOPassenger != (void *)-1) {
        if (shmdt(IOPassenger) == -1) {
            perror("Błąd przy odłączaniu pamięci dzielonej (IOPassenger)");
        }
    }

    // Usunięcie pamięci dzielonej, jeśli została przydzielona
    if (shmID != -1) {
        if (shmctl(shmID, IPC_RMID, NULL) == -1) {
            perror("Błąd przy usuwaniu pamięci dzielonej (shmID)");
        }
    }
    if (shmAmountofPeople != -1) {
        if (shmctl(shmAmountofPeople, IPC_RMID, NULL) == -1) {
            perror("Błąd przy usuwaniu pamięci dzielonej (shmAmountofPeople)");
        }
    }
    if (shmIOPassenger != -1) {
        if (shmctl(shmIOPassenger, IPC_RMID, NULL) == -1) {
            perror("Błąd przy usuwaniu pamięci dzielonej (shmIOPassenger)");
        }
    }

    // Usunięcie semaforów, jeśli zostały przydzielone
    if (semID != -1) {
        if (semctl(semID, 0, IPC_RMID) == -1) {
            perror("Błąd przy usuwaniu semaforów");
        }
    }

    // Zniszczenie mutexu, jeśli został zainicjalizowany
    if (pthread_mutex_destroy(&mutex) != 0) {
        perror("Błąd przy niszczeniu mutexu");
    }

    printf("Zasoby zostały wyczyszczone.\n");
}


