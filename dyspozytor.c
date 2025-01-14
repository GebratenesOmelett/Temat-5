#include "funkcje.h"
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>


#define MAXAIRPLANES 10

int semID, msgID, shmID, msgIDdyspozytor;
int numberOfPlanes;
int *memory;
int N = 4;

void *controller(void *arg) {
    int i = *(int *)arg;
    struct depoPassenger message;
    free(arg); // Pamięć już niepotrzebna

    message.mtype = 1; // Typ komunikatu
    message.data = i;  // Dane do wysłania

    while (1) {
        if (msgsnd(msgIDdyspozytor, &message, sizeof(int), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
        printf("wysłane\n");
    }
    return NULL;
}


int main(){
    key_t kluczA, kluczB, kluczC, kluczD;

    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }

    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);

    if ((kluczD = ftok(".", 'E')) == -1) {
        printf("Blad ftok (E)\n");
        exit(2);
    }
    msgctl(msgIDdyspozytor, IPC_RMID, NULL);
    msgIDdyspozytor= msgget(kluczD, IPC_CREAT | 0666);
    if(msgID == -1){
        printf("blad kolejki komunikatow pasazerow\n");
        exit(1);
    }

    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }
    shmID = shmget(kluczC, (MAXAIRPLANES+1) * sizeof(int), IPC_CREAT | 0666);
    if(shmID == -1){
        printf("blad pamieci dzielodznej pasazerow\n");
        exit(1);
    }

    memory = (int*)shmat(shmID, NULL, 0);
    waitSemafor(semID, 3,0);

    printf("ilosc samolotow dyspozytor %d\n",memory[MAXAIRPLANES]);
    numberOfPlanes = memory[MAXAIRPLANES];

    pthread_t watki[numberOfPlanes];

    for (int i = 0; i < numberOfPlanes; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            return 1;
        }
        *arg = i;
        pthread_create(&watki[i], NULL, controller, arg);
    }

    for (int j = 0; j < numberOfPlanes; j++) {
        pthread_join(watki[j], NULL);
    }

    return 0;
}