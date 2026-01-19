

#include "copyright.h"
#include "system.h"
#include "post.h"
#include "interrupt.h"
#include "reliablepost.h"
#include "varpost.h"

#include <stdio.h>
#include <string.h>



void ReliableTest(int farAddr) {
    // Instanciation des couches réseau
    ReliablePostOffice *rpo = new ReliablePostOffice(postOffice, 2);
    VarPostOffice vpo(rpo);

    int me = postOffice->GetNetAddr();

    if (farAddr < 0 || farAddr >= MAX_MACHINES || farAddr == me) {
        printf("ERROR: Invalid farAddr=%d\n", farAddr);
        interrupt->Halt();
        return;
    }
    if (me ==0) {
        // ======== SENDER ========
        printf("=== SENDER (machine %d -> %d) ===\n", me, farAddr);
        
        Delay(5);

        // Message à envoyer
        const char *message = 
            "Bonjour! Ceci est un message de test pour varpost. "
            "Ce message est suffisamment long pour être fragmenté en plusieurs paquets. "
            "Il permet de tester l'envoi et la réception de messages de taille variable. "
            "varpost gère automatiquement la fragmentation et le réassemblage. "
            "Fin du message de test.";
        
        int msgLen = strlen(message) + 1;  // +1 pour le '\0'

        printf("Sending message (%d bytes):\n\"%s\"\n\n", msgLen, message);

        bool ok = vpo.SendLong(farAddr, message, msgLen);

        if (!ok) {
            printf("ERROR: SendLong failed\n");
            interrupt->Halt();
            return;
        }

        printf("Message sent successfully!\n");

        // Attendre ACK
        char reply[MaxMailSize];
        int from = -1;
        rpo->ReceiveReliable(&from, reply, MaxMailSize);

        printf("Received ACK: \"%s\"\n", reply);
        Delay(1);
        interrupt->Halt();

    } else {
        // ======== RECEIVER ========
        printf("=== RECEIVER (machine %d <- %d) ===\n", me, farAddr);
        printf("Waiting for message...\n");

        int from = -1;
        char *recvBuf = NULL;

        int total = vpo.ReceiveLongAlloc(&from, &recvBuf);

        if (total < 0) {
            printf("ERROR: ReceiveLongAlloc failed\n");
            Delay(1);
            interrupt->Halt();
        }

        printf("Received message (%d bytes) from machine %d:\n", total, from);
        printf("\"%s\"\n\n", recvBuf);

        // Envoyer ACK
        const char *ack = "Message received OK!";
        rpo->SendReliable(from, ack, strlen(ack) + 1);
        
        printf("ACK sent.\n");

        delete[] recvBuf;
        Delay(1);
        interrupt->Halt();
    }
}