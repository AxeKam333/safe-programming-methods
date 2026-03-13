#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h> 
#include <sys/mman.h>

#define ITER 10000000

static atomic_int * ctr;
pthread_mutex_t L = PTHREAD_MUTEX_INITIALIZER;

void* increment() {
    for(int i = 0; i < ITER; i++) {
        (*ctr)++;
    }
    return NULL;
}

double z_czas(int liczba_watkow) {

    int myPid;
    struct timespec start, end;
    
    // Alokacja pamięci współdzielonej 
    ctr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (ctr == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    *ctr = 0; 
    
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i<liczba_watkow-1; i++){
        myPid = fork();
    // printf("My PID: %d\n", myPid);
        if (myPid == 0) {
            break;
        }
    }

    increment();

    // sprawdzenie czy wynik sumowania jest poprawny
    // printf("%d\n",*ctr);

    if(myPid == 0){
        exit(0);
    }
    while ((myPid = wait(NULL)) > 0);

    clock_gettime(CLOCK_MONOTONIC, &end);
    munmap(ctr,sizeof(int));
    
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    printf("Test przepustowosci...\n");
    printf("Liczba procesów, Czas wykonania [s]: \n");
    
    // Testujemy przepustowość dla 1 do 8 wątków
    for(int w = 1; w <= 32; w++) {
        printf("%d, %f\n", w, z_czas(w));
    }

        // printf("Czas: %d, %f\n", 1, z_czas(8));
    return 0;
}