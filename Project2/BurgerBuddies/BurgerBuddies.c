// Solve the Burger Buddies Problem
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Global Variables
int burger_num = 0;
int rack_num = 0;
int customers = 0;
int cashiers = 0;
int cooks = 0;
int served = 1;

// Semaphores
sem_t cashier;
sem_t customer;
sem_t burger;
sem_t noBurger;
sem_t mutex;

// Functions
void *customerThread(void *arg);
void *cookThread(void *arg);
void *cashierThread(void *arg);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <cooks> <cashiers> <customers> <rack_num>\n",
               argv[0]);
        return 1;
    }

    cooks = atoi(argv[1]);
    cashiers = atoi(argv[2]);
    customers = atoi(argv[3]);
    rack_num = atoi(argv[4]);

    int id_cooks[cooks];
    int id_cashiers[cashiers];
    int id_customers[customers];

    if (cooks < 1) {
        printf("Invalid number of cooks. \n");
        return 1;
    }

    if (cashiers < 1) {
        printf("Invalid number of cashiers. \n");
        return 1;
    }

    if (customers < 1) {
        printf("Invalid number of customers. \n");
        return 1;
    }

    if (rack_num < 1) {
        printf("Invalid number of rack. \n");
        return 1;
    }

    printf("Cooks[%d], Cashiers[%d], Customers[%d]\n", cooks, cashiers,
           customers);

    // Initialize Semaphores
    sem_init(&cashier, 0, 0);
    sem_init(&customer, 0, 0);
    sem_init(&burger, 0, 1);
    sem_init(&mutex, 0, 1);

    // Threads
    pthread_t customerThreads[customers];
    pthread_t cookThreads[cooks];
    pthread_t cashierThreads[cashiers];

    printf("Begin run. \n");

    // Create Cook Threads
    for (int i = 0; i < cooks; i++) {
        pthread_create(&cookThreads[i], NULL, cookThread, &id_cooks[i]);
        id_cooks[i] = i + 1;
    }

    // Create Customer Threads
    for (int i = 0; i < customers; i++) {
        pthread_create(&customerThreads[i], NULL, customerThread,
                       &id_customers[i]);
        id_customers[i] = i + 1;
    }

    // Create Cashier Threads
    for (int i = 0; i < cashiers; i++) {
        pthread_create(&cashierThreads[i], NULL, cashierThread,
                       &id_cashiers[i]);
        id_cashiers[i] = i + 1;
    }

    // Join Cook Threads
    for (int i = 0; i < cooks; i++) {
        pthread_join(cookThreads[i], NULL);
    }

    // Join Customer Threads
    for (int i = 0; i < customers; i++) {
        pthread_join(customerThreads[i], NULL);
    }

    // Join Cashier Threads
    for (int i = 0; i < cashiers; i++) {
        pthread_join(cashierThreads[i], NULL);
    }

    // Destroy Semaphores
    sem_destroy(&cashier);
    sem_destroy(&customer);
    sem_destroy(&burger);
    sem_destroy(&mutex);

    // Print Results
    printf("All customers are served! \n");

    return 0;
}

void *cookThread(void *arg) {
    int id = *(int *)arg;
    while (served < customers) {
        // cooking
        sem_wait(&mutex);
        sleep(rand() % 3 + 1);
        if (burger_num < rack_num) {
            burger_num++;
            sem_post(&burger);
        } else {
            sem_post(&mutex);
            sleep(rand() % 3 + 1);
            continue;
        }
        sem_post(&mutex);

        printf("\033[35mCook[%d] make a burger.\033[0m \n", id);
    }
    pthread_exit(0);
}

void *customerThread(void *arg) {
    int id = *(int *)arg;

    sem_post(&customer);
    sleep(rand() % 5 + 3);
    printf("\033[34mCustomer [%d] come. \033[0m\n", id);
    sem_wait(&cashier);

    pthread_exit(0);
}

void *cashierThread(void *arg) {
    int id = *(int *)arg;
    while (served < customers) {
        // sleep for the customer to come
        sem_wait(&customer);
        sem_wait(&burger);
        // taking order
        sem_wait(&mutex);
        if (burger_num > 0) {
            burger_num--;
            sleep(rand() % 2 + 1);
            served++;
            if (served >= customers) {  // release all semaphores
                sem_post(&mutex);
                sem_post(&cashier);
                sem_post(&burger);
                for (int i = 0; i < customers; i++) sem_post(&customer);
                break;
            }
        } else {
            sem_post(&mutex);
            continue;
        }
        sem_post(&mutex);

        printf("\033[31mCashier [%d] accepts an order.\033[0m \n", id);
        sem_post(&cashier);
    }
    pthread_exit(0);
}
