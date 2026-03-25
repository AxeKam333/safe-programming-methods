#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define ITER 1000000

static int ctr = 0;
pthread_mutex_t L = PTHREAD_MUTEX_INITIALIZER;

void* increment(void* arg) {
    for(int i = 0; i < ITER; i++) {
        pthread_mutex_lock(&L);   
        ctr++;                    
        pthread_mutex_unlock(&L); 
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
    int krok_test[] = {1, 2, 4, 8, 16, 24, 32, 48, 64, 80, 96};
    int l_krokow = sizeof(krok_test) / sizeof(krok_test[0]);
    printf("Test przepustowosci...\n");
    printf("Liczba wątków, Czas wykonania [s]: \n");

    for(int i = 0; i < l_krokow; i++) {
        int w = krok_test[i];
            z_czas(w);
    }

    for(int i = 0; i < l_krokow; i++) {
        for(int j = 1; j <= 6; j++) {
            int w = krok_test[i];
            printf("%d, %f\n", w, z_czas(w));
        }
    }

    return 0;
}
