#include "syscall.h"

// Une fonction qui ne fait rien, juste acte de présence
void dummy(void *arg) {

    UserThreadExit();
}

#define     MAX_ATTEMPTS 50 

int main() {
    int tids[MAX_ATTEMPTS];
    int i, j;
    int success_count = 0;

    PutString("--- Debut du Stress Test (Saturation) ---\n");


    for (i = 0; i < MAX_ATTEMPTS; i++) {
        int tid = UserThreadCreate(dummy, (void *)0);
        
        if (tid == -1) {
            PutString("Saturation atteinte a l'index: ");
            if (i >= 10) PutChar('0' + (i / 10));
            PutChar('0' + (i % 10));
            PutChar('\n');
            break; // On a atteint la limite 
        }
        
        tids[i] = tid;
        success_count++;
    }

    if (success_count == 0) {
        PutString("ECHEC CRITIQUE : Impossible de creer le moindre thread !\n");
        Halt();
    }

    if (success_count == MAX_ATTEMPTS) {
        PutString("ATTENTION : La limite n'a pas ete atteinte. Augmente MAX_ATTEMPTS ?\n");
    } else {
        PutString("SUCCES : Le systeme a refuse poliment un nouveau thread (-1).\n");
    }

    PutString("Nettoyage de la moitie des threads...\n");
    for (j = 0; j < success_count / 2; j++) {
        UserThreadJoin(tids[j]); 
    }


    PutString("Tentative de re-creation apres nettoyage...\n");
    int new_tid = UserThreadCreate(dummy, (void *)0);
    
    if (new_tid != -1) {
        PutString("SUCCES : Thread cree avec succes (Recyclage OK) !\n");
        UserThreadJoin(new_tid);
    } else {
        PutString("ECHEC : Impossible de recreer un thread (Fuite de memoire ?)\n");
    }

    for (; j < success_count; j++) {
        UserThreadJoin(tids[j]);
    }

    PutString("--- Fin du Stress Test ---\n");
    Halt();
}