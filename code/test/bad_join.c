#include "syscall.h"

int main() {
    PutString("--- Test Robustesse Join ---\n");
    
    PutString("Tentative de Join sur ID negatif...\n");
    UserThreadJoin(-5); // Ne doit pas crasher
    
    PutString("Tentative de Join sur ID trop grand...\n");
    UserThreadJoin(10000); // Ne doit pas crasher

    PutString("Tentative de Join sur ID inexistant (mais valide)...\n");
    UserThreadJoin(20); // Suppose que le thread 20 n'existe pas

    PutString("SUCCES : Le noyau a survie aux attaques !\n");
    return 0;
}