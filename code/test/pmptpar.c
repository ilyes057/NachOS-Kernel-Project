#include "syscall.h"

#define FILE_NAME "pmpt"
#define FILE_SIZE 32

int main() {
    char buf[16];
    int i;

    PutString("=== pmptpar ===\n");
    if (Create(FILE_NAME) < 0) {
        PutString("Create failed\n");
        Exit(1);
    }

    int fd = Open(FILE_NAME);
    if (fd < 0) {
        PutString("Open failed\n");
        Exit(1);
    }
    Write("P", 1, fd);

    int p0 = ForkExec("pmptc0");
    int p1 = ForkExec("pmptc1");

    if (p0 >= 0) Wait(p0);
    if (p1 >= 0) Wait(p1);

    Close(fd);

    fd = Open(FILE_NAME);
    if (fd < 0) {
        PutString("Open failed (read)\n");
        Exit(1);
    }
    PutString("File: ");
    int n;
    while ((n = Read(buf, sizeof(buf), fd)) > 0) {
        for (i = 0; i < n; i++) {
            PutChar(buf[i]);
        }
    }
    PutChar('\n');
    Close(fd);

    Exit(0);
}
