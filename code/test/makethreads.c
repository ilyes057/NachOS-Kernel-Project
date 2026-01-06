#include "syscall.h"

void thread_func(void *arg) {
    char c = (char)(int)arg;
    int i;

    for (i = 0; i < 3; i++) {
        PutChar(c);
    }
    PutString("hi\n");
}

int main() {
    int t1=UserThreadCreate(thread_func, (void *)'A');
    int t2=UserThreadCreate(thread_func, (void *)'B');
    int t3=UserThreadCreate(thread_func, (void *)'C');
    UserThreadJoin(t1);
    UserThreadJoin(t2);
    UserThreadJoin(t3);
    return 0;
}
