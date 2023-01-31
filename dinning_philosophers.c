#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define __win__
#elif __unix__
    #include <pthread.h>
#else
    #error "Could not recognize your os"
#endif

#include "semaphore.h"

#define N 5  // number of philosophers and forks

#define LEFT(id) (id + N - 1) % N // left hand
#define RIGHT(id) (id + 1) % N    // right hand

#define DOWN wait  // wait semaphore rename to DOWN too
#define UP signal  // signal semaphore rename to UP too

#define THINK(id) \
    sleep(1);     \
    printf("* Philosopher %d is Thinking\n", id);
#define EAT(id) \
    sleep(2);   \
    printf("* Philosopher %d is Eating\n", id);

typedef enum {
    HUNGRY,
    EATING,
    THINKING
} State;
State states[N] = {HUNGRY}; // init all hungry

Semaphore *forks[N];
Semaphore *mutex;

void test(int id)
{
    // if he is hungry and forks for him is down
    if (states[id] == HUNGRY
    && states[LEFT(id)] != EATING
    && states[RIGHT(id)] != EATING
    ) {
        states[id] = EATING;
        UP(forks[id]);
        printf("* Philosopher %d takes fork %d and %d\n", id, LEFT(id), id);
    }
}

void take_fork (int id)
{
    wait(mutex);
    states[id] = HUNGRY;
    printf("* Philosopher %d is Hungry\n", id);
    test(id);
    signal(mutex);
    DOWN(forks[id]);
}

void put_fork (int id)
{
    wait(mutex);
    printf("* Philosopher %d put fork %d and %d\n", id, LEFT(id), id);
    states[id] = THINKING;
    THINK(id);
    test(LEFT(id));
    test(RIGHT(id));
    signal(mutex);
}

#ifdef __win__
DWORD WINAPI start(void* data)
#elif __unix__
void * start(void* data)
#endif
{
    int id = *((int*)data);
    THINK(id);
    take_fork(id);
    EAT(id);
    put_fork(id);
    return 0;
}

int main()
{
    // Init semaphores
    mutex = get_semaphore(1, "mutex");
    if (mutex == NULL) {
        fprintf(stderr, "ERROR: Could not create semaphore!");
        exit(1);
    }
    for (int i = 0 ; i < N ; i++) {
        char * name = malloc(10);
        sprintf(name, "forks %d", i);
        forks[i] = get_semaphore(0, name);
        if (forks[i] == NULL) {
            fprintf(stderr, "ERROR: Could not create semaphore!");
            exit(1);
        }
    }
    // Create threads
#ifdef __win__
    HANDLE hThreadArray[N];
    for (int i = 0 ; i < N ; i++) {
        int * id = malloc(sizeof(int));
        *id = i;  // for pass the id
        hThreadArray[i] = CreateThread(NULL, 0, start, (void*)id, 0, NULL);
        if (hThreadArray[i] == NULL) {
            fprintf(stderr, "ERROR: Could not create threads %d!", i);
            exit(1);
        }
    }
    WaitForMultipleObjects(N, hThreadArray, TRUE, INFINITE);
#elif __unix__
    pthread_t threads[N];
    for (int i = 0 ; i < N ; i++) {
        int * id = malloc(sizeof(int));
        *id = i;
        int pth = pthread_create( &(threads[i]), NULL, start, (void*)id);
        if (pth != 0) {
            fprintf(stderr, "ERROR: Could not create threads!");
            exit(1);
        }
    }
    for (int i = 0 ; i < N ; i++)
        pthread_join(threads[i], NULL);
#endif
    free(mutex);
    for (int i = 0 ; i < N ; i++) {
        free(forks[i]);
    }
    return 0;
}
