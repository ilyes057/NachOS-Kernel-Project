// addrspace.cc
//      Routines to manage address spaces (executing user programs).
//
//      In order to run a user program, you must:
//
//      1. link with the -N -T 0 option
//      2. run coff2noff to convert the object file to Nachos format
//              (Nachos object code format is essentially just a simpler
//              version of the UNIX executable object code format)
//      3. load the NOFF file into the Nachos file system
//              (if you haven't implemented the file system yet, you
//              don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "addrspace.h"
#include "copyright.h"
#include "noff.h"
#include "system.h"
#ifdef STEP4
#include "frameprovider.h"
#endif
#include <strings.h> /* for bzero */


static inline void* IntToVoid(int x) { return (void*)(long)(unsigned)x; }
static inline int VoidToInt(void* p) { return (int)(long)p; }

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//      Create an address space to run a user program.
//      Load the program from a file "executable", and set everything
//      up so that we can start executing user instructions.
//
//      Assumes that the object code file is in NOFF format.
//
//      First, set up the translation from program memory to physical
//      memory.  For now, this is really simple (1:1), since we are
//      only uniprogramming, and we have a single unsegmented page table
//
//      "executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------
#ifdef STEP4
static void ReadAtVirtual(OpenFile *executable,
                          int virtualaddr,
                          int numBytes,
                          int position,
                          TranslationEntry *pageTable,
                          unsigned numPages){
    ASSERT(executable != nullptr);
    ASSERT(pageTable != nullptr);
    ASSERT(numBytes >= 0);
    ASSERT(numPages > 0);
    if (numBytes==0)return;
    char *tmp = new char[numBytes];
    int bytesRead = executable->ReadAt(tmp, numBytes, position);
    if (bytesRead <= 0) {
        delete[] tmp;
        return;
    }
    TranslationEntry *oldPT = machine->pageTable;
    unsigned oldPTSize = machine->pageTableSize;

    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;

    for (int i = 0; i < bytesRead; i++) {
        int value = (unsigned char)tmp[i];
        bool ok = machine->WriteMem(virtualaddr + i, 1, value);
        ASSERT(ok);
    }
    machine->pageTable = oldPT;
    machine->pageTableSize = oldPTSize;

    delete[] tmp;
}
#endif

//----------------------------------------------------------------------
// SwapHeader
//      Do little endian to big endian conversion on the bytes in the
//      object file header, in case the file was generated on a little
//      endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}
AddrSpace::AddrSpace(OpenFile *executable) {
    NoffHeader noffH;
    unsigned int i, size;

    userLock =new Lock("userLock");
    userThreadSem = new Semaphore("userThreadSem", 0);
    stackMap = new BitMap(MAX_USER_THREADS);
    stackMap->Mark(0);

    tidCap = 32;
    tidTable = new ThreadState[tidCap];
    for (int k = 0; k < tidCap; k++) {
        tidTable[k].used = false;
        tidTable[k].finished = false;
        tidTable[k].joined = false;
        tidTable[k].sem = nullptr;
    }

    nextTid = 1;
    freeTids = new List;
    tidTable[0].used = true;
    tidTable[0].finished = false;
    tidTable[0].joined = true;
    tidTable[0].sem = nullptr;

    semCap = 32;
    semTable = new SemState[semCap];
    for (i = 0; i < semCap; i++) {
        semTable[i].used = false;
        semTable[i].sem = nullptr;
    }
    nextSemId = 1;
    freeSemIds = new List;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size +
           OneUserStackSize * MAX_USER_THREADS; // we need to increase the size
    // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages); // check we're not trying
    // to run anything too big --
    // at least until we have
    // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages,
          size);
    // first, set up the translation
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        #ifndef STEP4
        pageTable[i].virtualPage = i; // for now, virtual page # = phys page #
        pageTable[i].physicalPage = i;
        #else
        int frame = frameProvider->GetEmptyFrame();
        ASSERT(frame >= 0);
        pageTable[i].virtualPage = i; // for now, virtual page # = phys page #
        pageTable[i].physicalPage = frame;
        #endif
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE; // if the code segment was entirely on
                                       // a separate page, we could set its
                                       // pages to be read-only
    }

    // zero out the entire address space, to zero the unitialized data segment
    // and the stack segment
    #ifndef STEP4
    bzero(machine->mainMemory, size);
    #endif
    // then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
              noffH.code.virtualAddr, noffH.code.size);
        #ifndef STEP4
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
                           noffH.code.size, noffH.code.inFileAddr);
        #else
        ReadAtVirtual(executable,
                  noffH.code.virtualAddr,
                  noffH.code.size,
                  noffH.code.inFileAddr,
                  pageTable, numPages);
        #endif
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
              noffH.initData.virtualAddr, noffH.initData.size);
        #ifndef STEP4
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
                           noffH.initData.size, noffH.initData.inFileAddr);
        #else
        ReadAtVirtual(executable,
                  noffH.initData.virtualAddr,
                  noffH.initData.size,
                  noffH.initData.inFileAddr,
                  pageTable, numPages);
        #endif

    }
}

