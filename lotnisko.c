#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include "funkcje.h"
#include <semaphore.h>
#include <pthread.h>
#include <sys/msg.h>
#include <stdatomic.h>

#define MAXPASSENGERS 100
#define SIGNALKILL SIGUSR2

int count = 0, assigned_thread;
struct Node *node = NULL;
sem_t thread_ready[3];
int thread_busy[3] = {0, 0, 0};
pthread_t threads[3], threadCheck;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
int msgID, semID;
int *memory, *memoryAllPidPassengers, *lotniskoPid;
volatile atomic_int startKill = 0;

static void adjustFrustrationOrder(struct Node *head);
static void addFrustration(struct Node *head);
static void *securityControl(void *arg);
static void *checkSecurity(void *arg);

// Obsługa sygnału do bezpiecznego zamykania lotniska
void handleSignalKill(int sig) {
    while (node != NULL) {
        sleep(1); // Czekaj na przetworzenie pozostałych pasażerów
    }
    atomic_store(&startKill, 1); // Aktywuj flagę zamykania
}

int main(void) {
    // Konfiguracja handlera dla sygnału zamykania
    struct sigaction saKill;
    saKill.sa_handler = handleSignalKill;
    saKill.sa_flags = 0;
    sigemptyset(&saKill.sa_mask);
    if (sigaction(SIGNALKILL, &saKill, NULL) == -1) {
        perror("Błąd konfiguracji sygnału zamykania");
        _exit(1);
    }

    printf("Lotnisko: inicjalizacja...\n");

    // Tworzenie semaforów dla kontroli dostępu
    key_t kluczA = ftok(".", 'A');
    if (kluczA == -1) {
        perror("Błąd generowania klucza dla semaforów");
        exit(EXIT_FAILURE);
    }
    const int N = 4;
    semID = alokujSemafor(kluczA, N, 0);
    if (semID == -1) {
        perror("Błąd inicjalizacji semaforów");
        exit(EXIT_FAILURE);
    }

    // Udostępnianie PID lotniska przez pamięć współdzieloną
    key_t kluczW = ftok(".", 'W');
    if (kluczW == -1) {
        perror("Błąd generowania klucza dla PID lotniska");
        exit(EXIT_FAILURE);
    }
    int lotniskoPidID = shmget(kluczW, sizeof(int), IPC_CREAT | 0666);
    if (lotniskoPidID == -1) {
        perror("Błąd tworzenia pamięci dla PID lotniska");
        exit(EXIT_FAILURE);
    }
    lotniskoPid = (int *)shmat(lotniskoPidID, NULL, 0);
    lotniskoPid[0] = getpid();

    // Kolejka komunikatów od pasażerów
    key_t kluczF = ftok(".", 'F');
    if (kluczF == -1) {
        perror("Błąd generowania klucza kolejki pasażerów");
        exit(EXIT_FAILURE);
    }
    int msgLotniskoID = msgget(kluczF, IPC_CREAT | 0666);
    if (msgLotniskoID == -1) {
        perror("Błąd tworzenia kolejki pasażerów");
        exit(EXIT_FAILURE);
    }

    // Kolejka komunikatów zwrotnych do pasażerów
    key_t kluczB = ftok(".", 'B');
    if (kluczB == -1) {
        perror("Błąd generowania klucza kolejki zwrotnej");
        exit(EXIT_FAILURE);
    }
    msgID = msgget(kluczB, IPC_CREAT | 0666);
    if (msgID == -1) {
        perror("Błąd tworzenia kolejki zwrotnej");
        exit(EXIT_FAILURE);
    }

    // Pamięć współdzielona dla wagi bagażu
    key_t kluczC = ftok(".", 'C');
    if (kluczC == -1) {
        perror("Błąd generowania klucza dla wagi bagażu");
        exit(EXIT_FAILURE);
    }
    int shmID = shmget(kluczC, sizeof(int), IPC_CREAT | 0666);
    if (shmID == -1) {
        perror("Błąd tworzenia pamięci dla wagi bagażu");
        exit(EXIT_FAILURE);
    }
    memory = (int *)shmat(shmID, NULL, 0);
    waitSemafor(semID, 0, 0);

    // Inicjalizacja wątków kontroli bezpieczeństwa
    for (long i = 0; i < 3; i++) {
        long *thread_id = malloc(sizeof(long));
        *thread_id = i;
        sem_init(&thread_ready[i], 0, 0);
        pthread_create(&threads[i], NULL, securityControl, thread_id);
    }
    pthread_create(&threadCheck, NULL, checkSecurity, NULL);

    // Główna pętla obsługi pasażerów
    struct messagePassengerAirport msgFromPassenger;
    while (startKill == 0) {
        signalSemafor(semID, 1);
        ssize_t ret = msgrcv(msgLotniskoID, &msgFromPassenger, sizeof(struct passenger), 1, 0);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("Błąd odczytu komunikatu");
                exit(EXIT_FAILURE);
            }
        }
        pthread_mutex_lock(&list_mutex);
        append(&node, msgFromPassenger.passenger); // Dodaj pasażera do kolejki
        adjustFrustrationOrder(node); // Sortuj według frustracji
        addFrustration(node); // Zwiększ frustrację oczekujących
        pthread_mutex_unlock(&list_mutex);

        printf("Lotnisko: wszedł pasażer [%d]\033[0;34m\n", msgFromPassenger.passenger.id);
    }

    printf("Lotnisko się zamyka.\033[0;34m\n");
    fflush(stdout);

    // Sprzątanie zasobów
    for (int i = 0; i < 3; i++) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
        sem_destroy(&thread_ready[i]);
    }
    pthread_cancel(threadCheck);
    pthread_join(threadCheck, NULL);

    shmdt(lotniskoPid);
    shmctl(lotniskoPidID, IPC_RMID, NULL);
    shmdt(memory);
    shmctl(shmID, IPC_RMID, NULL);

    msgctl(msgLotniskoID, IPC_RMID, NULL);
    msgctl(msgID, IPC_RMID, NULL);

    pthread_mutex_destroy(&list_mutex);

    return 0;
}

