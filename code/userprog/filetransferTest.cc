#include "system.h"
#include "interrupt.h"
#include "../network/post.h"
#include "../network/reliablepost.h"
#include "../network/varpost.h"
#include "../network/filetransfer.h"

#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
long GetFileSize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) return st.st_size;
    return 0;
}

void PrintStats(const char *op, int ticks, long bytes) {
    double sec = ticks / 1000000.0;
    double kb = bytes / 1024.0;
    printf("\n--- STATS %s ---\n", op);
    printf("Taille : %.2f Ko\n", kb);
    printf("Temps  : %.4f s (%d ticks)\n", sec, ticks);
    if (sec > 0) printf("Débit  : %.2f Ko/s\n", kb/sec);
    printf("------------------\n");
}

void FileTransferTest(int farAddr) {
    FileTransfer *ft = fileTransfer;
    int me = postOffice->GetNetAddr();

    if (me != 1) {
        // --- CLIENT ---
        printf("=== CLIENT (Machine 0) ===\n");
        Delay(2);

        char filePath[64];
        sprintf(filePath, "client%d_dir/source.txt", me);
        printf("Création du fichier '%s' avec appels système...\n", filePath);
        
        // 0644 = rw-r--r--
        int fd = open("source.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        
        if (fd >= 0) {
            char line[128];
            for(int i=0; i<200; i++) {
                // On formate la ligne en mémoire
                int len = sprintf(line, "Ligne de données %d pour faire du volume.\n", i);
                // On écrit directement le buffer avec l'appel système
                write(fd, line, len);
            }
            close(fd);
        } else {
            printf("ERREUR: Impossible de créer source.txt\n");
        }

        // TEST 1: PUT
        printf("\n Test PUT (Envoi source.txt)...\n");
        int start = stats->totalTicks;
        if (ft->RequestPut(farAddr, "source.txt", "server_copy.txt")) {
            PrintStats("PUT", stats->totalTicks - start, GetFileSize("source.txt"));
        }

        // TEST 2: GET
        printf("\n>>> Test GET (Récupération server_copy.txt)...\n");
        start = stats->totalTicks;
        if (ft->RequestGet(farAddr, "server_copy.txt", "final_copy.txt")) {
            PrintStats("GET", stats->totalTicks - start, GetFileSize("final_copy.txt"));
        }

        interrupt->Halt();

    } else {
        // --- SERVEUR ---
        printf("=== SERVEUR (Machine %d) ===\n", me);
        ft->StartServer();
    }
}