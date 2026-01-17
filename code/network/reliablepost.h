#ifndef RELIABLEPOST_H
#define RELIABLEPOST_H

#include "network.h"
#include "post.h"
#include "synchlist.h"
#include "synch.h"
#include "thread.h"

enum RelType { REL_DATA = 1, REL_ACK = 2 };

class RelHeader {
  public:
    int type; // RelType
    int seq;  // sequence number for DATA
    int ack;  // ack number for ACK
    int len;  // payload length for DATA
};

#define MAX_MACHINES 32

class ReliablePostOffice {
  public:
    ReliablePostOffice(PostOffice *poste, int boite, int tempoTicks, int nbMaxReemissions);
    ~ReliablePostOffice();

    bool SendReliable(int destMachine, const char *data, int len);
    int ReceiveReliable(int *fromMachine, char *outBuf, int maxLen);

  private:
    PostOffice *poste;
    int boite;
    int tempoTicks;
    int nbMaxReemissions;

    bool running;

    static void ArriveStub(int arg);
    void ArriveLoop();

    SynchList *delivered;

    int prochaineSeq[MAX_MACHINES];
    int expectedSeqFrom[MAX_MACHINES];

    Lock *sendLock;
    Lock *stateLock;
    Condition *stateCond;

    bool ackAttente;
    int  destAttente;
    int  seqAttente;

    bool ackRecu;
    bool timeout;

    bool timeoutArmed;
    long long timeoutDeadline;

    Semaphore *timeoutSem;
    static void TimeoutWorkerStub(int arg);
    void TimeoutWorkerLoop();

    static void TimeoutStub(int arg);
    void TimeoutArrive();

    int  ConstruireData(char *buffer, int seq, const char *payload, int payloadLen);
    int  ConstruireAck(char *buffer, int ackSeq);
    bool Decoder(const char *buffer, int msgLen, RelHeader *hdr, const char **payloadOut);

    bool EnvoyerData(int destMachine, int seq, const char *payload, int payloadLen);
    bool EnvoyerAck(int destMachine, int ackSeq);

    struct MessageLivre {
      int source;
      int len;
      char *data;
    };
};

#endif
