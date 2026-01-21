#include "system.h"
#include "interrupt.h"
#include "network.h"            
#include "../network/post.h"    

#include <stdio.h>
#include <string.h>

#define MAX_MACHINES_RING 4
static const char *STOP_MSG = "STOP";
static const char *ACK_MSG  = "ACK";

void RingTest() {
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

    int me = postOffice->GetNetAddr();
    int next = (me + 1) % MAX_MACHINES_RING;

    printf("[RingTest] me=%d next=%d\n", me, next);
    outMailHdr.from = 1;
    outMailHdr.to = 0;

    if (me == 0) {
        Delay(10);
        char msg[MaxMailSize];
        snprintf(msg, sizeof(msg), "TOKEN 0");

        outPktHdr.to = next;
        outMailHdr.to = 0;
        outMailHdr.length = strlen(msg) + 1;

        postOffice->Send(outPktHdr, outMailHdr, msg);
        printf("[RingTest] 0 injected token -> %d\n", next);
        
        // Wait ACK
        postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    }

    while (true) {
        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);

        if (strncmp(buffer, STOP_MSG, 4) == 0) {
            if (me == 0) {
                printf("[RingTest] 0 received STOP. Halting.\n");
                interrupt->Halt();
            } else {
                printf("[RingTest] %d forward STOP -> %d\n", me, next);
                outPktHdr.to = next;
                outMailHdr.to = 0;
                outMailHdr.from = 1;
                outMailHdr.length = strlen(STOP_MSG) + 1;
                postOffice->Send(outPktHdr, outMailHdr, STOP_MSG);
                interrupt->Halt();
            }
        }

        int hops = -1;
        if (sscanf(buffer, "TOKEN %d", &hops) == 1) {
            // Send ACK
            outPktHdr.to = inPktHdr.from;
            outMailHdr.to = inMailHdr.from;
            outMailHdr.from = 1;
            outMailHdr.length = strlen(ACK_MSG) + 1;
            postOffice->Send(outPktHdr, outMailHdr, ACK_MSG);

            if (me == 0 && hops+1 >= MAX_MACHINES_RING) {
                printf("[RingTest] Full round. Stopping.\n");
                outPktHdr.to = next;
                outMailHdr.to = 0;
                outMailHdr.from = 1;
                outMailHdr.length = strlen(STOP_MSG) + 1;
                postOffice->Send(outPktHdr, outMailHdr, STOP_MSG);
                continue;
            }

            char msg[MaxMailSize];
            snprintf(msg, sizeof(msg), "TOKEN %d", hops + 1);
            outPktHdr.to = next;
            outMailHdr.to = 0;
            outMailHdr.from = 1;
            outMailHdr.length = strlen(msg) + 1;
            postOffice->Send(outPktHdr, outMailHdr, msg);
            printf("[RingTest] %d forwarded token -> %d\n", me, next);
            printf("Machine 0: Attente de 10s avant d'injecter le jeton vers %d...\n", next);
            Delay(10);
            // Wait ACK
            postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
        }
    }
}