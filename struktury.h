#include <stdbool.h>

struct passenger{
    int id;
    double baggage_weight;
    char gender;
    bool is_vip;
    int frustration;
    int peoplePass;
};

struct airplane{

};
struct thread_data {
    int id;
    int fd;  // Deskryptor pliku FIFO
};

struct Node {
    struct passenger* passenger;
    struct Node* next;
};