#include "syscall.h"

/* --- Utilitaires de chaînes de caractères --- */

int strlen(const char *s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

void trim(char *s) {
    int len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' ')) {
        s[len - 1] = '\0';
        len--;
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

/* Découpe l'entrée utilisateur : "commande argument" -> "commande" et "argument" */
void parse_cmd(char* input, char* cmd, char* arg) {
    int i = 0, j = 0;
    while (input[i] != ' ' && input[i] != '\0') {
        cmd[i] = input[i];
        i++;
    }
    cmd[i] = '\0';
    
    if (input[i] == ' ') {
        i++;
        while (input[i] != '\0') {
            arg[j] = input[i];
            i++; j++;
        }
    }
    arg[j] = '\0';
}

/* --- Fonction principale --- */

int main() {
    int server = 1;
    char input[128];
    char cmd[32];
    char arg1[64];

    PutString("\n==========================================\n");
    PutString("       NACHOS OS - DISTRIBUTED FTP        \n");
    PutString("==========================================\n");
    PutString(" Commands: ls, lls, cat, get, put, exit\n");

    while (1) {
        PutString("\nftp> ");
        GetString(input, 128);
        trim(input);
        
        if (strlen(input) == 0) continue;

        // On sépare la commande de son argument (ex: "get fichier.txt")
        parse_cmd(input, cmd, arg1);

        /* QUITTER */
        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            break;
        }

        /* LS DISTANT (Serveur) */
        else if (strcmp(cmd, "ls") == 0) {
            PutString("Listing remote files...\n");
            FtpList(server, ".");
        }

        /* LLS LOCAL (Client) - Utilise le syscall LocalList */
        else if (strcmp(cmd, "lls") == 0) {
            LocalList("."); 
        }

        /* GET - Télécharger */
        else if (strcmp(cmd, "get") == 0) {
            if (strlen(arg1) == 0) {
                PutString("Usage: get <filename>\n");
            } else {
                PutString("Downloading "); PutString(arg1); PutString("...\n");
                FtpGet(server, arg1, arg1);
            }
        }

        /* PUT - Envoyer */
        else if (strcmp(cmd, "put") == 0) {
            if (strlen(arg1) == 0) {
                PutString("Usage: put <filename>\n");
            } else {
                PutString("Uploading "); PutString(arg1); PutString("...\n");
                FtpPut(server, arg1, arg1);
            }
        }

        /* CAT - Afficher le contenu - Utilise le syscall LocalCat */
        else if (strcmp(cmd, "cat") == 0) {
            if (strlen(arg1) == 0) {
                PutString("Usage: cat <filename>\n");
            } else {
                LocalCat(arg1);
            }
        }

        /* AIDE / COMMANDE INCONNUE */
        else {
            PutString("Unknown command. Available: ls, lls, cat, get, put, exit\n");
        }
    }

    PutString("\nShutting down client. Goodbye!\n");
    return 0;
}