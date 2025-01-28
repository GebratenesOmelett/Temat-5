#ifndef STRUKTURY_H
#define STRUKTURY_H

#endif //STRUKTURY_H
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



struct Node {
    struct passenger* passenger;
    struct Node* next;
};

struct messagePassenger {
    long mtype;       // Typ komunikatu
    int mvalue;  // Treść komunikatu
};
struct messagePassengerAirport {
    long mtype;                  // Typ komunikatu
    struct passenger passenger; // Dane pasażera
};

