#include "reliablepost.h"
#include "system.h"

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>



ReliablePostOffice::ReliablePostOffice(PostOffice *poste_, int boite_, int tempoTicks_, int nbMaxReemissions_)
    : poste(poste_), boite(boite_), tempoTicks(tempoTicks_), nbMaxReemissions(nbMaxReemissions_) {

    running = true;
    delivered = new SynchList();

    sendLock = new Lock("sendLock");
    stateLock = new Lock("stateLock");
    stateCond = new Condition("stateCond");
    timeoutSem = new Semaphore("timeoutSem", 0);

    ackAttente = false;
    destAttente = -1;
    seqAttente = -1;
    ackRecu = false;
    timeout = false;
    timeoutArmed = false;
    timeoutDeadline = 0;

    for (int i = 0; i < MAX_MACHINES; i++) {
        prochaineSeq[i] = 0;
        expectedSeqFrom[i] = 0;
    }

    Thread *arriveThread = new Thread("rpo-arrive");
    Thread *timeoutWorkerThread = new Thread("rpo-timeoutworker");

    arriveThread->Fork(ArriveStub, (int)this);
    timeoutWorkerThread->Fork(TimeoutWorkerStub, (int)this);
}

ReliablePostOffice::~ReliablePostOffice() {
    running = false;
    timeoutSem->V();

    delete delivered;
    delete sendLock;
    delete stateLock;
    delete stateCond;
    delete timeoutSem;
}

void ReliablePostOffice::ArriveStub(int arg) {
    ((ReliablePostOffice *)arg)->ArriveLoop();
}

void ReliablePostOffice::TimeoutWorkerStub(int arg) {
    ((ReliablePostOffice *)arg)->TimeoutWorkerLoop();
}

void ReliablePostOffice::TimeoutStub(int arg) {
    ((ReliablePostOffice *)arg)->TimeoutArrive();
}

// Construire le paquet de donnees qui contient le header a envoyer
int ReliablePostOffice::ConstruireData(char *out, int seq, const char *payload, int payloadLen) {
    RelHeader h;
    h.type = REL_DATA;
    h.seq = seq;
    h.ack = 0;

    int maxPayload = MaxMailSize - (int)sizeof(RelHeader);
    if (payloadLen < 0) payloadLen = 0;
    if (payloadLen > maxPayload) payloadLen = maxPayload;
    h.len = payloadLen;

    memcpy(out, &h, sizeof(RelHeader));
    if (payloadLen > 0) {
        memcpy(out + sizeof(RelHeader), payload, payloadLen);
    }

    return (int)sizeof(RelHeader) + payloadLen;
}

// Construire le paquet ACK a envoyer
int ReliablePostOffice::ConstruireAck(char *out, int ackSeq) {
    RelHeader h;
    h.type = REL_ACK;
    h.seq = 0;
    h.ack = ackSeq;
    h.len = 0;

    memcpy(out, &h, sizeof(RelHeader));
    return (int)sizeof(RelHeader);
}

// permet de decoder un paquet recu 
bool ReliablePostOffice::Decoder(const char *buf, int msgLen, RelHeader *hdr, const char **payloadOut) {
    if (msgLen < (int)sizeof(RelHeader)) return false;

    memcpy(hdr, buf, sizeof(RelHeader));

    if (hdr->type != REL_DATA && hdr->type != REL_ACK) return false;
    if (hdr->len < 0) return false;

    if (hdr->type == REL_ACK && hdr->len != 0) return false;

    if ((int)sizeof(RelHeader) + hdr->len != msgLen) return false;

    *payloadOut = buf + sizeof(RelHeader);
    return true;
}

bool ReliablePostOffice::EnvoyerData(int destMachine, int seq, const char *payload, int payloadLen) {
    int maxPayload = MaxMailSize - (int)sizeof(RelHeader);
    if (payloadLen < 0 || payloadLen > maxPayload) return false;

    PacketHeader pkt;
    MailHeader mail;

    pkt.to = destMachine;
    mail.to = boite;
    mail.from = boite;

    char buf[MaxMailSize];
    int n = ConstruireData(buf, seq, payload, payloadLen);
    mail.length = n;

    poste->Send(pkt, mail, buf);
    return true;
}

bool ReliablePostOffice::EnvoyerAck(int destMachine, int ackSeq) {
    PacketHeader pkt;
    MailHeader mail;

    pkt.to = destMachine;
    mail.to = boite;
    mail.from = boite;

    char buf[MaxMailSize];
    int n = ConstruireAck(buf, ackSeq);
    mail.length = n;

    poste->Send(pkt, mail, buf);
    return true;
}

