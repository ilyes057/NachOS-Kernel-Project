#include "syscall.h"

// Variables globales pour que les threads connaissent les IDs des autres
volatile int id_A = 0;
volatile int id_B = 0;

void thread_A(void *arg) {
    PutString("A: J'attends que B soit cree...\n");
    // Attente active le temps que le main crée B
    while (id_B == 0) {
        // On attend...
    }

    PutString("A: Je tente de Join(B)...\n");
    // C'est ici que A va s'endormir en attendant la mort de B
    UserThreadJoin(id_B);
    
    PutString("A: ECHEC ! Je ne devrais jamais arriver ici !\n");
    UserThreadExit();
}

void thread_B(void *arg) {
    PutString("B: J'attends que A soit cree...\n");
    while (id_A == 0) {
        // On attend...
    }

    PutString("B: Je tente de Join(A)...\n");
    // C'est ici que B va s'endormir en attendant la mort de A
    UserThreadJoin(id_A);
    
    PutString("B: ECHEC ! Je ne devrais jamais arriver ici !\n");
    UserThreadExit();
}

int main() {
    PutString("--- Test Deadlock (Join Croise) ---\n");

    // 1. On lance A
    id_A = UserThreadCreate(thread_A, (void *)0);
    
    // 2. On lance B
    id_B = UserThreadCreate(thread_B, (void *)0);


    PutString("Main: Je lance le deadlock et j'attends la fin du monde...\n");
    
    UserThreadJoin(id_A);
    UserThreadJoin(id_B);

    PutString("Main: ECHEC ! Le systeme ne s'est pas bloque !\n");
    return 0;
}