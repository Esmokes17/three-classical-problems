#include <stdlib.h>
#include <stdatomic.h>

#define LOGGING 0

typedef struct {
    volatile atomic_ulong count;
    const char * name;
} Semaphore;

Semaphore* get_semaphore(unsigned long init_count, const char * name) {
    Semaphore* semaphore = (Semaphore *)malloc(sizeof(Semaphore));
    if (semaphore == NULL) {
        return NULL;
    }
    semaphore->count = init_count;
    semaphore->name = name;
    return semaphore;
}

void wait(Semaphore* s) {
    while (atomic_load(&s->count) <= 0);
    atomic_fetch_sub(&s->count, 1);
    if (LOGGING) printf("%s semaphore is %ld\n", s->name, s->count);
}

void signal(Semaphore* s) {
    atomic_fetch_add(&s->count, 1);
    if (LOGGING) printf("%s semaphore is %ld\n", s->name, s->count);
}
