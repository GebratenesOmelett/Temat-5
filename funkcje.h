#include <stdlib.h>
#include <stdio.h>
#include "struktury.h"

int randNumber(int x) {
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
bool randVip(){
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
}
struct Node* createNode(struct passenger* data) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (!newNode) {
        printf("Błąd alokacji pamięci\n");
        exit(1);
    }
    newNode->passenger = data;
    newNode->next = NULL;
    return newNode;
}

// Funkcja do dodania węzła na początku listy
void push(struct Node** head, struct passenger data) {
    struct Node* newNode = createNode(&data);
    newNode->next = *head;
    *head = newNode;
}

// Funkcja do dodania węzła na końcu listy
void append(struct Node** head, struct passenger data) {
    struct Node* newNode = createNode(&data);
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

// Funkcja do zamiany miejscami dwóch węzłów na podstawie ich wartości
void swapNodes(struct Node** head, int x, int y) {


    struct Node *prevX = NULL, *currX = *head;
    struct Node *prevY = NULL, *currY = *head;

    // Znajdź węzeł `x` i jego poprzednika
    while (currX && currX->passenger->id != x) {
        prevX = currX;
        currX = currX->next;
    }

    // Znajdź węzeł `y` i jego poprzednika
    while (currY && currY->passenger->id != y) {
        prevY = currY;
        currY = currY->next;
    }

    // Jeśli `x` lub `y` nie istnieje, zakończ
    if (!currX || !currY) {
        printf("Jeden z elementów (%d, %d) nie istnieje w liście.\n", x, y);
        return;
    }

    // Jeśli `prevX` nie jest NULL, zaktualizuj wskaźnik
    if (prevX) {
        prevX->next = currY;
    } else {
        *head = currY;
    }

    // Jeśli `prevY` nie jest NULL, zaktualizuj wskaźnik
    if (prevY) {
        prevY->next = currX;
    } else {
        *head = currX;
    }

    // Zamień wskaźniki `next`
    struct Node* temp = currY->next;
    currY->next = currX->next;
    currX->next = temp;

    printf("Zamieniono elementy %d i %d.\n", x, y);
}
void printList(struct Node* head) {
    struct Node* temp = head;
    while (temp != NULL) {
        printf("ID: %d, Baggage Weight: %.2f, Gender: %c, VIP: %s, Frustration: %d, People Pass: %d\n",
               temp->passenger->id, temp->passenger->baggage_weight, temp->passenger->gender,
               temp->passenger->is_vip ? "Yes" : "No", temp->passenger->frustration, temp->passenger->peoplePass);
        temp = temp->next;
    }
    printf("NULL\n");
}

// Funkcja do zwolnienia pamięci zajmowanej przez listę
void freeList(struct Node** head) {
    struct Node* temp = *head;
    while (temp != NULL) {
        struct Node* next = temp->next;
        free(temp);
        temp = next;
    }
    *head = NULL;
}