// Wątek monitorujący dostępność stanowisk kontroli
void *checkSecurity(void *arg) {
    while (startKill == 0) {
        pthread_mutex_lock(&list_mutex);
        if (node != NULL) {
            assigned_thread = -1;
            for (int i = 0; i < 3; i++) {
                if (thread_busy[i] == 0) {
                    assigned_thread = i;
                    break;
                }
            }
            if (assigned_thread >= 0) {
                thread_busy[assigned_thread] = 1; // Zajmij stanowisko
                sem_post(&thread_ready[assigned_thread]); // Powiadom wątek
            }
        }
        pthread_mutex_unlock(&list_mutex);
    }
    return NULL;
}

// Główna logika kontroli bezpieczeństwa
void *securityControl(void *arg) {
    long thread_id = *((long *) arg);
    free(arg);
    while (startKill == 0) {
        sem_wait(&thread_ready[thread_id]); // Czekaj na przypisanie

        pthread_mutex_lock(&list_mutex);
        struct messagePassenger messageFirst;
        struct messagePassenger messageSecond;
        struct Node *first_passenger = node;
        if (first_passenger) {
            node = node->next;
        }

        struct Node *second_passenger = NULL;
        if (node && first_passenger->passenger->gender == node->passenger->gender) {
            second_passenger = node;
            node = node->next;
        }
        pthread_mutex_unlock(&list_mutex);

        if (first_passenger) {
            messageFirst.mtype = first_passenger->passenger->id;
            if (first_passenger->passenger->baggage_weight > memory[0]) {
                printf("Wątek %ld: Przekroczono wagę u pasażera %d (limit: %d)\n", thread_id, first_passenger->passenger->id, memory[0]);
                messageFirst.mvalue = 0;
            } else {
                messageFirst.mvalue = (first_passenger->passenger->is_equipped == 1) ? 2 : 1;
            }
            if (msgsnd(msgID, &messageFirst, sizeof(messageFirst.mvalue), 0) == -1) {
                perror("Błąd wysyłania odpowiedzi");
                exit(EXIT_FAILURE);
            }
            free(first_passenger->passenger);
            free(first_passenger);
        }

        if (second_passenger) {
            messageSecond.mtype = second_passenger->passenger->id;
            if (second_passenger->passenger->baggage_weight > memory[0]) {
                printf("Wątek %ld: Przekroczono wagę u pasażera %d\n", thread_id, second_passenger->passenger->id);
                messageSecond.mvalue = 0;
            } else {
                messageSecond.mvalue = (second_passenger->passenger->is_equipped == 1) ? 2 : 1;
            }
            if (msgsnd(msgID, &messageSecond, sizeof(messageSecond.mvalue), 0) == -1) {
                perror("Błąd wysyłania potwierdzenia");
                exit(EXIT_FAILURE);
            }
            free(second_passenger->passenger);
            free(second_passenger);
        }

        usleep(randNumber(1000, rand() % 100) + 1); // Symuluj czas kontroli

        pthread_mutex_lock(&list_mutex);
        thread_busy[thread_id] = 0; // Zwolnij stanowisko
        pthread_mutex_unlock(&list_mutex);
    }
    return NULL;
}

// Optymalizacja kolejki według poziomu frustracji
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

// System zwiększania frustracji oczekujących
void addFrustration(struct Node *head) {
    struct Node *temp = head;
    while (temp != NULL) {
        temp->passenger->frustration += randNumber(10, getpid());
        temp = temp->next;
    }
}