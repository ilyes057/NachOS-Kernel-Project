
#include "copyright.h"
#include "system.h"
#include "synchconsole.h"
#include "synch.h"
#include "userthread.h"
#include "syscall.h"

typedef struct UserThreadArgs {
    int f;     // MIPS address of function
    int arg;   // argument to pass
    int sp;
    int slot;
}UserThreadArgs_t;

static void StartUserThread(int f){
    
    UserThreadArgs *a = (UserThreadArgs*)f;
    int func   = a->f;     
    int arg = a->arg;  
    int sp=a->sp;
    int slot = a->slot; 
    currentThread->userStackSlot = slot;
    delete a;

    ASSERT(currentThread->space != nullptr);
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();

    machine->WriteRegister(PCReg, func);
    machine->WriteRegister(NextPCReg, func + 4);
    machine->WriteRegister(4, arg);

    machine->WriteRegister(StackReg, sp);
    machine->Run();
    ASSERT(FALSE);//bcs Run must not return
}


int do_UserThreadCreate(int f, int arg){
    AddrSpace *space = currentThread->space;
    if (space == NULL) {
        return -1;
    }

    UserThreadArgs_t *a = new UserThreadArgs_t;
    if (a == nullptr) {
        return -1;
    }
    a->f   = f;
    a->arg = arg;

    space->userLock->Acquire();
    int slot, sp;
    int ok = space->AllocateUserStack(&slot, &sp);
    if (ok < 0) { 
        space->userLock->Release();
        delete a;
        return -1;
    }
    space->tidUsed[slot]  = true;
    space->finished[slot] = false;
    space->joined[slot]   = false;
    if (space->joinSem[slot] != nullptr) delete space->joinSem[slot];
    space->joinSem[slot] = new Semaphore("joinSem", 0);
    a->sp=sp;
    a->slot = slot;
    space->nbThreads++;
    space->userLock->Release();

    Thread *t = new Thread("user thread");
    if (t == nullptr) {
        space->userLock->Acquire();
        space->FreeUserStack(slot);
        space->tidUsed[slot] = false;
        space->finished[slot] = false;
        space->joined[slot] = false;
    if (space->joinSem[slot] != nullptr) {
        delete space->joinSem[slot];
        space->joinSem[slot] = nullptr;
    }

        space->nbThreads--;
        space->userLock->Release();
        delete a;
        return -1;
    }
    //address space is correcctly set in Fork
    t->Fork(StartUserThread,(int) a);
    return slot;
}

void do_UserThreadExit() {
    AddrSpace *space = currentThread->space;
    ASSERT(space != NULL);
    int tid = currentThread->userStackSlot;

    space->userLock->Acquire();
    if ((tid >= 0 && tid < MAX_USER_THREADS) && space->tidUsed[tid]) {
        space->finished[tid] = true;
        if (space->joinSem[tid] != nullptr) {
            space->joinSem[tid]->V();
        }
    }
    space->FreeUserStack(tid);
    space->nbThreads--;
    if (space->nbThreads == 0) {
        space->userThreadSem->V();
    }
    space->userLock->Release();
    currentThread->Finish();
}

int do_UserThreadJoin(int tid) {
    AddrSpace *space = currentThread->space;
    ASSERT(space != NULL);
    if (!(tid >= 0 && tid < MAX_USER_THREADS) || tid == 0) return -1;
    space->userLock->Acquire();
    if (!space->tidUsed[tid]) {
        space->userLock->Release();
        return -1;
    }
    if (space->joined[tid]) {
        space->userLock->Release();
        return -1;
    }

    space->joined[tid] = true;
    if (space->finished[tid]) {
        space->tidUsed[tid] = false;
        if (space->joinSem[tid] != nullptr) { delete space->joinSem[tid]; space->joinSem[tid] = nullptr; }
        space->userLock->Release();
        return 0;
    }
    Semaphore* sem = space->joinSem[tid];
    space->userLock->Release();

    sem->P();

    space->userLock->Acquire();
    space->tidUsed[tid] = false;
    if (space->joinSem[tid] != nullptr) { delete space->joinSem[tid]; space->joinSem[tid] = nullptr; }
    space->userLock->Release();

    return 0;

}