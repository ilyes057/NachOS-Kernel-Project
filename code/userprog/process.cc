#include "copyright.h"
#include "system.h"
#include "process.h"
#include "addrspace.h"
#include "filesys.h"
#include "synch.h"

static Lock *procLock = nullptr;
static int procCount = 0;

static void InitProcess() {
    if (procLock == nullptr) {
        procLock = new Lock("procLock");
        procCount = 1;//le processus initial (pere qui fork)
    }
}

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
    OpenFile *executable = fileSystem->Open(filename);
    if (executable == nullptr) return -1;
    //init laddress space apartir de lexecutable
    AddrSpace *space = new AddrSpace(executable);

    delete executable;
    //creer le thread du processus
    Thread *t = new Thread(filename);
    if (t == nullptr) {
        delete space;
        return -1;
    }

    t->space = space;
    //initialise rle processus
    InitProcess();
    procLock->Acquire();
    //conteur de processus sous CS car acces concurrents 
    procCount++;
    procLock->Release();
    //lancer le thread du nouveau processus
    t->Fork(StartProcess, (int)space);

    return 0;
}


void do_ProcessExit() {
    //assure que linit est faite mm si exit est appelle sans que forkexec soit appl avant
    InitProcess();
    //recupere laddress space du thread courant
    AddrSpace *space = currentThread->space;

    //un proc doit attendre tous ses threads avant de se terminer
    space->userLock->Acquire();
    while (space->nbThreads > 0) {
        space->userLock->Release();
        space->userThreadSem->P();
        space->userLock->Acquire();
    }
    space->userLock->Release();

    currentThread->space = nullptr;
    delete space;

    procLock->Acquire();
    procCount--;
    int left = procCount;
    procLock->Release();
    //faire halt uniquement si le processus est le dernier, si non juste terminer le thread en qst
    if (left == 0) {
        interrupt->Halt();
    }
    currentThread->Finish();
    ASSERT(false);
}
