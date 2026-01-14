#include "copyright.h"
#include "system.h"
#include "process.h"
#include "addrspace.h"
#include "filesys.h"
#include "synch.h"

static void copyStringFromMachine2(int from, char *to, unsigned size)
{
    if (size == 0) return;
    unsigned i = 0;
    int ch = 0;
    for (; i < size - 1; i++) {
        if (!machine->ReadMem(from + (int)i, 1, &ch)) {
            break;
        }
        to[i] = (char)ch;
        if (to[i] == '\0') {
            return;
        }
    }
    to[size - 1] = '\0';
}

static void StartProcess(int arg)
{   
    //laddr space cree dans ForkExec
    AddrSpace *space = (AddrSpace*) arg;
    ASSERT(space != nullptr);
    //assigne laddr space au thread courant
    currentThread->space = space;
    //initialise les registres et le contexte
    space->InitRegisters();
    space->RestoreState();
    //lancer le programme utilisateur
    machine->Run();
    ASSERT(false); 
}

int do_ForkExec(int userFilenameAddr)
{
    char filename[256];
    copyStringFromMachine2(userFilenameAddr, filename, sizeof(filename));
    
    // Open the executable file
    OpenFile *executable = fileSystem->Open(filename);
    if (executable == nullptr) {
        printf("Unable to open file %s\n", filename);
        return -1;
    }

    // Create new address space
    AddrSpace *space = new AddrSpace(executable);
    delete executable;

    // Create new thread
    Thread *t = new Thread(filename);
    if (t == nullptr) {
        delete space;
        return -1;
    }

    // Allocate PID
    int pid = AllocPid();
    space->pid = pid;
    processTable[pid].space = space; // Although space might be deleted on exit, keeping track can be useful

    t->space = space;

    // Increment global process count
    processTableLock->Acquire();
    procCount++;
    processTableLock->Release();

    // Fork the new thread
    t->Fork(StartProcess, (int)space);

    return pid;
}

void do_ProcessExit(int exitStatus) {
    AddrSpace *space = currentThread->space;
    int pid = space->pid;

    // Complete all threads in this process
    space->userLock->Acquire();
    while (space->nbThreads > 0) {
        space->userLock->Release();
        space->userThreadSem->P();
        space->userLock->Acquire();
    }
    space->userLock->Release();
    
    if (pid != -1) {
        processTableLock->Acquire();
        processTable[pid].exitStatus = exitStatus;
        
        // Wake up waiting parent
        if (processTable[pid].waitSem != nullptr) {
            processTable[pid].waitSem->V();
        }
        processTableLock->Release();
    }

    // Cleanup address space
    currentThread->space = nullptr;
    delete space;

    processTableLock->Acquire();
    procCount--;
    int left = procCount;
    processTableLock->Release();

    if (left == 0) {
        interrupt->Halt();
    }
    
    currentThread->Finish();
    ASSERT(false);
}

int do_Wait(int pid) {
    if (pid < 0 || pid >= processTableCap) return -1;

    processTableLock->Acquire();
    if (!processTable[pid].valid) {
        processTableLock->Release();
        return -1;
    }
    
    // We need to wait.
    Semaphore* sem = processTable[pid].waitSem;
    processTableLock->Release();

    if (sem != nullptr) {
        sem->P();
    }

    processTableLock->Acquire();
    int exitStatus = processTable[pid].exitStatus;
    
    // Now we can free the PId
    processTableLock->Release();
    
    FreePid(pid);

    return exitStatus;
}