
#include "copyright.h"
#include "system.h"
#include "synchconsole.h"
#include "synch.h"
#include "userthread.h"
#include "syscall.h"

typedef struct UserThreadArgs {
    int f;     // MIPS address of function
    int arg;   // argument to pass
}UserThreadArgs_t;

static int ComputeUserStackForNewThread() {
    int creatorSP = machine->ReadRegister(StackReg);
    const int pagesBelow = 3;
    int sp = creatorSP - pagesBelow * PageSize;
    
    sp &= ~0x3;

    return sp;
}

static void StartUserThread(int f){
    
    UserThreadArgs *a = (UserThreadArgs*)f;
    int func   = a->f;     
    int arg = a->arg;   
    delete a;

    currentThread->space->InitRegisters();
    ASSERT(currentThread->space != nullptr);
    currentThread->space->RestoreState();

    machine->WriteRegister(PCReg, func);
    machine->WriteRegister(NextPCReg, func + 4);
    machine->WriteRegister(4, arg);

    int sp = ComputeUserStackForNewThread();
    machine->WriteRegister(StackReg, sp);
    machine->Run();
    ASSERT(FALSE);//bcs Run must not return
}

int do_UserThreadCreate(int f, int arg){
    //fork a kernel thread then create address space for the user thread
    Thread *t = new Thread("user thread");
    //t->space = currentThread->space;
    UserThreadArgs_t *a = new UserThreadArgs_t;
    if (a == nullptr) return -1;
    a->f   = f;
    a->arg = arg;
    t->Fork(StartUserThread,(int) a);
    if (t==nullptr) return -1;
    return 0; 
}

void do_UserThreadExit() {
    currentThread->Finish();

}