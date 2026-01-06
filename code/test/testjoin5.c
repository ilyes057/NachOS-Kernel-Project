#include "syscall.h"

void worker(void *arg) {
    char c = (char)(int)arg;
    int i;

    for (i = 0; i < 200; i++) {
        PutChar(c);
    }

    UserThreadExit();
}

int main() {
    int tA = UserThreadCreate(worker, (void*)'A');
    int tB = UserThreadCreate(worker, (void*)'B');
    int tC = UserThreadCreate(worker, (void*)'C');
    int tD = UserThreadCreate(worker, (void*)'D');
    int tE = UserThreadCreate(worker, (void*)'E');
    if (tA < 0 || tB < 0 || tC < 0 || tD < 0 || tE < 0) {
        PutChar('!');
        Halt();
    }

    PutChar('\n');
    PutChar('S'); PutChar('\n');  

    UserThreadJoin(tC); PutChar('1'); PutChar('\n'); 
    UserThreadJoin(tA); PutChar('2'); PutChar('\n');  
    UserThreadJoin(tE); PutChar('3'); PutChar('\n');  
    UserThreadJoin(tB); PutChar('4'); PutChar('\n');  
    UserThreadJoin(tD); PutChar('5'); PutChar('\n');  

    PutChar('D'); PutChar('O'); PutChar('N'); PutChar('E'); PutChar('\n');

    Halt();
    return 0;
}
