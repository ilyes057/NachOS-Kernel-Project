#include "syscall.h"

void thread_func(void *arg) {
    char c = (char)(int)arg;
    int i;

    for (i = 0; i < 3; i++) {
        PutChar(c);
    }
    PutString("hi\n");
    //UserThreadExit();
}

int main() {
    UserThreadCreate(thread_func, (void *)'A');
    UserThreadCreate(thread_func, (void *)'B');
    UserThreadCreate(thread_func, (void *)'C');
    Halt();
    return 0;
}
