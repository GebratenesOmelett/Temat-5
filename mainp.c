#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "funkcje.h"
#include <sys/ipc.h>
#include <sys/sem.h>

#define SG1 SIGUSR1
#define SG2 SIGUSR2

#define SEMAPHORE_COUNT 4

// Tablica globalna PID-ów procesów potomnych
pid_t pids[SEMAPHORE_COUNT];

void handleSignalKill(int sig) {
    for (int i = 0; i < SEMAPHORE_COUNT; i++) {
        if (pids[i] > 0) {
            printf("Wysyłanie sygnału SIGNALKILL do procesu PID: %d\n", pids[i]);
            if (kill(pids[i], SIGUSR2) == -1) {
                perror("Nie udało się wysłać sygnału");
            }
        }
    }
}

void initializeSignalHandling() {
    struct sigaction saCtrlC;
    saCtrlC.sa_handler = handleSignalKill;
    saCtrlC.sa_flags = 0;
    sigemptyset(&saCtrlC.sa_mask);

    if (sigaction(SIGINT, &saCtrlC, NULL) == -1) {
        perror("Błąd SIGINT");
        _exit(1);
    }
}

int createAndInitializeSemaphores(int semaphoreCount) {
    key_t key;
    int semID;

    if ((key = ftok(".", 'A')) == -1) {
        perror("Błąd ftok");
        _exit(2);
    }

    semID = alokujSemafor(key, semaphoreCount, IPC_CREAT | 0666);
    for (int i = 0; i < semaphoreCount; i++) {
        inicjalizujSemafor(semID, i, 0);
    }

    printf("Semafory gotowe!\n");
    return semID;
}

pid_t startProcess(const char *programName) {
    pid_t pid = fork();

    if (pid == 0) {
        execl(programName, programName, NULL);
        perror("Nie udało się uruchomić programu");
        _exit(1);
    } else if (pid < 0) {
        perror("Błąd podczas tworzenia procesu");
        _exit(1);
    }

    return pid;
}

void clean(int semID) {
    // Usuwanie semaforów
    if (zwolnijSemafor(semID,0) == -1) {
        perror("Nie udało się usunąć semaforów");
    } else {
        printf("Semafory usunięte pomyślnie.\n");
    }

}

int main() {
    // Inicjalizacja obsługi sygnałów
    initializeSignalHandling();

    // Tworzenie i inicjalizacja semaforów
    int semID = createAndInitializeSemaphores(SEMAPHORE_COUNT);

    // Uruchamianie procesów potomnych
    pids[0] = startProcess("./pasazer");
    pids[1] = startProcess("./ksamolotu");
    pids[2] = startProcess("./dyspozytor");
    pids[3] = startProcess("./lotnisko");

    // Wyświetlanie PID-ów procesów potomnych
    printf("PID procesów: \n");
    printf("Pasazer: %d\n", pids[0]);
    printf("Ksamolotu: %d\n", pids[1]);
    printf("Dyspozytor: %d\n", pids[2]);
    printf("Lotnisko: %d\n", pids[3]);

    // Podnoszenie semafora i oczekiwanie na zakończenie procesów
    signalSemafor(semID, 1);

    for (int i = 0; i < SEMAPHORE_COUNT; i++) {
        wait(NULL);
    }

    // Wywołanie clean przed zakończeniem programu
    clean(semID);

    // Wyświetlenie komunikatów o zakończeniu procesów
    printf("Program lotnisko zakończył działanie.\n");
    printf("Program pasazer zakończył działanie.\n");
    printf("Program ksamolotu zakończył działanie.\n");
    printf("Program dyspozytor zakończył działanie.\n");

    return 0;
}
