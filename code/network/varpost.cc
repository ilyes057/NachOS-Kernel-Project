#include "varpost.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

VarPostOffice::VarPostOffice(ReliablePostOffice *rpo_, int chunkSize)
  : rpo(rpo_), chunk(chunkSize) {
    if (chunk < 1) chunk = 1;
}

bool VarPostOffice::ParseHdr(const char *s, int *outLen) {
    if (strncmp(s, "HDR", 3) != 0) return false;
    s += 3;
    while (*s == ' ') s++;
    if (*s == '\0') return false;

    char *endptr = NULL;
    long v = strtol(s, &endptr, 10);
    if (endptr == s) return false;
    while (*endptr == ' ') endptr++;
    if (*endptr != '\0') return false;
    if (v < 0) return false;

    *outLen = (int)v;
    return true;
}

bool VarPostOffice::SendHeader(int destMachine, int len) {
    char hdr[64];
    snprintf(hdr, sizeof(hdr), "HDR %d", len);
    return rpo->SendReliable(destMachine, hdr, (int)strlen(hdr) + 1);
}

bool VarPostOffice::RecvHeader(int *fromMachine, int *outLen) {
    char buf[MaxMailSize];
    int from = -1;

    while (true) {
        unsigned int n = rpo->ReceiveReliable(&from, buf, MaxMailSize);
        if (n <= 0) continue;


        buf[(n < MaxMailSize) ? n : (MaxMailSize - 1)] = '\0';

        int L = -1;
        if (ParseHdr(buf, &L)) {
            *fromMachine = from;
            *outLen = L;
            return true;
        }

    }
}

bool VarPostOffice::SendLong(int destMachine, const char *data, int len) {
    if (len < 0) return false;

    if (!SendHeader(destMachine, len)) return false;

    int offset = 0;
    while (offset < len) {
        int part = len - offset;
        if (part > chunk) part = chunk;

        if (!rpo->SendReliable(destMachine, data + offset, part)) return false;
        offset += part;
    }
    return true;
}

int VarPostOffice::ReceiveLongAlloc(int *fromMachine, char **outData) {
    int from = -1;
    int total = -1;

    if (!RecvHeader(&from, &total)) return -1;

    char *buf = NULL;
    if (total == 0) {
        buf = new char[1];
        buf[0] = '\0';
        *fromMachine = from;
        *outData = buf;
        return 0;
    }

    buf = new char[total];
    int received = 0;

    while (received < total) {
        // On reçoit des morceaux via ReliablePostOffice
        char tmp[MaxMailSize];
        int src = -1;
        int n = rpo->ReceiveReliable(&src, tmp, MaxMailSize);
        if (n <= 0) continue;
        
        if (src != from) continue;

        int need = total - received;
        int take = (n < need) ? n : need;
        if (take > 0) memcpy(buf + received, tmp, take);
        received += take;
    }

    *fromMachine = from;
    *outData = buf;
    return total;
}
