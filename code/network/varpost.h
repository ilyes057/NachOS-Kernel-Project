#ifndef VARPOST_H
#define VARPOST_H

#include "reliablepost.h"

class VarPostOffice {
  public:
    VarPostOffice(ReliablePostOffice *rpo_, int chunkSize = MaxMailSize-sizeof(RelHeader));

    bool SendLong(int destMachine, const char *data, int len);
    int ReceiveLongAlloc(int *fromMachine, char **outData);

  private:
    ReliablePostOffice *rpo;
    int chunk;

    bool SendHeader(int destMachine, int len);
    bool RecvHeader(int *fromMachine, int *outLen);

    bool ParseHdr(const char *s, int *outLen);
};

#endif
