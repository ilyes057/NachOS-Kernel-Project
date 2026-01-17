
#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"

#include <stdio.h>
#include <string.h>

#define MAX_MACHINES 4
static const char *STOP_MSG = "STOP";
static const char *ACK_MSG  = "ACK";

void RingTest() {
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

    int me   = postOffice->GetNetAddr();
    int next = (me + 1) % MAX_MACHINES;

    printf("[RingTest] me=%d next=%d N=%d\n", me, next, MAX_MACHINES);
    fflush(stdout);
    outMailHdr.from = 1;
    outMailHdr.to = 0;

    if (me == 0) {
        Delay(1);

        char msg[MaxMailSize];
        snprintf(msg, sizeof(msg), "TOKEN %d", 0);

        outPktHdr.to = next;
        outMailHdr.to = 0;
        outMailHdr.length = strlen(msg) + 1;

        postOffice->Send(outPktHdr, outMailHdr, msg);
        printf("[RingTest] 0 injected \"%s\" -> machine %d (mbox 0)\n", msg, next);
        fflush(stdout);

        postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
        printf("[RingTest] 0 got ACK \"%s\" from %d, box %d\n",
               buffer, inPktHdr.from, inMailHdr.from);
        fflush(stdout);
    }

    while (true) {
        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);

        // ----- STOP -----
        if (strncmp(buffer, STOP_MSG, 4) == 0) {
            if (me == 0) {
                printf("[RingTest] 0 received STOP back from %d -> halting\n", inPktHdr.from);
                fflush(stdout);
                interrupt->Halt();
            } else {
                printf("[RingTest] %d got STOP from %d -> forward to %d then halt\n",
                       me, inPktHdr.from, next);
                fflush(stdout);

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
            printf("[RingTest] %d got TOKEN %d from %d (sender box %d)\n",
                   me, hops, inPktHdr.from, inMailHdr.from);
            fflush(stdout);
            outPktHdr.to = inPktHdr.from;
            outMailHdr.to = inMailHdr.from;
            outMailHdr.from = 1;
            outMailHdr.length = strlen(ACK_MSG) + 1;
            postOffice->Send(outPktHdr, outMailHdr, ACK_MSG);

            printf("[RingTest] %d sent ACK -> machine %d, box %d\n",
                   me, outPktHdr.to, outMailHdr.to);
            fflush(stdout);
            for (int k = 0; k < 50; k++) currentThread->Yield();

           if (me == 0 && hops+1 >= MAX_MACHINES) {
                printf("[RingTest] 0 full round detected (hops=%d) -> send STOP, wait STOP back\n", hops);
                fflush(stdout);

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

            printf("[RingTest] %d forwarded \"%s\" -> machine %d (mbox 0)\n",
                   me, msg, next);
            fflush(stdout);

            postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
            printf("[RingTest] %d got ACK \"%s\" from %d, box %d\n",
                   me, buffer, inPktHdr.from, inMailHdr.from);
            fflush(stdout);

        } else {
            printf("[RingTest] %d got unknown msg on mbox0: \"%s\"\n", me, buffer);
            fflush(stdout);
        }
    }
}
