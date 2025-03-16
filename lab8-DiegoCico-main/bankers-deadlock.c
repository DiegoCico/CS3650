#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>

#define N_ACCOUNTS 10
#define N_THREADS  20
#define N_ROUNDS   10000
#define INIT_BALANCE 100

struct account {
    long balance;
    pthread_mutex_t mtx;
} accts[N_ACCOUNTS];

int rand_range(int N) {
    return (int)(rand() / (RAND_MAX + 1) * N);
}

void *disburse(void *arg) {
    size_t i, from, to;
    long payment;

    for (i = 0; i < N_ROUNDS; i++) {
        from = rand_range(N_ACCOUNTS);
        do {
            to = rand_range(N_ACCOUNTS);
        } while (to == from);

        pthread_mutex_lock(&accts[from].mtx);
        pthread_mutex_lock(&accts[to].mtx);

        if (accts[from].balance > 0) {
            payment = 1 + rand_range(accts[from].balance);
            accts[from].balance -= payment;
            accts[to].balance += payment;
        }

        pthread_mutex_unlock(&accts[to].mtx);
        pthread_mutex_unlock(&accts[from].mtx);
    }
    return NULL;
}

int main(void) {
    size_t i;
    pthread_t bankers[N_THREADS];

    srand(time(NULL));

    for (i = 0; i < N_ACCOUNTS; i++) {
        accts[i].balance = INIT_BALANCE;
        pthread_mutex_init(&accts[i].mtx, NULL);
    }

    for (i = 0; i < N_THREADS; i++) {
        pthread_create(&bankers[i], NULL, disburse, NULL);
    }

    for (i = 0; i < N_THREADS; i++) {
        pthread_join(bankers[i], NULL);
    }

    return 0;
}
