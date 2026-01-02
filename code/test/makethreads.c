#include "syscall.h"

void thread_func(void *arg) {
    char c = (char)(int)arg;
    int i;

    for (i = 0; i < 10; i++) {
        PutChar(c);
    }

    UserThreadExit();
}

int main() {
    UserThreadCreate(thread_func, (void *)'A');
    UserThreadCreate(thread_func, (void *)'B');
    UserThreadCreate(thread_func, (void *)'C');


    /* Important: main must not Halt immediately */
    /* Otherwise threads may be killed */

    Halt(); /* never reached */
    return 0;
}
