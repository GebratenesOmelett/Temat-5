#include <stdlib.h>
#include <stdbool.h>

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
