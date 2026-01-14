#include "syscall.h"

void thread2(void *arg) {
    PutString("T2: je suis le thread cree par T1\n");
    PutString("T2: fin\n");
    UserThreadExit();
}

void thread1(void *arg) {
    PutString("T1: je suis le thread cree par main\n");
    PutString("T1: je cree T2...\n");

    int t2 = UserThreadCreate(thread2, 0);
    UserThreadJoin(t2);

    PutString("T1: T2 a fini, je termine\n");
    UserThreadExit();
}

int main() {
    PutString("main: debut\n");
    PutString("main: je cree T1...\n");

    int t1 = UserThreadCreate(thread1, 0);
    UserThreadJoin(t1);

    PutString("main: T1 a fini, fin du programme\n");
    return 0;
}
