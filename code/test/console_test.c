#include "syscall.h"

/* * On définit un buffer confortable de 128 octets.
 * Cela permet de lire la plupart des lignes d'un coup.
 */
#define MAX_BUFFER 128 

int main() {
    char buffer[MAX_BUFFER];
    
    PutString("<<<START_TEST>>>");

    while (1) {
        buffer[0] = '\0';


        GetString(buffer, MAX_BUFFER);

        

        PutString(buffer);
        if (buffer[0] == '\0') {
            
            break;
        }
    }

    PutString("<<<END_TEST>>>");
    
    Halt();
}