#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

void do_work(void *args) {
    // get busy
    printf("Hello from Thread! \n");
    return NULL;
}

int main(int arfc, char **argv) {
    // pthread_create
    //pid_t child = fork();
    pthread_t th; 
    int error = pthread_create(&th, NULL, do_work, NULL);
    
    /* if (child == 0) {
        do_work();
        exit(0);
    }

    // in parent do some other work
    waitpid(child, NULL, 0);
    */

   pthread_join(th, NULL);
    // child completed
    // done
}