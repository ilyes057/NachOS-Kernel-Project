#include "syscall.h"

void slow_child(void *arg) {
    int i;
    PutString("Enfant : Je suis lent...\n");
    for(i=0; i<30; i++) { 
        // Boucle pour perdre du temps
    }
    PutString("Enfant : J'ai fini APRES le main !\n");
    UserThreadExit();
}

int main() {
    PutString("Main : Je lance l'enfant.\n");
    UserThreadCreate(slow_child, (void *)0);
    
    PutString("Main : Je meurs tout de suite (Exit sans Join).\n");
    //UserThreadExit();
    
    PutString("ERREUR : Je suis un zombie !\n"); 
    Halt();
}