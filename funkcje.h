#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/errno.h>
#include "struktury.h"
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

int randNumber(int x) {
    srand(time(NULL));
    return (rand() % x) + 1;
}
char randGender(){
    int x = (rand() % 100) + 1;
    if(x <=50){
        return 'M';
    }
    else{
        return 'F';
    }
}
bool randRare(){
    int x = (rand() % 100) + 1;
    if(x <=5){
        return true;
    }
    else{
        return false;
    }
};

void print_passenger(const struct passenger* p) {
    printf("Passenger Details:\n");
    printf("ID: %d\n", p->id);
    printf("Baggage Weight: %.2f kg\n", p->baggage_weight);
    printf("Gender: %c\n", p->gender);
    printf("VIP Status: %s\n", p->is_vip ? "Yes" : "No");
    printf("Frustration Level: %d\n", p->frustration);
    printf("People Passed: %d\n", p->peoplePass);
    printf("Equipment status: %d\n", p->is_equipped);
}
struct Node* createNode(struct passenger data) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (!newNode) {
        printf("Memory allocation error\n");
        exit(1);
    }
    newNode->passenger = (struct passenger*)malloc(sizeof(struct passenger));
    if (!newNode->passenger) {
        printf("Memory allocation error for passenger\n");
        exit(1);
    }
    *(newNode->passenger) = data;  // Copy the data
    newNode->next = NULL;
    return newNode;
}

// Funkcja do dodania węzła na początku listy
void push(struct Node** head, struct passenger data) {
    struct Node* newNode = createNode(data);
    newNode->next = *head;
    *head = newNode;
}

// Funkcja do dodania węzła na końcu listy
void append(struct Node** head, struct passenger data) {
    struct Node* newNode = createNode(data);
    if (*head == NULL) {
        *head = newNode;
        return;
    }
    struct Node* temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newNode;
}

// Funkcja do usunięcia węzła z listy
void deleteNode(struct Node** head, int key) {
    struct Node* temp = *head;
    struct Node* prev = NULL;

    if (temp != NULL && temp->passenger->id == key) {
        *head = temp->next;
        free(temp);
        return;
    }

    while (temp != NULL && temp->passenger->id != key) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        printf("Element %d nie znaleziony w liście\n", key);
        return;
    }

    prev->next = temp->next;
    free(temp);
}


void printList(struct Node* head) {
    struct Node* temp = head;
    while (temp != NULL) {
        printf("ID: %d, Baggage Weight: %.2f, Gender: %c, VIP: %s, Frustration: %d, People Pass: %d, Equipped: %s\n",
               temp->passenger->id, temp->passenger->baggage_weight, temp->passenger->gender,
               temp->passenger->is_vip ? "Yes" : "No", temp->passenger->frustration, temp->passenger->peoplePass,
               temp->passenger->is_equipped ? "Yes" : "No");
        temp = temp->next;
    }
    printf("NULL\n");
}

void freeList(struct Node** head) {
    struct Node* temp = *head;
    while (temp != NULL) {
        struct Node* next = temp->next;
        free(temp->passenger);  // Free passenger memory
        free(temp);  // Free node memory
        temp = next;
    }
    *head = NULL;
}
int alokujSemafor(key_t klucz, int number, int flagi)
{
    int semID;
    if ( (semID = semget(klucz, number, flagi)) == -1)
    {
        perror("Blad semget (alokujSemafor): ");
        exit(1);
    }
    return semID;
}

int zwolnijSemafor(int semID, int number)
{
    return semctl(semID, number, IPC_RMID, NULL);
}

void inicjalizujSemafor(int semID, int number, int val)
{

    if ( semctl(semID, number, SETVAL, val) == -1 )
    {
        perror("Blad semctl (inicjalizujSemafor): ");
        exit(1);
    }
}

int waitSemafor(int semID, int number, int flags)
{
    int result;
    struct sembuf operacje[1];
    operacje[0].sem_num = number;
    operacje[0].sem_op = -1;
    operacje[0].sem_flg = 0 | flags;//SEM_UNDO;

    if ( semop(semID, operacje, 1) == -1 )
    {
        //perror("Blad semop (waitSemafor)");
        return -1;
    }

    return 1;
}

void signalSemafor(int semID, int number)
{
    struct sembuf operacje[1];
    operacje[0].sem_num = number;
    operacje[0].sem_op = 1;
    //operacje[0].sem_flg = SEM_UNDO;

    if (semop(semID, operacje, 1) == -1 )
        perror("Blad semop (postSemafor): ");

    return;
}
void createNewFifo(const char *fifoName){
    struct stat stats;
    if ( stat( fifoName, &stats ) < 0 )
    {
        if ( errno != ENOENT )          // ENOENT is ok, since we intend to delete the file anyways
        {
            perror( "stat failed" );    // any other error is a problem
            return;
        }
    }
    else                                // stat succeeded, so the file exists
    {
        if ( unlink( fifoName ) < 0 )   // attempt to delete the file
        {
            perror( "unlink failed" );  // the most likely error is EBUSY, indicating that some other process is using the file
            return;
        }
    }
    if (mkfifo(fifoName, 0666) == -1) {
        perror("mkfifo");
    }
}

