
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
    a->sp=sp;
    a->slot = slot;
    space->nbThreads++;
    space->userLock->Release();

    Thread *t = new Thread("user thread");
    if (t == nullptr) {
        space->userLock->Acquire();
        space->FreeUserStack(slot);
        space->nbThreads--;
        space->userLock->Release();
        delete a;
        return -1;
    }

    t->Fork(StartUserThread,(int) a);
    return 0;
}

void do_UserThreadExit() {
    AddrSpace *space = currentThread->space;
    ASSERT(space != NULL);

    space->userLock->Acquire();
    space->FreeUserStack(currentThread->userStackSlot);
    space->nbThreads--;
    if (space->nbThreads == 0) {
        space->userThreadSem->V();
    }
    space->userLock->Release();
    currentThread->Finish();
}