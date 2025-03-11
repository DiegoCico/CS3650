#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <assert.h>

int global = 1; 

void do_work(void *arg) {
    // get busy
    int *pi = arg;
    printf("Hello from Thread! \n");
    printf("arg = %d\n", *((int *) arg));
    printf("arg = %d\n", *pi);

    // printf(" thread hlobal = %d\n", global);
    sleep(1);
    // printf(" thread hlobal = %d\n", global);
    // global++;
    return *pi + 10;
}

int main(int arfc, char **argv) {
    // pthread_create
    //pid_t child = fork();
    pthread_t th; 

    printf("Starting thread to do work \n");
    int i = 10;
    int error = pthread_create(
        &th, // THREAD HANDLE
         NULL, // attibutes (NULL DEFAULT)
          do_work, // tread fuction
           &i); // thread function arguments
    assert(error == 0);
    assert(0 == pthread_join(th,NULL));

    printf("Hello from Thread! \n");
    printf(" thread hlobal = %d\n", global);
    global++;
    printf(" thread hlobal = %d\n", global);
    
    /* if (child == 0) {
        do_work();
        exit(0);
    }

    // in parent do some other work
    waitpid(child, NULL, 0);
    */

   printf(" thread hlobal = %d\n", global);
   printf("Work Done\n");
   return 0; 
    // child completed
    // done
}