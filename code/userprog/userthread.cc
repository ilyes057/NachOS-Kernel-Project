
#include "copyright.h"
#include "system.h"
#include "synchconsole.h"
#include "synch.h"
#include "userthread.h"
#include "syscall.h"

typedef struct UserThreadArgs {
    int f;
    int arg;
    int sp;
    int slot;
    int tid;
    int finish;   // adresse de __UserThreadFinish (user)
} UserThreadArgs_t;


static void StartUserThread(int f){
    
    UserThreadArgs *a = (UserThreadArgs*)f;
    int func   = a->f;     
    int arg = a->arg;  
    int sp=a->sp;
    int slot = a->slot; 
    int tid  = a->tid;
    int finish = a->finish;   
    currentThread->userStackSlot = slot;
    currentThread->userTid = tid;
    delete a;

    ASSERT(currentThread->space != nullptr);
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();

    machine->WriteRegister(PCReg, func);
    machine->WriteRegister(NextPCReg, func + 4);
    machine->WriteRegister(4, arg);

    machine->WriteRegister(StackReg, sp);
    machine->WriteRegister(31, finish);      
    machine->Run();
    ASSERT(FALSE);//bcs Run must not return
}


int do_UserThreadCreate(int f, int arg, int finish) {
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
    a->finish = finish;   

    space->userLock->Acquire();
    int slot, sp;
    int ok = space->AllocateUserStack(&slot, &sp);
    if (ok < 0) { 
        space->userLock->Release();
        delete a;
        return -1;
    }
    int tid = space->AllocTid();
    
    a->sp=sp;
    a->slot = slot;
    a->tid = tid;
    space->nbThreads++;
    space->userLock->Release();

    Thread *t = new Thread("user thread");
    if (t == nullptr) {
        space->userLock->Acquire();
        space->FreeUserStack(slot);
        space->FreeTid(tid);
        space->nbThreads--;
        space->userLock->Release();
        delete a;
        return -1;
    }
    //address space is correcctly set in Fork
    t->Fork(StartUserThread,(int) a);
    return tid;
}

void do_UserThreadExit() {
    AddrSpace *space = currentThread->space;
    ASSERT(space != NULL);
    int tid  = currentThread->userTid;
    int slot = currentThread->userStackSlot;
    DEBUG('a', "exit on : tid:%d, slot %d\n", tid, slot);

    space->userLock->Acquire();

    ThreadState* r = space->GetRec(tid);
    if (r != nullptr && r->used) {
        r->finished = true;
        if (r->sem != nullptr) r->sem->V(); //wake the joiner thread
    }

    space->FreeUserStack(slot);
    space->nbThreads--;
    if (space->nbThreads == 0) {
        space->userThreadSem->V(); //wake the process waiting for all threads to finish (halt if it was called
        //or exit from the creator process )
    }
    space->userLock->Release();
    currentThread->Finish();
}

int do_UserThreadJoin(int tid) {
    AddrSpace *space = currentThread->space;
    ASSERT(space != NULL);
    if (tid <= 0) return -1;
    space->userLock->Acquire();
    ThreadState* r = space->GetRec(tid);
    if (r == nullptr || !r->used || r->joined) {
        space->userLock->Release();
        return -1;
    }

    r->joined = true;

    if (r->finished) {
        space->FreeTid(tid);
        space->userLock->Release();
        return 0;
    }

    Semaphore* sem = r->sem;
    space->userLock->Release();

    sem->P();

    space->userLock->Acquire();
    space->FreeTid(tid);
    space->userLock->Release();

    return 0;
}

#ifdef STEP4
int do_sbrk(int n){
    if (n < 0) {
        return -1;
    }
    AddrSpace *space = currentThread->space;
    if (space == nullptr) {
        return -1;
    }
    space->userLock->Acquire();
    void* p = space->Sbrk((unsigned int)n);
    space->userLock->Release();
    if (p == (void*)-1) return -1;
    return (int)(long)p;
}
#endif
