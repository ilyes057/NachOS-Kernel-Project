#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "addrspace.h"
#include "usersem.h"

static inline SemState* GetSemRecLocked(AddrSpace* space, int id) {
    if (space == nullptr) return nullptr;
    if (id <= 0 || (unsigned int)id >= space->semCap) return nullptr;
    return &space->semTable[id];
}

int do_SemCreate(int initialValue) {
    AddrSpace* space = currentThread->space;
    if (space == nullptr) return -1;
    if (initialValue < 0) return -1;

    space->semListLock->Acquire();

    int id;
    void* v = (space->freeSemIds != nullptr) ? space->freeSemIds->Remove() : nullptr;
    if (v != nullptr) {
        id = (int)(long)v;
    } else {
        id = space->nextSemId++;
    }

    if ((unsigned int)id >= space->semCap) {
        unsigned int newCap = space->semCap;
        while (newCap <= (unsigned int)id) newCap *= 2;

        SemState* newTab = new SemState[newCap];
        for (unsigned int i = 0; i < newCap; i++) {
            newTab[i].used = false;
            newTab[i].sem = nullptr;
        }
        for (unsigned int i = 0; i < space->semCap; i++) {
            newTab[i] = space->semTable[i];
        }

        delete[] space->semTable;
        space->semTable = newTab;
        space->semCap = newCap;
    }

    SemState* r = &space->semTable[id];
    if (r->sem != nullptr) {
        delete r->sem;
        r->sem = nullptr;
    }

    r->used = true;
    r->sem = new Semaphore("userSem", initialValue);

    space->semListLock->Release();
    return id;
}

int do_SemDestroy(int semId) {
    AddrSpace* space = currentThread->space;
    if (space == nullptr) return -1;

    space->semListLock->Acquire();
    SemState* r = GetSemRecLocked(space, semId);
    if (r == nullptr || !r->used || r->sem == nullptr) {
        space->semListLock->Release();
        return -1;
    }

    r->used = false;

    delete r->sem;
    r->sem = nullptr;
    if (space->freeSemIds != nullptr) {
        space->freeSemIds->Append((void*)(long)semId);
    }
    space->semListLock->Release();
    return 0;
}

int do_SemP(int semId) {
    AddrSpace* space = currentThread->space;
    if (space == nullptr) return -1;

    Semaphore* s = nullptr;

    space->semListLock->Acquire();
    SemState* r = GetSemRecLocked(space, semId);
    if (r == nullptr || !r->used || r->sem == nullptr) {
        space->semListLock->Release();
        return -1;
    }
    s = r->sem;
    space->semListLock->Release();

    s->P();
    return 0;
}

int do_SemV(int semId) {
    AddrSpace* space = currentThread->space;
    if (space == nullptr) return -1;

    Semaphore* s = nullptr;

    space->semListLock->Acquire();
    SemState* r = GetSemRecLocked(space, semId);
    if (r == nullptr || !r->used || r->sem == nullptr) {
        space->semListLock->Release();
        return -1;
    }
    s = r->sem;
    space->semListLock->Release();

    s->V();
    return 0;
}
