#include <stdio.h>
#include "struktury.h"
#include <fcntl.h>
#define FIFO_NAME "passenger_fifo"
int main() {
    // Otworzenie FIFO do odczytu
    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("Błąd przy otwieraniu FIFO do odczytu");
        return 1;
    }

    struct passenger p;
    while (1) {
        // Odbieranie pasażera z FIFO
        if (read(fd, &p, sizeof(struct passenger)) == -1) {
            perror("Błąd przy odczycie z FIFO");
            break;
        }
        printf("Odebrano pasażera z FIFO:\n");
        printf("ID: %d\n", p.id);
        printf("Baggage Weight: %.2f kg\n", p.baggage_weight);
        printf("VIP %d\n", p.is_vip);
        printf("Gender: %c\n", p.gender);
        printf("Frustration Level: %d\n", p.frustration);
        printf("People Passed: %d\n", p.peoplePass);
    }

    // Zamknięcie deskryptora
    close(fd);

    // Usunięcie FIFO
    unlink(FIFO_NAME);

    return 0;
}