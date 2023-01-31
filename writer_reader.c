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

Semaphore *mutex;          // mutex for control all writer
Semaphore *wmutex;          // mutex for control all writer
Semaphore *rmutex;          // mutex for control reader
Semaphore *rresource_mutex; // mutex for control counter reader
Semaphore *wresource_mutex; // mutex for control counter writer

FILE *writer_stream;
FILE *reader_stream;

size_t count_writer = 0;
size_t count_reader = 0;
const unsigned int msg_size = 10;

unsigned int reader_index = 0;
unsigned int writer_index = 0;

void write_in_file(const char *str)
{
    fseek(writer_stream, 0, SEEK_END);
    fwrite(str, msg_size, sizeof(char), writer_stream);
    printf("* write in file: %s", str);
}

#ifdef __win__
DWORD WINAPI writer_thread(void* data)
#elif __unix__
void * writer_thread(void* data)
#endif
{
    char * msg;
    while (writer_index < 10) {
        wait(wresource_mutex);
        count_writer++;
        if (count_writer == 1)
            wait(rmutex);
        signal(wresource_mutex);
        wait(wmutex);
        // produce msg
        msg = malloc(msg_size);
        memset(msg, 0, msg_size);
        sprintf(msg, "Producer %d\n", writer_index++);
        // write
        if(writer_index <= 10)
            write_in_file(msg);
        // exit
        signal(wmutex);
        wait(wresource_mutex);
        count_writer--;
        if (count_writer == 0)
            signal(rmutex);
        signal(wresource_mutex);
    }
    return 0;
}

void read_from_file()
{
    char buff[msg_size];
    if(fread(buff, msg_size, sizeof(char), reader_stream) == 0){
        fprintf(stderr, "ERROR: Could not read from file\n");
        return;
    }
    printf("* read from file: %s\n", buff);
}

#ifdef __win__
DWORD WINAPI reader_thread(void* data)
#elif __unix__
void * reader_thread(void* data)
#endif
{
    while (reader_index < 9) {
        // enter
        wait(rresource_mutex);
        wait(rmutex);
        count_reader++;
        if (count_reader == 1)
            wait(wmutex);
        signal(rmutex);
        signal(rresource_mutex);
        // read
        read_from_file();
        reader_index++;
        // exit
        wait(rresource_mutex);
        count_reader--;
        if (count_reader == 0)
            signal(wmutex);
        signal(rresource_mutex);
    }
    return 0;
}


int main() {
    // Init mutexes
    wmutex = get_semaphore(1, "wmutex");
    rmutex = get_semaphore(1, "rmutex");
    rresource_mutex = get_semaphore(1, "rresource_mutex");
    wresource_mutex = get_semaphore(1, "wresource_mutex");
    if (wmutex == NULL || rmutex == NULL
        || rresource_mutex == NULL || wresource_mutex == NULL
    ) {
        fprintf(stderr, "ERROR: Could not create semaphore!");
        exit(1);
    }
    // Open file
    const char * filename = "example";
    writer_stream = fopen(filename, "w");
    reader_stream = fopen(filename, "r");
    if (reader_stream == NULL) {
        fprintf(stderr, "ERROR: could not open the file %s as reader!", filename);
        exit(1);
    }
    if (writer_stream == NULL) {
        fprintf(stderr, "ERROR: could not open the file %s as writer!", filename);
        exit(1);
    }
    // Create threads if os is Windows
#ifdef __win__
    HANDLE hThreadArray[3];
    hThreadArray[0] = CreateThread(NULL, 0, writer_thread, NULL, 0, NULL);  // writer1
    hThreadArray[1] = CreateThread(NULL, 0, writer_thread, NULL, 0, NULL);  // writer2
    hThreadArray[2] = CreateThread(NULL, 0, reader_thread, NULL, 0, NULL);  // reader
    if (hThreadArray[0] == NULL || hThreadArray[1] == NULL || hThreadArray[2] == NULL) {
        fprintf(stderr, "ERROR: Could not create threads!\n");
        exit(1);
    }
    WaitForMultipleObjects(3, hThreadArray, TRUE, INFINITE);
    // Create threads if os is Unix
#elif __unix__
    pthread_t threads[3];
    int pth1 = pthread_create(&(threads[0]), NULL, writer_thread, NULL);  // writer1
    int pth2 = pthread_create(&(threads[1]), NULL, writer_thread, NULL);  // writer2
    int pth3 = pthread_create(&(threads[2]), NULL, reader_thread, NULL);  // reader
    if ((pth1 != 0) | (pth2 != 0) | (pth3 != 0)) {
        fprintf(stderr, "ERROR: Could not create threads!\n");
        exit(1);
    }
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL); 
    pthread_join(threads[2], NULL); 
#endif
    // Close file
    if ((fclose(reader_stream) & fclose(writer_stream)) != 0) {
        fprintf(stderr, "ERROR: Could not close file!");
        exit(1);
    }
    free(wmutex);
    free(rmutex);
    free(rresource_mutex);
    free(wresource_mutex);
    return 0;
}
