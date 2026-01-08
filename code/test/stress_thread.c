#include "syscall.h"

// Une fonction qui ne fait rien, juste acte de présence
void dummy(void *arg) {
    // On ne fait rien de spécial, on finit tout de suite.
    // Le but est de laisser le thread en état "ZOMBIE" 
    // tant que le main ne l'a pas Join.
    UserThreadExit();
}

#define     MAX_ATTEMPTS 50 // Essayons d'en créer 50 (si ta limite est 4, ça va vite bloquer)

int main() {
    int tids[MAX_ATTEMPTS];
    int i, j;
    int success_count = 0;

    PutString("--- Debut du Stress Test (Saturation) ---\n");

    // ETAPE 1 : Remplissage (Saturation)
    // On crée des threads tant qu'on peut, sans les Joiner.
    // Les slots de threads vont se remplir car on ne libère pas les IDs.
    for (i = 0; i < MAX_ATTEMPTS; i++) {
        int tid = UserThreadCreate(dummy, (void *)0);
        
        if (tid == -1) {
            PutString("Saturation atteinte a l'index: ");
            // Astuce d'affichage simple pour un entier (si < 100)
            if (i >= 10) PutChar('0' + (i / 10));
            PutChar('0' + (i % 10));
            PutChar('\n');
            break; // On a atteint la limite !
        }
        
        tids[i] = tid;
        success_count++;
        // On ne fait PAS de Join ici pour saturer la table
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

    // ETAPE 2 : Recyclage (Nettoyage partiel)
    // On libère la moitié des threads
    PutString("Nettoyage de la moitie des threads...\n");
    for (j = 0; j < success_count / 2; j++) {
        UserThreadJoin(tids[j]); 
        // Ici, ton noyau DOIT libérer tidUsed[x] et la mémoire pile.
    }

    // ETAPE 3 : Vérification du recyclage
    // On essaie de recréer un thread. Ça DOIT marcher car on a fait de la place.
    PutString("Tentative de re-creation apres nettoyage...\n");
    int new_tid = UserThreadCreate(dummy, (void *)0);
    
    if (new_tid != -1) {
        PutString("SUCCES : Thread cree avec succes (Recyclage OK) !\n");
        // N'oublie pas de le join lui aussi
        UserThreadJoin(new_tid);
    } else {
        PutString("ECHEC : Impossible de recreer un thread (Fuite de memoire ?)\n");
    }

    // Nettoyage final du reste
    for (; j < success_count; j++) {
        UserThreadJoin(tids[j]);
    }

    PutString("--- Fin du Stress Test ---\n");
    // Halt implicite via Exit du dernier thread
    Halt();
}