#include "funkcje.h"


#define FIFO_NAME "passengerFifo"
#define MAXAIRPLANES 10
#define PREFIX "fifoplane"

static void *createAndSendPassenger(void *arg);

int fifoSend;
int N = 4;
int semID, msgID, shmID, msgIDdyspozytor;
int *memory;
int numberOfPlanes;
int *tableOfFlights;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void fifoSendAirplane(int numberOfPlanes);

int main() {
    key_t kluczA, kluczB, kluczC, kluczD;


    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }

    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);
    waitSemafor(semID, 1, 0); //brak jeżeli nie ma mainp
    int liczba_watkow = 10000; // Liczba wątków do utworzenia
    pthread_t watki[liczba_watkow];

    createNewFifo(FIFO_NAME);

    if ((kluczB = ftok(".", 'C')) == -1) {
        printf("Blad ftok (C)\n");
        exit(2);
    }
    msgctl(msgID, IPC_RMID, NULL);
    msgID = msgget(kluczB, IPC_CREAT | 0666);
    if (msgID == -1) {
        printf("blad kolejki komunikatow pasazerow\n");
        exit(1);
    }

    //--------------------------------------------------------------------------
    if ((kluczD = ftok(".", 'E')) == -1) {
        printf("Blad ftok (E)\n");
        exit(2);
    }
    msgctl(msgIDdyspozytor, IPC_RMID, NULL);
    msgIDdyspozytor = msgget(kluczD, IPC_CREAT | 0666);
    if (msgIDdyspozytor == -1) {
        printf("blad kolejki komunikatow pasazerow\n");
        exit(1);
    }
    //--------------------------------------------------------------------------
    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }
    shmID = shmget(kluczC, (MAXAIRPLANES + 1) * sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        printf("blad pamieci dzielodznej pasazerow\n");
        exit(1);
    }
    memory = (int *) shmat(shmID, NULL, 0);
    signalSemafor(semID, 0);
    signalSemafor(semID, 0);
    waitSemafor(semID, 3, 0);
    printf("odblokowa pasazer\n");
    printf("ilosc samolotow pasazer %d\n", memory[MAXAIRPLANES]);
    numberOfPlanes = memory[MAXAIRPLANES];
    tableOfFlights = malloc(numberOfPlanes * sizeof(int));

    fifoSendAirplane(numberOfPlanes);
    printf("jest git-------------------------------------------");

    fifoSend = open(FIFO_NAME, O_WRONLY);
    if (fifoSend == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    printf("jest git");
    int i = 0;
    while (1) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&watki[i], NULL, createAndSendPassenger, id) != 0) {
            perror("Błąd przy tworzeniu wątku");
            free(id);
            return 1;
        }
        sleep(randNumber(1));
        i++;
    }
    for (int j = 0; j < liczba_watkow; j++) {
        pthread_join(watki[j], NULL);
    }

    close(fifoSend);
    free(tableOfFlights);
    pthread_mutex_destroy(&mutex);
    return 0;
}

// Funkcja, którą będą wykonywać wątki
void *createAndSendPassenger(void *arg) {
    int id = *((int *) arg);
    struct passenger *passenger = (struct passenger *) malloc(sizeof(struct passenger));
    struct messagePassenger message;
    struct depoPassenger messageDepo;
    free(arg);

    passenger->id = id;
    passenger->baggage_weight = randNumber(100);
    passenger->gender = randGender();
    passenger->is_vip = randRare();
    passenger->is_equipped = randRare();
    passenger->frustration = randNumber(20);
    passenger->peoplePass = 0;
    passenger->airplaneNumber = getAirplane(numberOfPlanes);
    sleep(randNumber(3));

    while (1) {
        if (write(fifoSend, passenger, sizeof(struct passenger)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&mutex);
        if (msgrcv(msgID, &message, sizeof(message.mvalue), passenger->id, 0) == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        if (message.mvalue == 1) {
            break;
        } else {
            passenger->baggage_weight = randNumber(passenger->baggage_weight);
        }
        pthread_mutex_unlock(&mutex);

        sleep(randNumber(5));
    }
    printf("pasazer %d  czeka\n", passenger->id);

    printf("table : %d", tableOfFlights[passenger->airplaneNumber]);

//    if(passenger->is_vip == 0){
//        if (msgrcv(msgIDdyspozytor, &messageDepo, sizeof(int), passenger->airplaneNumber, 0) == -1) {
//            perror("msgrcv");
//            exit(EXIT_FAILURE);
//        }
//    }
    pthread_mutex_lock(&mutex);
    signalSemafor(semID, 2);
    if (write(tableOfFlights[passenger->airplaneNumber], passenger, sizeof(struct passenger)) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex);
    free(passenger);
    return NULL;
}

void fifoSendAirplane(int numberOfPlanes) {
    char fifoName[20];
    for (int i = 0; i < numberOfPlanes; i++) {
        snprintf(fifoName, sizeof(fifoName), "%s%d", PREFIX, i); // Tworzymy unikalną nazwę FIFO
        printf("daje do zapisu         1\n");


        tableOfFlights[i] = open(fifoName, O_WRONLY);
        printf("zakończone           1\n");
//        if (tableOfFlights[i] == -1) {
//            perror("open : ");
//            exit(EXIT_FAILURE);
//        }

        printf("%d <------ nasze i\n",i);
        printf("FIFO %d created successfully.\n", tableOfFlights[i]);
    }
    printf("koncze ta funckje---------------------");
}

