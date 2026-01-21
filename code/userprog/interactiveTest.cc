#include "system.h"
#include "network.h"
#include "../network/filetransfer.h"
#include <stdio.h>
#include <string.h>

// Helper pour lire une ligne proprement
void GetLine(char *buffer, int maxLen) {
    if (fgets(buffer, maxLen, stdin) != NULL) {
        int len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
    }
}

void InteractiveTest(int farAddr) {
    int me = postOffice->GetNetAddr();

    // 1. Définition de la Sandbox (Racine virtuelle)
    char myRoot[64];
    if (me == 1) {
        strcpy(myRoot, "server_dir");
    } else {
        sprintf(myRoot, "client%d_dir", me);
    }

    printf(">>> Init Network (Racine: build/%s/) <<<\n", myRoot);
    FileTransfer *ft = fileTransfer;
    if (me == 1) { // --- MODE SERVEUR ---
        printf("\n=== SERVEUR FTP (Machine %d) ===\n", me);
        ft->StartServer();

    } else { // --- MODE CLIENT ---
        printf("\n=== CLIENT FTP ULTIME (Machine %d -> %d) ===\n", me, farAddr);
        
        printf(" -- COMMANDES LOCALES (Mon PC) --\n");
        printf("  pwd               : Dossier actuel\n");
        printf("  lls               : Lister ici\n");
        printf("  lmkdir <dir>      : Creer dossier local\n");
        printf("  lrm <file>        : Supprimer local\n");

        printf(" -- COMMANDES DISTANTES (Serveur) --\n");
        printf("  ls [dir]          : Lister distant\n");
        printf("  put <loc> [dist]  : Envoyer (Upload)\n");
        printf("  get <dist> [loc]  : Recevoir (Download)\n");
        printf("  mkdir <dir>       : Creer dossier\n");
        printf("  rmdir <dir>       : Supprimer dossier\n");
        printf("  delete <file>     : Supprimer fichier (rm)\n");
        printf("  rename <old> <new>: Renommer (mv)\n");
        printf("  quit              : Quitter\n");
        printf("==========================================\n");

        char buffer[256], cmd[20], arg1[100], arg2[100];

        while (true) {
            printf("\n%s@FTP[%d]> ", myRoot, me);
            GetLine(buffer, 256);

            // Reset args
            arg1[0] = '\0'; arg2[0] = '\0';
            // Parsing simple
            int n = sscanf(buffer, "%s %s %s", cmd, arg1, arg2);
            if (n < 1) continue;

            // --- QUITTER ---
            if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) break;

            // --- COMMANDES LOCALES ---
            else if (strcmp(cmd, "pwd") == 0) ft->PrintWorkingDir();
            else if (strcmp(cmd, "lls") == 0) ft->ListLocal();
            else if (strcmp(cmd, "lmkdir") == 0) ft->LocalMkdir(arg1);
            else if (strcmp(cmd, "lrmdir") == 0) ft->LocalRmdir(arg1);
            else if (strcmp(cmd, "lrm") == 0) ft->LocalDelete(arg1);
            else if (strcmp(cmd, "lmv") == 0) ft->LocalRename(arg1, arg2);

            // --- COMMANDES DISTANTES ---
            
            // Lister
            else if (strcmp(cmd, "ls") == 0) ft->RequestList(farAddr, arg1);
            
            // Envoyer (PUT)
            else if (strcmp(cmd, "put") == 0) {
                if (n < 2) { printf("Usage: put <local> [distant]\n"); continue; }
                const char *d = (n >= 3) ? arg2 : arg1;
                if (ft->RequestPut(farAddr, arg1, d)) printf("-> OK (Envoye)\n");
            }
            
            // Recevoir (GET)
            else if (strcmp(cmd, "get") == 0) {
                if (n < 2) { printf("Usage: get <distant> [local]\n"); continue; }
                const char *d = (n >= 3) ? arg2 : arg1;
                if (ft->RequestGet(farAddr, arg1, d)) printf("-> OK (Recu)\n");
            }

            // Supprimer (DELETE / RM)
            else if (strcmp(cmd, "rm") == 0 || strcmp(cmd, "delete") == 0 || strcmp(cmd, "del") == 0) {
                 if (n < 2) { printf("Usage: rm <fichier>\n"); continue; }
                 if (ft->RequestDelete(farAddr, arg1)) printf("-> OK (Supprime)\n");
                 else printf("-> ECHEC\n");
            }

            // Renommer (RENAME / MV)
            else if (strcmp(cmd, "mv") == 0 || strcmp(cmd, "rename") == 0) {
                 if (n < 3) { printf("Usage: rename <old> <new>\n"); continue; }
                 if (ft->RequestRename(farAddr, arg1, arg2)) printf("-> OK (Renomme)\n");
                 else printf("-> ECHEC\n");
            }

            // Dossiers
            else if (strcmp(cmd, "mkdir") == 0) {
                 if (ft->RequestMkdir(farAddr, arg1)) printf("-> OK\n"); else printf("-> ECHEC\n");
            }
            else if (strcmp(cmd, "rmdir") == 0) {
                 if (ft->RequestRmdir(farAddr, arg1)) printf("-> OK\n"); else printf("-> ECHEC\n");
            }

            else {
                printf("Commande inconnue.\n");
            }
        }
        interrupt->Halt();
    }
}