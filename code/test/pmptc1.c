#include "syscall.h"

#define THREADS 3

static int tids[THREADS];

static void worker(void *arg) {
    char c = (char)('a' + (int)arg);
    Write(&c, 1, 0);
    UserThreadExit();
}

int main() {
    int i;

    for (i = 0; i < THREADS; i++) {
        tids[i] = UserThreadCreate(worker, (void *)i);
    }
    for (i = 0; i < THREADS; i++) {
        UserThreadJoin(tids[i]);
    }
    Exit(0);
}
