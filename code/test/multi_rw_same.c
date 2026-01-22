#include "syscall.h"

#define FILE_NAME "t2same"
#define THREADS 3
#define BLOCK_SIZE 4
#define FILE_SIZE (THREADS * BLOCK_SIZE)
#define FILL_CHAR '.'
#define BASE_CHAR 'A'

static int tids[THREADS];
static int status[THREADS];

static void fill_buffer(char *buf, int len, char c) {
    int i;
    for (i = 0; i < len; i++) {
        buf[i] = c;
    }
}

static void writer(void *arg) {
    int id = (int)arg;
    char buf[BLOCK_SIZE];

    fill_buffer(buf, BLOCK_SIZE, (char)(BASE_CHAR + id));
    int fd = Open(FILE_NAME);
    if (fd < 0) {
        status[id] = 1;
        UserThreadExit();
    }
    Write(buf, BLOCK_SIZE, fd);
    Close(fd);
    UserThreadExit();
}

int main() {
    char fill[BLOCK_SIZE];
    int i;

    PutString("=== multi_rw_same ===\n");
    if (Create(FILE_NAME) < 0) {
        PutString("Create failed\n");
        Exit(1);
    }

    int fd = Open(FILE_NAME);
    if (fd < 0) {
        PutString("Open failed\n");
        Exit(1);
    }
    fill_buffer(fill, BLOCK_SIZE, FILL_CHAR);
    for (i = 0; i < THREADS; i++) {
        Write(fill, BLOCK_SIZE, fd);
    }
    Close(fd);

    for (i = 0; i < THREADS; i++) {
        status[i] = 0;
        tids[i] = UserThreadCreate(writer, (void *)i);
        if (tids[i] < 0) status[i] = 1;
    }
    for (i = 0; i < THREADS; i++) {
        if (tids[i] >= 0) UserThreadJoin(tids[i]);
    }

    for (i = 0; i < THREADS; i++) {
        if (status[i]) {
            PutString("FAIL\n");
            Exit(1);
        }
    }

    Exit(0);
}
