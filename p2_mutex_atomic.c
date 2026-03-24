#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>
#include <stdlib.h>

#define TEST_SIZE 6
#define ITER 100000000

static pthread_barrier_t barrier;
volatile atomic_int ctr;
// pthread_mutex_t L = PTHREAD_MUTEX_INITIALIZER;

void* increment(void* arg) {
    long int * liczba_watkow = (long int *)arg;
    pthread_barrier_wait(&barrier);

    for(long int i = 0; i < ITER/(*liczba_watkow); i++) {
        ctr++;
    }
    return NULL;
}

double z_czas(long int liczba_watkow) {
    pthread_t watki[96];
    struct timespec start, end;
    
    // Reset licznika przed każdym testem
    ctr = 0; 
    
    int binit = pthread_barrier_init(&barrier, NULL, liczba_watkow + 1);
    if (binit != 0)
        perror("pthread_barrier_init");

    for(long int i = 0; i < liczba_watkow; i++) {
        pthread_create(&watki[i], NULL, increment, &liczba_watkow);
    }

    pthread_barrier_wait(&barrier);
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(long int i = 0; i < liczba_watkow; i++) {
        pthread_join(watki[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    pthread_barrier_destroy(&barrier);
    
    double time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / (1e9);
    return (ITER)/time;
}

int main() {
    int krok_test[] = {1, 2, 4, 8, 16, 24, 32, 48, 64, 80, 96};
    int l_krokow = sizeof(krok_test) / sizeof(krok_test[0]);
    printf("Test przepustowosci...\n");
    printf("Liczba wątków, Przepustowość [Mops/s]: \n");
    
    // rozgrzewka
    for(int i = 0; i < l_krokow; i++) {
        int w = krok_test[i];
        z_czas(w);
    }

    for(int i = 0; i < l_krokow; i++) {
        for(int j = 1; j <= TEST_SIZE; j++) {
            int w = krok_test[i];
            printf("%d, %f\n", w, z_czas(w));
        }
    }

    for(int w = 1; w <= 32; w++) {
        for(int j = 1; j <= TEST_SIZE; j++) {
            printf("%d, %f\n", w, z_czas(w));
        }
    }

    return 0;
}
