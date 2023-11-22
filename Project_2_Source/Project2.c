#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h> 

sem_t rooms;
sem_t check_in_line;
sem_t check_out_line;
sem_t greeting;
sem_t give_key;
sem_t farewell;
sem_t give_reciept;
int room_status[3] = {-1,-1,-1}; // keeps track of guest in room
int pool = 0;                    // number of guests who used the pool
int restaurant = 0;              // number of guests who ate in the restaurant
int fitness = 0;                 // number of guests who used the fitness center
int business = 0;                // number of guests who used the business center

int in_guest = -1;               // global variable only modified in the mutially exclusive block of code used to check in to track guest
int out_guest = -1;              // global variable only modified in the mutually exclusive block of code used to check out to track guest

void do_activity(int guest){
    srand((unsigned int)time(NULL)); // seeding random to have more random results
    int activity = rand() % 4;
    switch (activity){
        case 0:
            pool = pool + 1;
            printf("Guest %d goes to the swimming pool\n", guest);
            break;
        case 1:
            restaurant = restaurant + 1;
            printf("Guest %d goes to the restaurant\n", guest);
            break;
        case 2:
            fitness = fitness + 1;
            printf("Guest %d goes to the fitness center\n", guest);
            break;
        case 3:
            business = business + 1;
            printf("Guest %d goes to the business center\n", guest);
            break;
    }
    int duration = (rand() % 3) + 1;
    sleep(duration);
}

void* guest(void* index){
    // 3 blocks
    // 1 is check_in
    sem_wait(&rooms);
    int guest = * (int *) index;
    printf("Guest %d enters the hotel\n", guest);

    // Wait for reservationist to be available
    sem_wait(&check_in_line);
    printf("Guest %d goes to the check-in reservationist\n", guest);
    in_guest = guest;
    sem_post(&greeting);

    sem_wait(&give_key);
    in_guest = -1;
    
    int room_num = -1;
    for(int i = 0; i < 3; i++){
        if(room_status[i] == guest){
            room_num = i;
        }
    }
    printf("Guest %d recieves Room %d and completes check-in\n", guest, room_num);
    sem_post(&check_in_line);

    // 2 is activity
    // pick activity and sleep
    do_activity(guest);

    // 3 is check_out
    sem_wait(&check_out_line);
    printf("Guest %d goes to the check-out reservationist and returns room %d\n", guest, room_num);
    out_guest = guest;
    sem_post(&farewell);
    
    sem_wait(&give_reciept);
    out_guest = -1;
    sem_post(&check_out_line);
    printf("Guest %d recieves the reciept\n", guest);
    free(index);
    // call signal on rooms for checking out
    sem_post(&rooms);
}

void* check_in(void* arg){
    while(1){
        // wait for guest to come up
        sem_wait(&greeting);
        int guest = in_guest;
        printf("The check-in reservationist greets Guest %d\n", guest);
        int index = 0;
        for(index; index < 3; index++){
            if(room_status[index] == -1){
                printf("Check-in reservationist assigns room %d to Guest %d\n", index, guest);
                room_status[index] = guest;
                break;
            }
        }
        sem_post(&give_key);
    }

}

void* check_out(void* arg){
    while(1){
        sem_wait(&farewell);
        int guest = out_guest;
        int room_num = -1;
        for(int i = 0; i < 3; i++){
            if(room_status[i] == guest){
                room_num = i;
                room_status[i] = -1;
                break;
            }
        }
        printf("The check-out reservationist greets Guest %d and recieves the key from room %d\n", guest, room_num);
        printf("The reciept was printed\n");
        sem_post(&give_reciept);
    }

}


int main(){

    // used to track how many rooms are available
    sem_init(&rooms, 0, 3);

    // used to provide mutual exclusion on check-in
    sem_init(&check_in_line, 0, 1);
    
    // used to provide mutual exclusion on check-out
    sem_init(&check_out_line, 0, 1);
    
    // used for event ordering of initial greeting
    sem_init(&greeting, 0, 0);
    
    // used for event ordering of giving the key
    sem_init(&give_key, 0, 0);
    
    // used for evnet ordering of farewell greeting
    sem_init(&farewell, 0, 0);
    
    // used for event ordering of giving the reciept
    sem_init(&give_reciept, 0, 0);

    pthread_t in_thread, out_thread;
    pthread_create(&in_thread, NULL, check_in, NULL);
    pthread_create(&out_thread, NULL, check_out, NULL);

    pthread_t threads[5];
    for(int i = 0; i < 5; i++){
        int* index = (int*)malloc(sizeof(int));
        *index = i;
        pthread_create(&threads[i], NULL, guest, (void*) index);
    }
    
    for(int i = 0; i<5; i++){
        pthread_join(threads[i], NULL);
    }
    
    printf("Total Guests: 5\nPool: %d\nRestaurant: %d\nFitness Center: %d\nBusiness Center: %d\n", pool, restaurant, fitness, business);
}


