#include <stdio.h>
#include <string.h>
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


Semaphore * full_mutex;
Semaphore * empty_mutex;

#define BUFFER_CAPACITY 2
const char * buffer[BUFFER_CAPACITY];
int index_end = -1;
int index_front = -1;

void write_in_buffer(const char * str)
{
    index_end = (index_end + 1) % BUFFER_CAPACITY;
    buffer[index_end] = str;
    fprintf(stdout, "* producer write in index %d buffer: %s\n", index_end, str);
}

#define NUMBER_OF_DATA 20

#ifdef __win__
DWORD WINAPI producer_thread(void* data)
#elif __unix__
void * producer_thread(void* data)
#endif
{
    const unsigned int msg_size = 20;
    char * msg;
    for (int i = 0; i < NUMBER_OF_DATA ; i++) {
        // produce msg
        msg = malloc(msg_size);
        memset(msg, 0, msg_size);
        sprintf(msg, "Producer %d", i);
        // put in buffer
        wait(empty_mutex);
        write_in_buffer(msg);
        signal(full_mutex);
    }
    return 0;
}

void read_in_buffer()
{
    index_front = (index_front + 1) % BUFFER_CAPACITY;
    const char * str = buffer[index_front];
    buffer[index_front] = NULL;
    printf("* Consumer read from index %d buffer: %s\n", index_front, str);
}

#ifdef __win__
DWORD WINAPI consumer_thread(void* data)
#elif __unix__
void * consumer_thread(void* data)
#endif
{
    for (int i = 0; i < NUMBER_OF_DATA ; i++) {
        wait(full_mutex);
        // pop from buffer
        read_in_buffer();
        signal(empty_mutex);
    }
    return 0;
}


int main()
{
    // Init semaphores
    full_mutex = get_semaphore(0, "full_mutex");
    empty_mutex = get_semaphore(BUFFER_CAPACITY, "empty_mutex");
    if (full_mutex == NULL || empty_mutex == NULL) {
        fprintf(stderr, "ERROR: Could not create semaphore!");
        exit(1);
    }
    // Create threads
#ifdef __win__
    HANDLE hThreadArray[4];
    hThreadArray[0] = CreateThread(NULL, 0, producer_thread, NULL, 0, NULL);  // producer
    hThreadArray[1] = CreateThread(NULL, 0, consumer_thread, NULL, 0, NULL);  // consumer
    if (hThreadArray[0] == NULL || hThreadArray[1] == NULL) {
        fprintf(stderr, "ERROR: Could not create threads!");
        exit(1);
    }
    WaitForMultipleObjects(2, hThreadArray, TRUE, INFINITE);
#elif __unix__
    pthread_t threads[2];
    int pth1 = pthread_create( &(threads[0]), NULL, producer_thread, NULL);  // producer
    int pth2 = pthread_create( &(threads[1]), NULL, consumer_thread, NULL);  // consumer
    if ((pth1 & pth2) != 0) {
        fprintf(stderr, "ERROR: Could not create threads!");
        exit(1);
    }

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL); 
#endif
    free(full_mutex);
    free(empty_mutex);
    return 0;
}
