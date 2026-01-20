#ifndef SYSTEM_TABLE_H
#define SYSTEM_TABLE_H

#include "synch.h"   // Lock

class systemTable {
public:
    explicit systemTable(int capacity = 10);
    ~systemTable();

    bool Open(int hdrSector);
    void Close(int hdrSector);
    bool IsOpen(int hdrSector);
    int GetOpenCount(int hdrSector);
    void Print();

private:
    struct Entry {
        bool used;
        int hdrSector;
        int openCount;
    };

    int capacity;
    Entry* entries;
    Lock* tableLock;

    int FindIndex(int hdrSector);
    int FindFreeIndex();
};

#endif 
