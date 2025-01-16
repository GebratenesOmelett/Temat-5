#include "funkcje.h"


#define MAXAIRPLANES 10
#define PREFIX "fifoplane"

int numberOfPlanes;
int semID, msgID, shmID;
int *memory;
int N = 4;
int *tableOfFlights;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Define the mutex
// Mutex do synchronizacji zapisu


static void createFIFOs(int numberOfPlanes);
static void readFromFifo(int numberOfPlanes);

void *airplaneControl(void *arg) {
    int i = *(int *)arg;

    free(arg);
    memory[i] = rand() % 100 + 1;
    printf("samolot %d z wagą %d\n", i, memory[i]);

    while (1) {
        struct passenger p;
        pthread_mutex_lock(&mutex); // Critical section start
        waitSemafor(semID,2,0);
        ssize_t bytesRead = read(tableOfFlights[i], &p, sizeof(struct passenger));

        printf("------------------------------------------------------ samolot : %d", i);
        print_passenger(&p);
        pthread_mutex_unlock(&mutex); // Critical section end
    }


    return NULL;
}
int main() {
    key_t kluczA, kluczC;
    numberOfPlanes = randNumber(MAXAIRPLANES);
    tableOfFlights = malloc(numberOfPlanes * sizeof(int));

    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }
    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);
    waitSemafor(semID, 0, 0); //brak jeżeli nie ma mainp

    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }
    shmID = shmget(kluczC, (MAXAIRPLANES+1) * sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        printf("blad pamieci dzielodznej samolotu\n");
        exit(1);
    }
    memory = (int *)shmat(shmID, NULL, 0);
    memory[MAXAIRPLANES] = numberOfPlanes;
    createFIFOs(numberOfPlanes);
    printf("ilosc samolotow to salomoty %d\n", numberOfPlanes);
    signalSemafor(semID, 3);
    signalSemafor(semID, 3);
    printf("odblokowa\n");
    readFromFifo(numberOfPlanes);

    if (numberOfPlanes <= 0) {
        fprintf(stderr, "Invalid number of planes: %d\n", numberOfPlanes);
        return 1;
    }

    pthread_t watki[numberOfPlanes];
    for (int i = 0; i < numberOfPlanes; i++) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            return 1;
        }
        *arg = i;
        pthread_create(&watki[i], NULL, airplaneControl, arg);
    }


    for (int j = 0; j < numberOfPlanes; j++) {
        if (pthread_join(watki[j], NULL) != 0) {
            perror("pthread_join");
        }
    }

//    printf("oki doki skończone");
    // Zniszczenie mutexu przed zakończeniem programu
    pthread_mutex_destroy(&mutex);

    free(tableOfFlights);
    shmdt(memory);
    return 0;
}

void createFIFOs(int numberOfPlanes) {
    char fifoName[20];
    for (int i = 0; i < numberOfPlanes; i++) {
        snprintf(fifoName, sizeof(fifoName), "%s%d", PREFIX, i); // Tworzymy unikalną nazwę FIFO
        createNewFifo(fifoName);
        printf("FIFO %s created successfully.\n", fifoName);
    }
}

void readFromFifo(int numberOfPlanes) {
    char fifoName[20];
    for (int i = 0; i < numberOfPlanes; i++) {
        snprintf(fifoName, sizeof(fifoName), "%s%d", PREFIX, i); // Tworzymy unikalną nazwę FIFO
        printf("daje do odczytu\n");
        tableOfFlights[i] = open(fifoName, O_RDONLY);
        printf("otworzono %d\n", tableOfFlights[i]);
//        if (tableOfFlights[i] == -1) {
//            perror("open\n------------");
//            exit(EXIT_FAILURE);
//        }
    }
    printf("koncze ta funckje--------------------- samolot\n");
}
