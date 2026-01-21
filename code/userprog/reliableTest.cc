#include "system.h"
#include "interrupt.h"
#include "../network/post.h"
#include "../network/reliablepost.h"

#include <stdio.h>
#include <string.h>

static void WaitForStartup(int me) {
    if (me == 0) {
        printf("Sender waiting 5s for receiver...\n");
        Delay(5);
        printf("Starting test.\n");
    } else {
        printf("Receiver ready.\n");
        Delay(1);
    }
}

// RENOMMÉ pour éviter les conflits
void ReliablePostTest(int farAddr) {
    if (rpo == NULL) {
        printf("Erreur: Le système réseau n'est pas initialisé.\n");
        return;
    }
    int me = postOffice->GetNetAddr();
    
    if (farAddr < 0 || farAddr >= 32 || farAddr == me) {
        printf("ERROR: Invalid farAddr=%d\n", farAddr);
        interrupt->Halt();
        return;
    }
    
    WaitForStartup(me);
    
    const int N = 20;
    char msg[MaxMailSize];
    int from;
    
    if (me == 0) {
        printf("=== SENDER: Sending %d messages to machine %d ===\n", N, farAddr);
        for (int i = 0; i < N; i++) {
            snprintf(msg, sizeof(msg), "Message %d", i);
            printf("[%d/%d] Sending: \"%s\"\n", i+1, N, msg);
            rpo->SendReliable(farAddr, msg, (int)strlen(msg) + 1);
            Delay(1);
        }
        rpo->SendReliable(farAddr, "STOP", 5);
        printf("=== TEST COMPLETED ===\n");
        interrupt->Halt();
    } else {
        printf("=== RECEIVER: Waiting for messages from machine %d ===\n", farAddr);
        int count = 0;
        while (true) {
            int n = rpo->ReceiveReliable(&from, msg, MaxMailSize);
            if (n <= 0) continue;
            count++;
            printf("[msg #%d] Received: \"%s\" from %d\n", count, msg, from);
            if (strcmp(msg, "STOP") == 0) {
                printf("STOP received. Sending ACK implicitly via protocol...\n");
                Delay(2); // Laisser le temps pour l'ACK
                interrupt->Halt();
            }
        }
    }
}