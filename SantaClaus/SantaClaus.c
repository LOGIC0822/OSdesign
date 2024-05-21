// Solve the Santa claus problem with 9 reindeers and 10 elves and awaken by all
// 9 reindeer or 3 elves
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 30

// Global Variables
int reindeer = 0, elves = 0, elf_counter = 0;
int reindeer_waiting = 0, elves_waiting = 0;
int steps = 0;

// Semaphores
sem_t santa_sem, reindeer_sem, elves_sem;

// Mutexes
pthread_mutex_t reindeer_mutex, elves_mutex;

// Functions
void prepareSleigh() {
    printf("Santa Claus is preparing the sleigh.\n");
    sleep(1);
}

void getHitched(int id) {
    printf("Reindeer %d is getting hitched to the sleigh.\n", id);
    sleep(1);
}

void helpElves() {
    printf("Santa Claus is helping the elves.\n");
    sleep(1);
}

void getHelp(int id) {
    printf("Elf %d is getting help from Santa Claus.\n", id);
    sleep(1);
}

void wait_elf(int id) {
    printf("Elf %d is waiting for Santa Claus.\n", id);
    sleep(1);
}

void wait_reindeer(int id) {
    printf("Reindeer %d is waiting for Santa Claus.\n", id);
    sleep(1);
}

void wake_up_elves_waiting() {
    for (int i = 0; i < elves_waiting; i++) {
        sem_post(&elves_sem);
    }
    elves_waiting = 0;
    sleep(1);
}

// Functions
void *santaClaus(void *arg) {
    while (steps < N) {
        sem_wait(&santa_sem);
        pthread_mutex_lock(&reindeer_mutex);
        if (reindeer == 9) {
            prepareSleigh();
            steps++;
            for (int i = 0; i < 9; i++) {
                sem_post(&reindeer_sem);
            }
            reindeer = 0;
        }
        pthread_mutex_unlock(&reindeer_mutex);
        pthread_mutex_lock(&elves_mutex);
        if (elves == 3) {
            helpElves();
            for (int i = 0; i < 3; i++) {
                sem_post(&elves_sem);
            }
            elves = 0;
        }
        pthread_mutex_unlock(&elves_mutex);
    }
    pthread_exit(0);
}

void *Reindeer(void *arg) {
    int id = *(int *)arg;
    while (steps < N) {
        pthread_mutex_lock(&reindeer_mutex);
        if (reindeer >= 9) {  // To control the number of reindeer
            pthread_mutex_unlock(&reindeer_mutex);
            sleep(1);
            continue;
        }
        reindeer++;
        if (reindeer == 9) {
            sem_post(&santa_sem);
        } else {
            reindeer_waiting++;
        }
        pthread_mutex_unlock(&reindeer_mutex);
        wait_reindeer(id);
        sem_wait(&reindeer_sem);
        getHitched(id);
    }
    pthread_exit(0);
}

void *Elf(void *arg) {
    int id = *(int *)arg;
    while (steps < N) {
        pthread_mutex_lock(&elves_mutex);
        if (elves >= 10) {  // To control the number of elves
            pthread_mutex_unlock(&elves_mutex);
            sleep(1);
            continue;
        }
        if (elves == 3 || elf_counter == 3) {
            elves_waiting++;
            pthread_mutex_unlock(&elves_mutex);
            wait_elf(id);
        } else {
            elves++;
            elf_counter++;
            if (elves == 3) {
                sem_post(&santa_sem);
            }
            pthread_mutex_unlock(&elves_mutex);
            sem_wait(&elves_sem);
            getHelp(id);
            pthread_mutex_lock(&elves_mutex);
            elf_counter--;
            if (elf_counter == 0) {
                wake_up_elves_waiting();
            }
            pthread_mutex_unlock(&elves_mutex);
        }
    }
    pthread_exit(0);
}

int main() {
    // initialize semaphores and mutexes
    sem_init(&santa_sem, 0, 0);
    sem_init(&reindeer_sem, 0, 0);
    sem_init(&elves_sem, 0, 0);
    pthread_mutex_init(&reindeer_mutex, NULL);
    pthread_mutex_init(&elves_mutex, NULL);

    // create threads
    pthread_t santa, reindeers[9], elves[10];

    int id_deer[9], id_elf[10];
    if (pthread_create(&santa, NULL, santaClaus, NULL)) {
        printf("Error: error in pthread_create() \n");
        exit(-1);
    }
    for (int i = 0; i < 9; i++) {
        id_deer[i] = i + 1;
        if (pthread_create(&reindeers[i], NULL, Reindeer, &id_deer[i])) {
            printf("Error: error in pthread_create() \n");
            exit(-1);
        }
    }
    for (int i = 0; i < 10; i++) {
        id_elf[i] = i + 1;
        if (pthread_create(&elves[i], NULL, Elf, &id_elf[i])) {
            printf("Error: error in pthread_create() \n");
            exit(-1);
        }
    }

    // wait for threads to finish
    pthread_join(santa, NULL);
    for (int i = 0; i < 9; i++) {
        pthread_join(reindeers[i], NULL);
    }
    for (int i = 0; i < 10; i++) {
        pthread_join(elves[i], NULL);
    }

    printf("Christmas is over!\n");
    return 0;
}