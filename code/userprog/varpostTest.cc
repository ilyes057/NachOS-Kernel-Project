#include "copyright.h"
#include "system.h"
#include "interrupt.h"
#include "../network/post.h"
#include "../network/reliablepost.h"
#include "../network/varpost.h"

#include <stdio.h>
#include <string.h>

// RENOMMÉ pour éviter les conflits
void VarPostTest(int farAddr) {

    int me = postOffice->GetNetAddr();

    if (farAddr < 0 || farAddr >= 32 || farAddr == me) {
        printf("ERROR: Invalid farAddr=%d\n", farAddr);
        interrupt->Halt();
        return;
    }

    if (me == 0) {
        printf("=== SENDER (machine %d -> %d) ===\n", me, farAddr);
        Delay(5);

        const char *message = 
            "Bonjour! Ceci est un message de test pour varpost. "
            "Ce message est suffisamment long pour être fragmenté en plusieurs paquets. "
            "Il permet de tester l'envoi et la réception de messages de taille variable. "
            "varpost gère automatiquement la fragmentation et le réassemblage. "
            "Fin du message de test.";
        
        int msgLen = strlen(message) + 1;
        printf("Sending message (%d bytes)...\n", msgLen);

        if (vpo->SendLong(farAddr, message, msgLen)) {
             printf("Message sent successfully!\n");
             // Attente ACK applicatif
             char reply[MaxMailSize];
             int from = -1;
             rpo->ReceiveReliable(&from, reply, MaxMailSize);
             printf("Received ACK: \"%s\"\n", reply);
        } else {
             printf("ERROR: SendLong failed\n");
        }
        Delay(1);
        interrupt->Halt();

    } else {
        printf("=== RECEIVER (machine %d <- %d) ===\n", me, farAddr);
        printf("Waiting for message...\n");

        int from = -1;
        char *recvBuf = NULL;
        int total = vpo->ReceiveLongAlloc(&from, &recvBuf);

        if (total >= 0) {
            printf("Received message (%d bytes) from %d:\n\"%s\"\n", total, from, recvBuf);
            rpo->SendReliable(from, "Message received OK!", 21);
            delete[] recvBuf;
        } else {
            printf("ERROR: ReceiveLongAlloc failed\n");
        }
        Delay(1);
        interrupt->Halt();
    }
}