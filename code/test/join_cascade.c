#include "syscall.h"

void alphabet_runner(void *arg) {
    char c;
    int i;
    
    PutString("--- Enfant : Je commence l'alphabet ---\n");
    
    // L'enfant écrit de A à Z
    for (c = 'A'; c <= 'Z'; c++) {
        PutChar(c);
        //un peu de perte du temps pour focer un context switch
        for(i=0; i<10; i++); 
    }
    
    PutString("\n--- Enfant : J'ai fini ! ---\n");
    UserThreadExit();
}

int main() {
    int tid;

    PutString("Main : Je cree le thread enfant.\n");
    tid = UserThreadCreate(alphabet_runner, (void *)0);

    PutString("Main : J'appelle Join et j'attends...\n");
    
    UserThreadJoin(tid);

    PutString("Main \n");
    
    Halt();
}