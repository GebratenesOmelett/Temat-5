#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pid = fork();
    if (pid == 0) {
        // Proces potomny uruchamia program `pasazer`
        execl("./pasazer", "pasazer", NULL);
        perror("Nie udało się uruchomić programu pasazer");
        return 1;
    } else if (pid > 0) {
        // Proces rodzic czeka na zakończenie potomka
        printf("Program pasazer zakończył działanie.\n");
    } else {
        perror("fork");
        return 1;
    }
    pid = fork();
    if (pid == 0) {
        // Proces potomny uruchamia program `pasazer`
        execl("./lotnisko", "pasazer", NULL);
        perror("Nie udało się uruchomić programu pasazer");
        return 1;
    } else if (pid > 0) {
        // Proces rodzic czeka na zakończenie potomka
        wait(NULL);
        printf("Program pasazer zakończył działanie.\n");
    } else {
        perror("fork");
        return 1;
    }
    return 0;
}