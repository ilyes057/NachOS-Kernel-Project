#include "syscall.h"

int main() {
    char buffer[256];
    int dest, from;
    int choice;
    int myId;

    PutString("\n--- NACHOS CHAT INTERACTIF ---\n");
    PutString("ID de cette machine : ");
    GetInt(&myId);
    PutString("ID de la machine cible : ");
    GetInt(&dest);

    while(1) {
        PutString("\n1. Envoyer un message\n");
        PutString("2. Attendre un message (bloquant)\n");
        PutString("3. Quitter\n");
        PutString("Action > ");
        GetInt(&choice);

        if (choice == 1) {
            PutString("Votre message : ");
            GetString(buffer, 256);
            
            // Calcul de la taille manuellement
            int len = 0;
            while(buffer[len] != '\0') len++;
            
            PutString("Envoi en cours...\n");
            ReseauSend(dest, buffer, len + 1);
            PutString("Envoyé !\n");
        } 
        else if (choice == 2) {
            PutString("En attente de reception...\n");
            int n = ReseauReceive(buffer, 256, &from);
            if (n > 0) {
                PutString("\n--- MESSAGE RECU de ");
                PutInt(from);
                PutString(" ---\n");
                PutString(buffer);
                PutString("\n----------------------\n");
            }
        } 
        else if (choice == 3) {
            break;
        }
    }

    Exit(0);
}