#include "funkcje.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <malloc.h>

#define FIFO_NAME "passengerFifo"
#define MAXAIRPLANES 10

#define SIGNALKILL SIGUSR2

pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
int N = 4;
struct Node *node = NULL;
int semID, msgID, shmID;
int occupied;
sem_t thread_ready[3];
int fifoSend;
int *memory;
int assigned_thread;
// Wskaźnik zajętości wątku
int thread_busy[3] = {0, 0, 0};
pthread_t threads[3];

static void adjustFrustrationOrder(struct Node *head);
static void addFrustration(struct Node *head);
static void *securityControl(void *arg);
static void cleanupResources();


void handleSignalKill(int sig) {
    printf("Odebrano sygnał %d (SIGUSR2): Zatrzymuję program i czyszczę zasoby.\n", sig);
    cleanupResources(); // Sprzątanie zasobów
}

int main() {
    //############################## Obsługa sygnału
    struct sigaction saKill;

    saKill.sa_handler = handleSignalKill;
    saKill.sa_flags = 0;
    sigemptyset(&saKill.sa_mask);
    if(sigaction(SIGNALKILL, &saKill, NULL) == -1){
        perror("Błąd SIGNALKILL");
        return 1;
    }

//###############################
    key_t kluczA, kluczB, kluczC;
    if ((kluczA = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }
    semID = alokujSemafor(kluczA, N, IPC_CREAT | 0666);
    if (semID == -1) {
        printf("blad semaforow\n");
        exit(1);
    }
    if ((kluczB = ftok(".", 'C')) == -1) {
        printf("Blad ftok (C)\n");
        exit(2);
    }
    msgID = msgget(kluczB, IPC_CREAT | 0666);
    if (msgID == -1) {
        printf("blad kolejki komunikatow lotnisko\n");
        exit(1);
    }

    if ((kluczC = ftok(".", 'D')) == -1) {
        printf("Blad ftok (D)\n");
        exit(2);
    }
    shmID = shmget(kluczC, (MAXAIRPLANES + 1) * sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        printf("blad pamieci dzielodznej lotniska\n");
        exit(1);
    }

    waitSemafor(semID, 2, 0);
    memory = (int *) shmat(shmID, NULL, 0);
    fifoSend = open(FIFO_NAME, O_RDONLY);
    if (fifoSend == -1) {
        perror("Błąd przy otwieraniu FIFO do odczytu");
        return 1;
    }
    printf("działą fifoSend : %d\n", fifoSend);


    // Inicjalizacja semaforów dla wątków
    for (long i = 0; i < 3; i++) {
        long *thread_id = malloc(sizeof(long));
        *thread_id = i;
        sem_init(&thread_ready[i], 0, 0);
        pthread_create(&threads[i], NULL, securityControl, thread_id);
    }

    // Initialize the linked list head to NULL
    while (1) {
        struct passenger p;
        ssize_t bytesRead = read(fifoSend, &p, sizeof(struct passenger));
        if (bytesRead < sizeof(struct passenger)) {
            perror("Error reading from FIFO");
            exit(EXIT_FAILURE);
        }
        if (pthread_mutex_lock(&list_mutex) != 0) {
            perror("Mutex lock failed");
            exit(EXIT_FAILURE);
        }
        append(&node, p);  // Append passenger to the list
        pthread_mutex_unlock(&list_mutex);
        printList(node);   // Print the updated list

        assigned_thread = -1;
        // Check if any thread is free
        for (int i = 0; i < 3; i++) {
            if (thread_busy[i] == 0) {
                assigned_thread = i;
                break;
            }
        }

        if (assigned_thread >= 0) {
            thread_busy[assigned_thread] = 1; // Mark as busy
            sem_post(&thread_ready[assigned_thread]); // Signal the thread to start processing
        } else {
            printf("Wszystkie wątki zajęte, dane oczekują w kolejce\n");
        }

        // Adjust frustration order and update frustration level
        if (pthread_mutex_lock(&list_mutex) != 0) {
            perror("Mutex lock failed");
            exit(EXIT_FAILURE);
        }
        adjustFrustrationOrder(node);
        addFrustration(node);
        pthread_mutex_unlock(&list_mutex);
    }

    // Wait for threads to finish
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    // Close the FIFO
    close(fifoSend);

    // Remove FIFO
    unlink(FIFO_NAME);

    return 0;
}

void *securityControl(void *arg) {
    long thread_id = *((long *) arg);
    free(arg);
    printf("Thread ID: %ld\n", thread_id);
    while (1) {
        sem_wait(&thread_ready[thread_id]); // Wait for signal from main thread

        if (pthread_mutex_lock(&list_mutex) != 0) {
            perror("Mutex lock failed");
            exit(EXIT_FAILURE);
        }

        struct messagePassenger messageFirst;
        struct messagePassenger messageSecond;
        struct Node *first_passenger = node;
        if (first_passenger) {
            node = node->next;
        }

        struct Node *second_passenger = NULL;
        if (node) {
            if (first_passenger->passenger->gender == node->passenger->gender) {
                second_passenger = node;
                node = node->next;
            }
        }

        pthread_mutex_unlock(&list_mutex);

        // Process first passenger
        if (first_passenger) {
            messageFirst.mtype = first_passenger->passenger->id;
            if (first_passenger->passenger->baggage_weight > memory[first_passenger->passenger->airplaneNumber]) {
                printf("Wątek %ld: Za duży bagaż u pasażera %d\n", thread_id, first_passenger->passenger->id);
                messageFirst.mvalue = 0;
            } else {
                if (first_passenger->passenger->is_equipped == 1) {
                    printf("Proba wniesienia przedmiotu niebezpiecznego, wyrzucenie pasazera o id %d\n",
                           first_passenger->passenger->id);
                    messageFirst.mvalue = 2;
                } else {
                    messageFirst.mvalue = 1;
                }
            }
            if (msgsnd(msgID, &messageFirst, sizeof(messageFirst.mvalue), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }
            free(first_passenger->passenger);
            free(first_passenger);
        }

        // Process second passenger
        if (second_passenger) {
            messageSecond.mtype = second_passenger->passenger->id;
            if (second_passenger->passenger->baggage_weight > memory[second_passenger->passenger->airplaneNumber]) {
                printf("Wątek %ld: Za duży bagaż u pasażera %d\n", thread_id, second_passenger->passenger->id);
                messageSecond.mvalue = 0;
            } else {
                if (second_passenger->passenger->is_equipped == 1) {
                    printf("Proba wniesienia przedmiotu niebezpiecznego, wyrzucenie pasazera o id %d\n",
                           second_passenger->passenger->id);
                    messageSecond.mvalue = 2;
                } else {
                    messageSecond.mvalue = 1;
                }
            }
            if (msgsnd(msgID, &messageSecond, sizeof(messageSecond.mvalue), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }
            free(second_passenger->passenger);
            free(second_passenger);
        }

        usleep(randNumber(1000)); // Simulate processing time

        if (pthread_mutex_lock(&list_mutex) != 0) {
            perror("Mutex lock failed");
            exit(EXIT_FAILURE);
        }
        thread_busy[thread_id] = 0; // Mark thread as ready
        pthread_mutex_unlock(&list_mutex);

        sleep(randNumber(4)); // Optional delay
    }

    return NULL;
}

void adjustFrustrationOrder(struct Node *head) {
    struct Node *current = head;
    struct passenger *temp;
    while (current != NULL && current->next != NULL) {
        if (current->passenger->peoplePass < 3 &&
            current->passenger->frustration < current->next->passenger->frustration) {
            current->passenger->peoplePass++;
            temp = current->passenger;
            current->passenger = current->next->passenger;
            current->next->passenger = temp;
        }
        current = current->next;
    }
}

void addFrustration(struct Node *head) {
    struct Node *temp = head;
    while (temp != NULL) {
        temp->passenger->frustration += randNumber(10);
        temp = temp->next;
    }
}

void cleanupResources() {
    printf("Czyszczenie zasobów...\n");

    // Odłączenie pamięci dzielonej (jeśli jest zaalokowana)
    if (memory != (void *) -1) {
        if (shmdt(memory) == -1) {
            perror("Błąd przy odłączaniu pamięci dzielonej");
        }
        memory = (int *) -1;
    }

    // Usunięcie pamięci dzielonej (jeśli jest zaalokowana)
    if (shmID != -1) {
        if (shmctl(shmID, IPC_RMID, NULL) == -1) {
            perror("Błąd przy usuwaniu pamięci dzielonej (shmID)");
        }
        shmID = -1;
    }

    // Usunięcie semaforów (jeśli są zaalokowane)
    if (semID != -1) {
        if (semctl(semID, 0, IPC_RMID) == -1) {
            perror("Błąd przy usuwaniu semaforów (semID)");
        }
        semID = -1;
    }

    // Zwalnianie semaforów wątków
    for (int i = 0; i < 3; i++) {
        if (sem_destroy(&thread_ready[i]) != 0) {
            perror("Błąd przy niszczeniu semafora wątku");
        }
    }

    // Zniszczenie mutexu (jeśli jest zaalokowany)
    if (pthread_mutex_destroy(&list_mutex) != 0) {
        perror("Błąd przy niszczeniu mutexu");
    }

    // Zamykanie i usuwanie FIFO
    if (fifoSend != -1) {
        close(fifoSend);
        fifoSend = -1;
    }
    if (unlink(FIFO_NAME) == -1) {
        perror("Błąd przy usuwaniu FIFO");
    }

    // Zakończenie wątków (jeśli są aktywne)
    for (int i = 0; i < 3; i++) {
        // Jeśli wątek jeszcze działa, można zastosować odpowiednią metodę zatrzymania
        // np. poprzez odpowiedni mechanizm komunikacji z wątkami, aby zakończyć ich działanie.
        // W tym przypadku zakładamy, że wątki zakończą się same po zakończeniu głównego procesu.
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Błąd przy czekaniu na zakończenie wątku");
        }
    }

    // Dodatkowe oczyszczanie pamięci listy pasażerów
    struct Node *current = node;
    while (current != NULL) {
        struct Node *temp = current;
        current = current->next;
        free(temp->passenger);  // Zwalnianie pamięci pasażera
        free(temp);             // Zwalnianie pamięci węzła listy
    }

    // Zakończenie programu
    printf("Zasoby zostały wyczyszczone.\n");
}

