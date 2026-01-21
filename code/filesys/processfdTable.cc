#include "processfdTable.h"
#include "synch.h"

processfdTable::processfdTable() {
    fdtableLock = new Lock("processfdTableLock");
    for (int i = 0; i < MAX_OPEN_FILES; i++){
        table[i].used = false;
        table[i].of = NULL;
        table[i].hdrSector = -1;
    }
        
}

processfdTable::~processfdTable() {
    CloseAll();
    delete fdtableLock;
}

int processfdTable::Add(OpenFile *f, int hdrSector) {
    if (f == NULL) return -1;
    fdtableLock->Acquire();
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!table[i].used) {
            table[i].used = true;
            table[i].of = f;
            table[i].hdrSector = hdrSector;
            fdtableLock->Release();
            return i;
        }
    }
    fdtableLock->Release();
    return -1; // table full
}

int processfdTable::AddAt(int fd, OpenFile *f, int hdrSector) {
    if (f == NULL) return -1;
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -1;

    fdtableLock->Acquire();
    if (table[fd].used) {
        fdtableLock->Release();
        return -1;
    }
    table[fd].used = true;
    table[fd].of = f;
    table[fd].hdrSector = hdrSector;
    fdtableLock->Release();
    return fd;
}

OpenFile* processfdTable::Get(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return NULL;
    fdtableLock->Acquire();
    OpenFile *res = (table[fd].used) ? table[fd].of : NULL;
    fdtableLock->Release();
    
    return res;
}

// Close = enlève l'entrée + delete l'OpenFile + renvoie hdrSector
// Le syscall fera ensuite systemTable->Close(hdrSector).
int processfdTable::Close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -1;

    fdtableLock->Acquire();
    if (!table[fd].used) {
        fdtableLock->Release();
        return -1;
    }

    OpenFile *f = table[fd].of;
    int hdrSector = table[fd].hdrSector;

    table[fd].used = false;
    table[fd].of = NULL;
    table[fd].hdrSector = -1;

    fdtableLock->Release();

    delete f;
    return hdrSector;
}


void processfdTable::CloseAll() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        Close(i);
    }
}

int processfdTable::GetHdrSector(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -1;

    fdtableLock->Acquire();
    int s = table[fd].used ? table[fd].hdrSector : -1;
    fdtableLock->Release();
    return s;
}

void processfdTable::Print() {
    fdtableLock->Acquire();
    printf("FDTable:\n");
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        printf("  fd=%d used=%d sector=%d\n",
               i, table[i].used ? 1 : 0, table[i].hdrSector);
    }
    fdtableLock->Release();
}

bool processfdTable::IsUsed(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return false;
    fdtableLock->Acquire();
    bool used = table[fd].used;
    fdtableLock->Release();
    return used;
}
