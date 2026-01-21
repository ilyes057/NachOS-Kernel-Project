#include "syscall.h"

#define NUM_THREADS 4
#define NUM_OPS 1

char name[] = "oc_file";

void open_close_worker(void *arg) {
    int tid = (int)arg;
    int i;

    for (i = 0; i < NUM_OPS; i++) {
        int fd = Open(name);
        PutString("Thread ");
        PutInt(tid);
        PutString(" Open -> ");
        PutInt(fd);
        PutChar('\n');

        if (fd >= 0) {
            Close(fd);
            PutString("Thread ");
            PutInt(tid);
            PutString(" Close -> 0\n");
        } else {
            PutString("Thread ");
            PutInt(tid);
            PutString(" Close skipped\n");
        }
    }

    UserThreadExit();
}

int main() {
    int i;
    int tids[NUM_THREADS];

    int c = Create(name, 0);
    PutString("Create -> ");
    PutInt(c);
    PutChar('\n');

    for (i = 0; i < NUM_THREADS; i++) {
        tids[i] = UserThreadCreate(open_close_worker, (void *)i);
        if (tids[i] < 0) {
            PutString("UserThreadCreate failed\n");
        }
    }

    for (i = 0; i < NUM_THREADS; i++) {
        if (tids[i] >= 0) {
            UserThreadJoin(tids[i]);
        }
    }

    PutString("Done\n");
    Exit(0);
}
