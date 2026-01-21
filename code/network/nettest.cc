#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"

// Fonction bouchon pour satisfaire la compilation
void MailTest(int farAddr) {
    printf("MailTest is deprecated. Use -t options instead.\n");
    interrupt->Halt();
}