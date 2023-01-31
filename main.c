#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#elif __unix__
// TODO: import thread
#else
#error "Could not recognize your os"
#endif

#include "semaphore.h"

Semaphore * sem;
FILE * file;

void critical(const char * str) {
    fprintf(file, "%s\n", str);
}

DWORD WINAPI ThreadFunc(void* data) {
    char * msg = (char *) data;
    for (int i = 0; i < 10 ; i++) {
        wait(sem);
        critical(msg);
        signal(sem);
    }
}

int main() {
    sem = get_semaphore(1, "sem");
    if (sem == NULL) {
        fprintf(stderr, "ERROR: Could not create semaphore!");
        exit(1);
    }
    file = fopen("report", "w");
    HANDLE hThreadArray[2];
    hThreadArray[0] = CreateThread(NULL, 0, ThreadFunc, "PING", 0, NULL);
    hThreadArray[1] = CreateThread(NULL, 0, ThreadFunc, "PONG", 0, NULL);
    if (hThreadArray[0] == NULL || hThreadArray[1] == NULL) {
        fprintf(stderr, "ERROR: Could not create thread!");
        exit(1);
    }
    WaitForMultipleObjects(2, hThreadArray, TRUE, INFINITE);
    if (fclose(file) != 0) {
        fprintf(stderr, "ERROR: Could not close file!");
        exit(1);
    }
    return 0;
}
