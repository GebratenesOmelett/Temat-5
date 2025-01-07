#include "funkcje.h"
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#define FIFO_NAME "passenger_fifo"

pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
int N = 4;
struct Node *node = NULL;
int semID;
int occupied;
sem_t thread_ready[3];
int Md;
// Wskaźnik zajętości wątku
int thread_busy[3] = {0, 0, 0};

void *securityControl(void *arg) {
    long thread_id = (long) arg;

    while (1) {
        sem_wait(&thread_ready[thread_id]); // Oczekiwanie na sygnał od głównego wątku

        pthread_mutex_lock(&list_mutex);

        // Pobranie pierwszego pasażera z listy
        struct Node *first_passenger = node;
        if (first_passenger) {
            node = node->next;
        }

        // Pobranie drugiego pasażera z listy (jeśli istnieje)
        struct Node *second_passenger = NULL;
        if (node) {
            if (first_passenger->passenger->gender == node->passenger->gender) {
                second_passenger = node;
                node = node->next;
            }
        }

        pthread_mutex_unlock(&list_mutex);

        // Przetwarzanie pasażerów
        if (first_passenger) {
            if (first_passenger->passenger->baggage_weight > Md) {
                printf("Wątek %ld: Za duży bagaż u pasażera %d\n", thread_id, first_passenger->passenger->id);
            }
            printf("Wątek %ld przetwarza pasażera %d o plci %s\n", thread_id, first_passenger->passenger->id,&first_passenger->passenger->gender);
            free(first_passenger->passenger);
            free(first_passenger);
        }

        if (second_passenger) {
            if (second_passenger->passenger->baggage_weight > Md) {
                printf("Wątek %ld: Za duży bagaż u pasażera %d", thread_id, second_passenger->passenger->id);
            }
            printf("Wątek %ld przetwarza pasażera %d\n o plci %s\n", thread_id, second_passenger->passenger->id,&first_passenger->passenger->gender);
            free(second_passenger->passenger);
            free(second_passenger);
        }

        // Symulacja czasu przetwarzania
        usleep(randNumber(1000)); // 100 ms

        // Ustawienie wątku jako gotowego
        pthread_mutex_lock(&list_mutex);
        thread_busy[thread_id] = 0;
        pthread_mutex_unlock(&list_mutex);

        sleep(randNumber(10)); // Opcjonalny dodatkowy delay
    }

    return NULL;
}


int main() {
    key_t klucz;
    pthread_t threads[3];
    Md = randNumber(100);
    printf("limit bagażu to %d", Md);
    if ((klucz = ftok(".", 'A')) == -1) {
        printf("Blad ftok (A)\n");
        exit(2);
    }
    semID = alokujSemafor(klucz, N, IPC_CREAT | 0666);
    waitSemafor(semID, 0, 0);

    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("Błąd przy otwieraniu FIFO do odczytu");
        return 1;
    }
    // Inicjalizacja semaforów dla wątków
    for (int i = 0; i < 3; i++) {
        sem_init(&thread_ready[i], 0, 0);
        pthread_create(&threads[i], NULL, securityControl, (void *) i);
    }
    // Initialize the linked list head to NULL

    while (1) {

        struct passenger p;
        ssize_t bytesRead = read(fd, &p, sizeof(struct passenger));

        // Process the passenger data
//        print_passenger(&p);

        append(&node, p);  // Append passenger to the list
        printList(node);   // Print the updated list
        int assigned_thread = -1;
        for (int i = 0; i < 3; i++) {
            if (thread_busy[i] == 0) {
                assigned_thread = i;
                break;
            }
        }

        if (assigned_thread >= 0) {
            thread_busy[assigned_thread] = 1; // Ustaw jako zajęty
            sem_post(&thread_ready[assigned_thread]); // Sygnalizuj gotowość
        } else {
            printf("Wszystkie wątki zajęte, dane oczekują w kolejce\n");
        }

    }
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    // Zamknięcie deskryptora
    close(fd);

    // Usunięcie FIFO
    unlink(FIFO_NAME);

    return 0;
}