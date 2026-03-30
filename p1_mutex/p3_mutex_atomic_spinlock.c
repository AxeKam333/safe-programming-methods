#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#define TEST_SIZE 6
#define ITER 1000000

static pthread_barrier_t barrier;
static pthread_spinlock_t spinlock;
static int ctr;

void* increment(void* arg) {
    long int *liczba_watkow = (long int *)arg;
    pthread_barrier_wait(&barrier);
    for(long int i = 0; i < ITER / (*liczba_watkow); i++) {
        pthread_spin_lock(&spinlock);
        ctr++;
        pthread_spin_unlock(&spinlock);
    }
    return NULL;
}

double z_czas(long int liczba_watkow) {
    pthread_t watki[96];
    struct timespec start, end;

    ctr = 0;

    pthread_barrier_init(&barrier, NULL, liczba_watkow + 1);
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);

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
    pthread_spin_destroy(&spinlock);

    double time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    return ITER / time;
}

int main() {
    int krok_test[] = {1, 2, 4, 8, 16, 24, 32, 48, 64, 80, 96};
    int l_krokow = sizeof(krok_test) / sizeof(krok_test[0]);

    printf("Liczba wątków, Przepustowość [ops/s]:\n");

    for(int i = 0; i < l_krokow; i++)
        z_czas(krok_test[i]);  // rozgrzewka

    for(int w = 1; w <= 32; w++) {
        for(int j = 1; j <= TEST_SIZE; j++) {
            printf("%d, %f\n", w, z_czas(w));
        }
    }

    for(int i = 0; i < l_krokow; i++) {
        for(int j = 0; j < TEST_SIZE; j++) {
            printf("%d, %f\n", krok_test[i], z_czas(krok_test[i]));
        }
    }
    return 0;
}