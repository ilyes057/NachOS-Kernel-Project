
#include "system.h"
#include "post.h"
#include "interrupt.h"
#include "reliablepost.h"

#include <stdio.h>
#include <string.h>

static ReliablePostOffice *rpo = NULL;

static void InitRPO() {
    if (rpo == NULL) {
        rpo = new ReliablePostOffice(postOffice, 2);
        printf("ReliablePostOffice initialized\n");
        fflush(stdout);
    }
}

static void WaitForStartup(int me) {
    if (me == 0) {
        printf("Sender waiting 10s for receiver...\n");
        fflush(stdout);
        Delay(5);
        printf("Starting test.\n");
        fflush(stdout);
    } else {
        printf("Receiver ready.\n");
        fflush(stdout);
        Delay(1);
    }
}

void ReliableTest(int farAddr) {
    InitRPO();
    
    int me = postOffice->GetNetAddr();
    
    if (farAddr < 0 || farAddr >= MAX_MACHINES || farAddr == me) {
        printf("ERROR: Invalid farAddr=%d\n", farAddr);
        fflush(stdout);
        interrupt->Halt();
        return;
    }
    
    WaitForStartup(me);
    
    const int N = 20;
    char msg[MaxMailSize];
    int from;
    
    if (me == 0) {
        printf("=== SENDER: Sending %d messages to machine %d ===\n", N, farAddr);
        fflush(stdout);
        
        for (int i = 0; i < N; i++) {
            snprintf(msg, sizeof(msg), "Message %d", i);
            
            printf("[%d/%d] Sending: \"%s\"\n", i+1, N, msg);
            fflush(stdout);
            
            bool ok = rpo->SendReliable(farAddr, msg, (int)strlen(msg) + 1);
            
            if (!ok) {
                printf("[%d/%d] FAILED to send\n", i+1, N);
                fflush(stdout);
            } else {
                printf("[%d/%d] Sent successfully\n", i+1, N);
                fflush(stdout);
            }
            
            Delay(1);
        }
        
        // Envoi STOP
        printf("\nSending STOP signal...\n");
        fflush(stdout);
        
        const char *stop = "STOP";
        bool ok = rpo->SendReliable(farAddr, stop, (int)strlen(stop) + 1);
        
        if (ok) {
            printf("STOP sent successfully.\n");
        } else {
            printf("Warning: STOP may not have been received.\n");
        }
        
        printf("=== TEST COMPLETED ===\n");
        fflush(stdout);
        interrupt->Halt();
        
    } else {
        // RECEIVER: Reçoit les messages
        printf("=== RECEIVER: Waiting for messages from machine %d ===\n", farAddr);
        fflush(stdout);
        
        int count = 0;
        
        while (true) {
            int n = rpo->ReceiveReliable(&from, msg, MaxMailSize);
            
            if (n <= 0) {
                printf("Warning: Empty message\n");
                fflush(stdout);
                continue;
            }
            
            count++;
            printf("[msg #%d] Received: \"%s\" from machine %d\n", count, msg, from);
            fflush(stdout);
            
            // Si STOP reçu, attendre un peu avant de mourir (pour envoyer l'ACK)
            if (strcmp(msg, "STOP") == 0) {
                printf("\nSTOP received. Processed %d messages.\n", count);
                printf("Waiting 3s to ensure ACK is sent...\n");
                fflush(stdout);
                
                Delay(3); // Laisser le temps à l'ACK d'être envoyé et reçu
                
                printf("=== TEST COMPLETED ===\n");
                fflush(stdout);
                interrupt->Halt();
                return;
            }
        }
    }
}