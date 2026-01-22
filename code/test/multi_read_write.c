#include "syscall.h"

#define FILE_NAME "mrw_simple"

#define THREADS 4
#define BLOCK_SIZE 4
static int sharedFd = -1;

static int writerTids[THREADS];

static void fill_buffer(char *buf, int len, char c) {
    int i;
    for (i = 0; i < len; i++) {
        buf[i] = c;
    }
}


static void writer(void *arg) {
    int id = (int)arg;
    char buf[BLOCK_SIZE];

    fill_buffer(buf, BLOCK_SIZE, (char)('A' + id));


    if (sharedFd < 0) {
        UserThreadExit();
    }

    Write(buf, BLOCK_SIZE, sharedFd);
    PutString(buf);
    UserThreadExit();
}

int main() {
    int i;

    PutString("=== multi_thread_read_write (simple) ===\n");

    if (Create(FILE_NAME) < 0) {
        PutString("Create failed (try running with -f)\n");
        Exit(1);
    }

    sharedFd = Open(FILE_NAME);
    if (sharedFd < 0) {
        PutString("Open failed\n");
        Exit(1);
    }

    for (i = 0; i < THREADS; i++) {
        writerTids[i] = UserThreadCreate(writer, (void *)i);
    }

    for (i = 0; i < THREADS; i++) {
        if (writerTids[i] >= 0) UserThreadJoin(writerTids[i]);
    }
    Close(sharedFd);

    Exit(0);
}
