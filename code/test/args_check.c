#include "syscall.h"

void printer(void *arg) {
    char *message = (char *)arg;
    // Si l'argument est mal passé, ceci affichera du garbage ou plantera
    PutString("Thread dit : ");
    PutString(message);
    PutString("\n");
    UserThreadExit();
}

int main() {
    PutString("--- Test Passage Arguments (Pointeurs) ---\n");
    // On passe une adresse mémoire (pointeur vers string)
    int t1 = UserThreadCreate(printer, (void *)"Bonjour");
    int t2 = UserThreadCreate(printer, (void *)"Monde");
    
    UserThreadJoin(t1);
    UserThreadJoin(t2);
    
    return 0;
}