#include "syscall.h"

volatile int global_counter = 0;

void runner(void *arg) {
    char id = (char)(int)arg;
    int i;
    
    for (i = 0; i < 20; i++) {

        PutChar(id);
        

        global_counter++; 
    }
    
    //fin du thread
    UserThreadExit();
}

int main() {
    int t1, t2, t3;
    
    PutString("--- Debut du Test Step 3 (Standard) ---\n");


    t1 = UserThreadCreate(runner, (void *)'A');
    t2 = UserThreadCreate(runner, (void *)'B');
    t3 = UserThreadCreate(runner, (void *)'C');

    PutString("Threads lances. Le main attend (Join)...\n");

    UserThreadJoin(t1);
    UserThreadJoin(t2);
    UserThreadJoin(t3);

    PutString("\n--- Tous les threads ont fini ! ---\n");
    
    if (global_counter > 0) {
        PutString("Compteur global modifie (Preuve de memoire partages).\n");
    }

    return 0;
}