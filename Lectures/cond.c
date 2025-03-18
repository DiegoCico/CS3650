
#include <assert.h>

// Locks(mutex)
// - initialized - unlocked
// - lock - acquire the lock - putting the thread to sleep if the lock is not available 
// - unlock - release the lock - signaling to other thread waiting for the lock that it is aailable
// - (trylock)
//
// Condition
// - initialize
// - signal - signal a threat that a condition was met
// - wait - wait fror a condition to be met

int counter = 0;
int update = 0;

void count(int n) {
    for (int i = 0; i < n; i++) {
        counter++;
        usleep(20000);
    }
}

void *count_workervoid (void *arg) {

    while(1) {
        count(10);
        update = 1;
    }

    return NULL;
}

void *watcher(void *arg) {
    while (1) {
        if (update) {
            printf("counter = %d\n", counter);
            update = 0;
        }
    }
}