#include "syscall.h"

void thread_long(void *arg) {
    int i;
    PutString("--- Enfant (T1) : Je demarre et je suis lent ---\n");
    for(i=0; i<50; i++); 
    PutString("--- Enfant (T1) : J'ai fini ! (C'est moi qui devrais eteindre la lumiere) ---\n");
    UserThreadExit();
}

void thread_rapide(void *arg) {
    PutString("--- Enfant (T2) : Je suis rapide ---\n");
    UserThreadExit();
}

int main() {
    PutString("Main : Je lance T2 (rapide).\n");
    int t2 = UserThreadCreate(thread_rapide, (void *)0);
    
    PutString("Main : J'attends T2 (Join)...\n");
    UserThreadJoin(t2);
    PutString("Main : T2 fini. Je reprends mon travail ! (Preuve que Join ne tue pas le Main)\n");

    PutString("Main : Je lance T1 (lent).\n");
    UserThreadCreate(thread_long, (void *)0);
    
    PutString("Main : Je termine SANS attendre T1 (Pas de Join).\n");
    PutString("Main : Si NachOS s'arrete maintenant, c'est un BUG.\n");
    

    
    Halt(); // Ne sera jamais atteint
}