void ReliablePostOffice::TimeoutArrive() {
    timeoutSem->V();
}

void ReliablePostOffice::TimeoutWorkerLoop() {
    while (true) {
        timeoutSem->P();
        if (!running) break;

        stateLock->Acquire();
        if (timeoutArmed && ackAttente && !ackRecu) {
            if (stats->totalTicks >= timeoutDeadline) {
                timeout = true;
                timeoutArmed = false;
                stateCond->Signal(stateLock);
            }
        }
        stateLock->Release();
    }
}

// Execute par un autre thread qui permet de recevoir des messages 
// soit des paquet data ou bien ACK
void ReliablePostOffice::ArriveLoop() {
    PacketHeader inPkt;
    MailHeader inMail;
    char buf[MaxMailSize];

    while (true) {
        poste->Receive(boite, &inPkt, &inMail, buf);

        RelHeader h;
        const char *payload = NULL;
        if (!Decoder(buf, (int)inMail.length, &h, &payload)) {
            continue;
        }

        int src = inPkt.from;
        if (src < 0 || src >= MAX_MACHINES) {
            continue;
        }

        if (h.type == REL_ACK) {
            stateLock->Acquire();
            if (ackAttente && src == destAttente && h.ack == seqAttente) {
                ackRecu = true;
                timeoutArmed = false;
                ackAttente = false;
                stateCond->Signal(stateLock);
            }
            stateLock->Release();
            continue;
        }

        if (h.type == REL_DATA) {
            bool deliver = false;

            stateLock->Acquire();
            if (h.seq == expectedSeqFrom[src]) {
                expectedSeqFrom[src]++;
                deliver = true;
            } else {
                deliver = false; // duplicate or out-of-order
            }
            stateLock->Release();

            EnvoyerAck(src, h.seq);

            if (!deliver) {
                continue;
            }

            MessageLivre *msg = new MessageLivre;
            msg->source = src;
            msg->len = h.len;
            msg->data = new char[h.len];

            if (h.len > 0) {
                memcpy(msg->data, payload, h.len);
            }

            delivered->Append((void *)msg);
            continue;
        }
    }
}

bool ReliablePostOffice::SendReliable(int destMachine, const char *data, int len) {
    if (destMachine < 0 || destMachine >= MAX_MACHINES) return false;

    int maxPayload = MaxMailSize - (int)sizeof(RelHeader);
    if (len < 0 || len > maxPayload) return false;

    sendLock->Acquire();

    int seq = prochaineSeq[destMachine]++;
    bool success = false;

    for (int attempt = 0; attempt <= nbMaxReemissions; attempt++) {
        int delay = tempoTicks;
        if (delay <= 0) delay = 1;

        stateLock->Acquire();
        ackAttente = true;
        destAttente = destMachine;
        seqAttente = seq;
        ackRecu = false;
        timeout = false;
        timeoutArmed = true;
        timeoutDeadline = stats->totalTicks + delay;

        // Timer interrupt
        interrupt->Schedule(TimeoutStub, (int)this, delay, TimerInt);
        stateLock->Release();

        if (!EnvoyerData(destMachine, seq, data, len)) {
            stateLock->Acquire();
            ackAttente = false;
            timeoutArmed = false;
            stateLock->Release();
            break;
        }
        stateLock->Acquire();
        while (!ackRecu && !timeout) {
            stateCond->Wait(stateLock);
        }

        if (ackRecu) {
            success = true;
            timeoutArmed = false;
            ackAttente = false;
            stateLock->Release();
            break;
        }

        timeoutArmed = false;
        ackAttente = false;
        stateLock->Release();
    }

    stateLock->Acquire();
    ackAttente = false;
    timeoutArmed = false;
    stateLock->Release();

    sendLock->Release();
    return success;
}

int ReliablePostOffice::ReceiveReliable(int *fromMachine, char *outBuf, int maxLen) {
    MessageLivre *msg = (MessageLivre *)delivered->Remove();

    int toCopy = msg->len;
    if (toCopy > maxLen) toCopy = maxLen;
    if (toCopy < 0) toCopy = 0;

    if (toCopy > 0) {
        memcpy(outBuf, msg->data, toCopy);
    }

    *fromMachine = msg->source;

    delete[] msg->data;
    delete msg;

    return toCopy;
}
