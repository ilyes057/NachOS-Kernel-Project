#include "systemTable.h"

systemTable::systemTable(int cap)
    : capacity(cap), entries(nullptr), tableLock(nullptr)
{
    if (capacity <= 0) capacity = 10;

    entries = new Entry[capacity];
    for (int i = 0; i < capacity; i++) {
        entries[i].used = false;
        entries[i].hdrSector = -1;
        entries[i].openCount = 0;
    }

    tableLock = new Lock("SystemOpenFilesTableLock");
}

systemTable::~systemTable()
{
    delete tableLock;
    delete[] entries;
}

int systemTable::FindIndex(int hdrSector)
{
    for (int i = 0; i < capacity; i++) {
        if (entries[i].used && entries[i].hdrSector == hdrSector) {
            return i;
        }
    }
    return -1;
}

int systemTable::FindFreeIndex()
{
    for (int i = 0; i < capacity; i++) {
        if (!entries[i].used) return i;
    }
    return -1;
}

bool systemTable::Open(int hdrSector)
{
    if (hdrSector < 0) return false;

    tableLock->Acquire();

    int idx = FindIndex(hdrSector);
    if (idx >= 0) {
        entries[idx].openCount++;
        tableLock->Release();
        return true;
    }

    int freeIdx = FindFreeIndex();
    if (freeIdx < 0) {
        tableLock->Release();
        return false; // table pleine
    }

    entries[freeIdx].used = true;
    entries[freeIdx].hdrSector = hdrSector;
    entries[freeIdx].openCount = 1;

    tableLock->Release();
    return true;
}

void systemTable::Close(int hdrSector)
{
    if (hdrSector < 0) return;

    tableLock->Acquire();

    int idx = FindIndex(hdrSector);
    if (idx >= 0) {
        if (entries[idx].openCount > 0) {
            entries[idx].openCount--;
        }

        if (entries[idx].openCount == 0) {
            entries[idx].used = false;
            entries[idx].hdrSector = -1;
        }
    }

    tableLock->Release();
}

bool systemTable::IsOpen(int hdrSector)
{
    if (hdrSector < 0) return false;

    tableLock->Acquire();
    int idx = FindIndex(hdrSector);
    bool opened = (idx >= 0 && entries[idx].openCount > 0);
    tableLock->Release();

    return opened;
}

int systemTable::GetOpenCount(int hdrSector)
{
    if (hdrSector < 0) return 0;

    tableLock->Acquire();
    int idx = FindIndex(hdrSector);
    int c = (idx >= 0) ? entries[idx].openCount : 0;
    tableLock->Release();

    return c;
}