AddrSpace::~AddrSpace() {
    #ifdef STEP4
    if (pageTable != nullptr) {
        for (unsigned i = 0; i < numPages; i++) {
            if (pageTable[i].valid) {
                frameProvider->ReleaseFrame(pageTable[i].physicalPage);
                pageTable[i].valid = FALSE;
            }
        }
    }
    #endif
    // LB: Missing [] for delete
    // delete pageTable;
    delete[] pageTable;
    pageTable = nullptr;
    if (tidTable != nullptr) {
        for (int i = 0; i < tidCap; i++) {
            if (tidTable[i].sem != nullptr) {
                delete tidTable[i].sem;
                tidTable[i].sem = nullptr;
            }
        }
        delete[] tidTable;
        tidTable = nullptr;
    }

    if (freeTids != nullptr) {
        delete freeTids;
        freeTids = nullptr;
    }
    if (semTable != nullptr) {
        for (unsigned int i = 0; i < semCap; i++) {
            if (semTable[i].sem != nullptr) {
                delete semTable[i].sem;
                semTable[i].sem = nullptr;
            }
        }
        delete[] semTable;
        semTable = nullptr;
    }
    if (freeSemIds != nullptr) {
        delete freeSemIds;
        freeSemIds = nullptr;
    }
    delete userThreadSem;
    delete userLock;
    delete stackMap;
    // End of modification
}
int AddrSpace::AllocateUserStack(int* outSlot, int* outSp) {
    int slot = stackMap->Find();
    if (slot < 0) {
        return -1;
    }
    int sp = stackStartMain - slot * OneUserStackSize;
    sp &= ~0x3; // keep word alignment

    *outSlot = slot;
    *outSp = sp;
    return 0;
}

void AddrSpace::FreeUserStack(int slot) {
    if (slot < 0 || slot >= MAX_USER_THREADS) {
        return;
    }
    if (!stackMap->Test(slot)) {
        return;
    }
    stackMap->Clear(slot);
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//      Set the initial values for the user-level register set.
//
//      We write these directly into the "machine" registers, so
//      that we can immediately jump to user code.  Note that these
//      will be saved/restored into the currentThread->userRegisters
//      when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    stackStartMain=numPages * PageSize - 16;
    machine->WriteRegister(StackReg, stackStartMain);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//      On a context switch, save any machine state, specific
//      to this address space, that needs saving.
//
//      For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//      On a context switch, restore the machine state so that
//      this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}


void AddrSpace::UpgradeTidCapacity(int tid) {
    if (tid < tidCap) return;

    int newCap = tidCap;
    while (newCap <= tid) newCap *= 2;

    ThreadState* newTab = new ThreadState[newCap];
    for (int i = 0; i < newCap; i++) {
            newTab[i].used = false;
            newTab[i].finished = false;
            newTab[i].joined = false;
            newTab[i].sem = nullptr;
        }

    for (int i = 0; i < tidCap; i++) newTab[i] = tidTable[i];

    delete[] tidTable;
    tidTable = newTab;
    tidCap = newCap;
}
ThreadState* AddrSpace::GetRec(int tid) {
    if (tid < 0 || tid >= tidCap) return nullptr;
    return &tidTable[tid];
}
int AddrSpace::AllocTid() {
    int tid;
    //on recup un tid dans la free list si un tid est pret a etre reutilise, on met null si non
    void* v = (freeTids != nullptr) ? freeTids->Remove() : nullptr;
    if (v != nullptr) {
        //reutilisation d'un tid
        tid = VoidToInt(v);
    } else {
        //nouveau tid
        tid = nextTid++;
    }
    //on agrandit le tableau si necessaire
    UpgradeTidCapacity(tid);

    //fill the threads table record
    ThreadState* r = &tidTable[tid];
    r->used = true;
    r->finished = false;
    r->joined = false;
    r->sem = new Semaphore("joinSem", 0);

    return tid;
}

void AddrSpace::FreeTid(int tid) {
    if (tid <= 0) return; // ne recycle pas 0
    if (tid < 0 || tid >= tidCap) return;

    ThreadState* r = &tidTable[tid];
    if (r == nullptr || !r->used) return;

    r->used = false;
    r->finished = false;
    r->joined = false;

    if (r->sem != nullptr) {
        delete r->sem;
        r->sem = nullptr;
    }

    if (freeTids != nullptr) {
        freeTids->Append(IntToVoid(tid));
    }
}
