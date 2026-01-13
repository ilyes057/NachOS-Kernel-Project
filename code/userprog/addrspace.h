// addrspace.h
//      Data structures to keep track of executing user programs
//      (address spaces).
//
//      For now, we don't keep any information about address spaces.
//      The user level CPU state is saved and restored in the thread
//      executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "translate.h"
#include "bitmap.h"


class Lock;
class Semaphore;
class List;

#define OneUserStackSize 256 // increase this as necessary!
#define MAX_USER_THREADS 20


struct ThreadState {
    bool used;
    bool finished;
    bool joined;
    Semaphore* sem;

    ThreadState() : used(false), finished(false), joined(false), sem(0) {}
};
struct SemState {
    bool used;
    Semaphore* sem;
};

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable); // Create an address space,
    // initializing it with the program
    // stored in the file "executable"
    ~AddrSpace(); // De-allocate an address space

    void InitRegisters(); // Initialize user-level CPU registers,
    // before jumping to user code

    void SaveState();    // Save/restore address space-specific
    void RestoreState(); // info on a context switch

    Lock *userLock,*semListLock;
    Semaphore *userThreadSem;
    int nbThreads=0;

    int AllocateUserStack(int* outSlot, int* outSp);
    void FreeUserStack(int slot);
    int AllocTid();
    ThreadState* GetRec(int tid);
    void FreeTid(int tid);

    unsigned int semCap;
    SemState* semTable;
    int nextSemId;
    List* freeSemIds;
    void* Sbrk(unsigned int n);

  private:
    void UpgradeTidCapacity(int tid);

    TranslationEntry *pageTable; // Assume linear page table translation
    // for now!
    unsigned int numPages; // Number of pages in the virtual
    // address space
    int stackStartMain;
    BitMap *stackMap;

    ThreadState* tidTable;
    int tidCap;
    int nextTid;
    List* freeTids;
    unsigned int brk;
    unsigned int heapLimit;
};

#endif // ADDRSPACE_H
