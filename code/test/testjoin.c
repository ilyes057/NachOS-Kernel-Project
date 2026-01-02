#include "syscall.h"

void threadA(void *arg) {
    int i;
    for (i = 0; i < 200; i++) {
        PutChar('A');
    }
    UserThreadExit();
}

int main() {
    int tid = UserThreadCreate(threadA, 0);

    PutChar('S');       
    UserThreadJoin(tid); 
    PutChar('J');    
    PutChar('E');        

    Halt();
    return 0;
}
