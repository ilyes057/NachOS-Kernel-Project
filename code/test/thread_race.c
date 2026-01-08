#include "syscall.h"

volatile int shared_counter = 0;

#define NUM_ITERATIONS 50

#define NUM_THREADS 3

void incrementer(void *arg) {
    int i, j;
    int temp;

    for (i = 0; i < NUM_ITERATIONS; i++) {
        temp = shared_counter;
        
        for(j=0; j<20; j++); 

        shared_counter = temp + 1;
    }
    
    UserThreadExit();
}

int main() {
    int tid1, tid2, tid3;

    PutString("Lancement du test de Race Condition...\n");
    PutString("Attendu: "); 
        
    tid1 = UserThreadCreate(incrementer, (void *)0);
    tid2 = UserThreadCreate(incrementer, (void *)0);
    tid3 = UserThreadCreate(incrementer, (void *)0);
    
    UserThreadJoin(tid1);
    UserThreadJoin(tid2);
    UserThreadJoin(tid3);

    
    if (shared_counter == NUM_ITERATIONS * NUM_THREADS) {
        PutString("SUCCES : Compteur correct !\n");
    } else {
        PutString("ECHEC (Prevu) : Race Condition detectee !\n");
    }

    Halt(); 
}