#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define ITER 10000000

static int ctr = 0;
pthread_mutex_t L = PTHREAD_MUTEX_INITIALIZER;

void* increment(void* arg) {
    for(int i = 0; i < ITER; i++) {
        pthread_mutex_lock(&L);   // L.acquire()
        ctr++;                    // Sekcja krytyczna
        pthread_mutex_unlock(&L); // L.release()
    }
    return NULL;
}

double z_czas(int liczba_watkow) {
    pthread_t watki[liczba_watkow];
    struct timespec start, end;
    
    // Reset licznika przed każdym testem
    ctr = 0; 
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i = 0; i < liczba_watkow; i++) {
        pthread_create(&watki[i], NULL, increment, NULL);
    }
    for(int i = 0; i < liczba_watkow; i++) {
        pthread_join(watki[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    printf("Test przepustowosci...\n");
    printf("Liczba wątków, Czas wykonania [s]: \n");
    
    // Testujemy przepustowość dla 1, 2, 4 i 8 wątków
    for(int w = 1; w <= 32; w++) {
        printf("%d, %f\n", w, z_czas(w));
    }
    
    return 0;
}