#include "funkcje.h"
#include <fcntl.h>

#define FIFO_NAME "passenger_fifo"
int main() {
    // Otworzenie FIFO do odczytu
    struct passenger p;
    struct Node* node;
    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("Błąd przy otwieraniu FIFO do odczytu");
        return 1;
    }
    if (read(fd, &p, sizeof(struct passenger)) == -1) {
        perror("Błąd przy odczycie z FIFO");
    }
    node = createNode(&p);

    while (1) {
        struct passenger p;
        if (read(fd, &p, sizeof(struct passenger)) == -1) {
            perror("Błąd przy odczycie z FIFO");
            break;
        }
        print_passenger(&p);
        append(&node, p);
        printList(node);
    }

    // Zamknięcie deskryptora
    close(fd);

    // Usunięcie FIFO
    unlink(FIFO_NAME);

    return 0;
}