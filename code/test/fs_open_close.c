#include "syscall.h"

int main() {
    char name[] = "oc_file";

    int c = Create(name, 0);
    PutString("Create -> ");
    PutInt(c);
    PutChar('\n');

    int fd = Open(name);
    PutString("Open -> ");
    PutInt(fd);
    PutChar('\n');
    if (fd >= 0) {
        Close(fd);
        PutString("Close -> 0\n");
    } else {
        PutString("Close skipped\n");
    }

    int fd1 = Open(name);
    int fd2 = Open(name);
    PutString("Open1 -> ");
    PutInt(fd1);
    PutChar('\n');
    PutString("Open2 -> ");
    PutInt(fd2);
    PutChar('\n');
    if (fd1 >= 0) Close(fd1);
    if (fd2 >= 0) Close(fd2);

    PutString("Done\n");
    Exit(0);
}
