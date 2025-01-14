#include <stdbool.h>

struct passenger{
    int id;
    double baggage_weight;
    char gender;
    bool is_vip;
    bool is_equipped;
    int frustration;
    int peoplePass;
    int airplaneNumber;
};

struct airplane{
    int Md;
    int amountOfPassengers;

};
struct thread_data {
    int id;
    int fd;  // Deskryptor pliku FIFO
};

struct Node {
    struct passenger* passenger;
    struct Node* next;
};

struct messagePassenger {
    long mtype;       // Typ komunikatu
    int mvalue;  // Treść komunikatu
};
struct depoPassenger {
    long mtype;       // Typ komunikatu (musi być typu `long`)
    int data;         // Dane komunikatu (w tym przypadku int)
};
