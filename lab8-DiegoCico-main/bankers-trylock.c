#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sched.h>

#define N_ACCOUNTS 10
#define N_THREADS  20
#define N_ROUNDS   10000
#define INIT_BALANCE 100

struct account {
    long balance;
    pthread_mutex_t mtx;
} accts[N_ACCOUNTS];

int rand_range(int N) {
    return (int)((double)rand() / ((double)RAND_MAX + 1) * N);
}

void *disburse(void *arg) {
    size_t i, from, to;
    long payment;

    for (i = 0; i < N_ROUNDS; i++) {
        from = rand_range(N_ACCOUNTS);
        do {
            to = rand_range(N_ACCOUNTS);
        } while (to == from);

        while (1) {
            pthread_mutex_lock(&accts[from].mtx);

            if (pthread_mutex_trylock(&accts[to].mtx) == 0)
                break;

            pthread_mutex_unlock(&accts[from].mtx);
            sched_yield();
        }

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
