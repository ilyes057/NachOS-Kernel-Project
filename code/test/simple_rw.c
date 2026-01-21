#include "syscall.h"

#define FILE_NAME "t1file"
#define DATA_SIZE 4

static int verify_buffer(char *buf, int len, char *expect) {
    int i;
    for (i = 0; i < len; i++) {
        if (buf[i] != expect[i]) return -1;
    }
    return 0;
}

int main() {
    char data[DATA_SIZE] = { 'A', 'B', 'C', 'D' };
    char out[DATA_SIZE];

    PutString("=== simple_rw ===\n");
    if (Create(FILE_NAME, DATA_SIZE) < 0) {
        PutString("Create failed\n");
        Exit(1);
    }

    int fd = Open(FILE_NAME);
    if (fd < 0) {
        PutString("Open failed\n");
        Exit(1);
    }
    Write(data, DATA_SIZE, fd);
    Close(fd);

    fd = Open(FILE_NAME);
    if (fd < 0) {
        PutString("Open failed (read)\n");
        Exit(1);
    }
    int n = Read(out, DATA_SIZE, fd);
    Close(fd);

    if (n == DATA_SIZE && verify_buffer(out, DATA_SIZE, data) == 0) {
        PutString("OK\n");
    } else {
        PutString("FAIL\n");
    }

    Exit(0);
}
