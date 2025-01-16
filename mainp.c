#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "funkcje.h"

int N = 4;
int main() {
    key_t klucz;
    int semID;

    if ( (klucz = ftok(".", 'A')) == -1 )
    {
        printf("Blad ftok (A)\n");
        exit(2);
    }
    semID = alokujSemafor(klucz, N, IPC_CREAT | 0666);
    for (int i = 0; i < N; i++)
        inicjalizujSemafor(semID, i, 0); // inicjalizujemy zerami

    printf("Semafory gotowe!\n");

//    createNewFifo();
    int pid = fork();
    if (pid == 0) {
        // Proces potomny uruchamia program `pasazer`
        execl("./pasazer", "pasazer", NULL);
        perror("Nie udało się uruchomić programu pasazer");
        return 1;
    } else if (pid > 0) {
        // Proces rodzic czeka na zakończenie potomka

    } else {
        perror("fork");
        return 1;
    }
    pid = fork();
    if (pid == 0) {
        // Proces potomny uruchamia program `pasazer`
        execl("./ksamolotu", "ksamolotu", NULL);
        perror("Nie udało się uruchomić programu samolot");
        return 1;
    } else if (pid > 0) {
        // Proces rodzic czeka na zakończenie potomka

    } else {
        perror("fork");
        return 1;
    }
    pid = fork();
    if (pid == 0) {
        // Proces potomny uruchamia program `pasazer`
        execl("./dyspozytor", "dyspozytor", NULL);
        perror("Nie udało się uruchomić programu dyspozytor");
        return 1;
    } else if (pid > 0) {
        // Proces rodzic czeka na zakończenie potomka

    } else {
        perror("fork");
        return 1;
    }
    pid = fork();
    if (pid == 0) {
        // Proces potomny uruchamia program `pasazer`
        execl("./lotnisko", "lotnisko", NULL);
        perror("Nie udało się uruchomić programu lotnisko");
        return 1;
    } else if (pid > 0) {
        // Proces rodzic czeka na zakończenie potomka
        signalSemafor(semID, 1);
        wait(NULL);
        wait(NULL);
        wait(NULL);
        wait(NULL);
        printf("Program lotnisko zakończył działanie.\n");
        printf("Program pasazer zakończył działanie.\n");
        printf("Program ksamolotu zakończył działanie.\n");
        printf("Program dyzpozytor zakończył działanie.\n");
    } else {
        perror("fork");
        return 1;
    }
    return 0;
}