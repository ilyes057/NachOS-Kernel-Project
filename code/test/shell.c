#include "syscall.h"

// --- Fonction utilitaire pour comparer deux chaînes ---
// Retourne 0 si s1 est identique à s2
int my_strcmp(char *s1, char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void clean(char* buffer){
    for(int i = 0; i < 60; i++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0'; 
                break;            
            }
            
            if (buffer[i] == '\0') break; 
    }
}

int main() {
    char buffer[60];
    int newProcID;


    PutString("Bienvenue dans le NachOS Shell \n");

    while(1) {
        PutString("nachos> ");

        GetString(buffer, 60); 
        clean(buffer);

        if (my_strcmp(buffer, "quit") == 0) {
            PutString("quitting...");
            break;
        }

        newProcID = ForkExec(buffer); 
        PutInt(newProcID);
        if (newProcID == -1) {
            PutString("Error fork exec\n");
        }else{
            Wait(newProcID);

        }
    }
    
    PutString("Fermeture du Shell.\n");
    return 0;
}