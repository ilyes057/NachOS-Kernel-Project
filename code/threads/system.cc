// system.cc
//      Nachos initialization and cleanup routines.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "system.h"
#include "copyright.h"
#include "../userprog/frameprovider.h"
#include <unistd.h>    
#include <sys/stat.h>
#include <sys/types.h>

// --- NOUVEAUX INCLUDES RESEAU ---
#ifdef NETWORK
#include "../network/post.h"
#include "../network/reliablepost.h"
#include "../network/varpost.h"
#include "../network/filetransfer.h"
#endif

#include "systemTable.h"
// This defines *all* of the global data structures used by Nachos.
// These are all initialized and de-allocated by this file.

Thread *currentThread;       // the thread we are running now
Thread *threadToBeDestroyed; // the thread that just finished
Scheduler *scheduler;        // the ready list
Interrupt *interrupt;        // interrupt status
Statistics *stats;           // performance metrics
Timer *timer;                // the hardware timer device

FrameProvider *frameProvider = nullptr;

ProcessInfo* processTable;
int processTableCap;
int nextPid;
List* freePids;
Lock* processTableLock;
int procCount;

#ifdef FILESYS_NEEDED
FileSystem *fileSystem;
#endif

#ifdef FILESYS
SynchDisk *synchDisk;
systemTable *sysTable;
#endif

#ifdef USER_PROGRAM // requires either FILESYS or FILESYS_STUB
Machine *machine;   // user program memory and registers
SynchConsole *synchconsole;
#endif

#ifdef NETWORK
PostOffice *postOffice;
ReliablePostOffice *rpo;
VarPostOffice *vpo;
FileTransfer *fileTransfer;
#endif

// External definition, to allow us to take a pointer to this function
extern void Cleanup();

//----------------------------------------------------------------------
// TimerInterruptHandler
//----------------------------------------------------------------------
static void TimerInterruptHandler(int dummy) {
    if (interrupt->getStatus() != IdleMode)
        interrupt->YieldOnReturn();
}

//----------------------------------------------------------------------
// Initialize
//----------------------------------------------------------------------
void Initialize(int argc, char **argv) {
    int argCount;
    const char *debugArgs = "";
    bool randomYield = FALSE;
    int netname __attribute__((unused)) = -1;

    #ifdef USER_PROGRAM
        bool debugUserProg = FALSE;
    #endif
    #ifdef FILESYS_NEEDED
        bool format = FALSE;
    #endif
    #ifdef NETWORK
        double rely = 1;
    #endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
        argCount = 1;
        if (!strcmp(*argv, "-d")) {
            if (argc == 1)
                debugArgs = "+";
            else {
                debugArgs = *(argv + 1);
                argCount = 2;
            }
        } else if (!strcmp(*argv, "-rs")) {
            ASSERT(argc > 1);
            RandomInit(atoi(*(argv + 1)));
            randomYield = TRUE;
            argCount = 2;
        }
#ifdef FILESYS_NEEDED
        else if (!strcmp(*argv, "-f")) {
            format = TRUE;
        }
#endif
#ifdef NETWORK
        if (!strcmp(*argv, "-l")) {
            ASSERT(argc > 1);
            rely = atof(*(argv + 1));
            argCount = 2;
        } else if (!strcmp(*argv, "-m")) {
            ASSERT(argc > 1);
            netname = atoi(*(argv + 1));
            argCount = 2;
        }
#endif
    }

    DebugInit(debugArgs);        // initialize DEBUG messages
    setvbuf(stdout, NULL, _IONBF, 0);
    stats = new Statistics();    // collect statistics
    interrupt = new Interrupt;   // start up interrupt handling
    scheduler = new Scheduler(); // initialize the ready queue
    if (randomYield)             // start the timer (if needed)
        timer = new Timer(TimerInterruptHandler, 0, randomYield);

    threadToBeDestroyed = NULL;
    currentThread = new Thread("main");
    currentThread->setStatus(RUNNING);

    interrupt->Enable();
    CallOnUserAbort(Cleanup);

#ifdef USER_PROGRAM
    machine = new Machine(debugUserProg);
    synchconsole = new SynchConsole(NULL, NULL);

    #if defined(STEP4) || defined(STEP5) || defined(NETWORK)
    if (frameProvider == nullptr) { // Sécurité anti-double initialisation
        frameProvider = new FrameProvider(NumPhysPages);
        InitProcessSystem(); 
    }
    #endif
#endif

#ifdef FILESYS
    char diskName[32];
    int id = (netname != -1) ? netname : 0;
    sprintf(diskName, "DISK_%d", id); 
    
    synchDisk = new SynchDisk(diskName);
    sysTable = new systemTable(10);
#endif

#ifdef FILESYS_NEEDED
    fileSystem = new FileSystem(format);
#endif

#ifdef NETWORK
    if (netname != -1) {
        postOffice = new PostOffice(netname, rely, 10);
        rpo = new ReliablePostOffice(postOffice, 1);
        vpo = new VarPostOffice(rpo);
        // On initialise le transfert de fichiers avec ce répertoire racine
        fileTransfer = new FileTransfer(vpo, rpo);
        
        
        /* NOTE: On ne fait pas chdir(myRoot) ici car Nachos ne trouverait 
           plus le binaire utilisateur (ex: ftp_client) qui est dans build/.
        */
    }
#endif
}
//----------------------------------------------------------------------
// Cleanup
//----------------------------------------------------------------------
void Cleanup() {
    printf("\nCleaning up...\n");

    // --- NETTOYAGE RESEAU ---
#ifdef NETWORK
    if (fileTransfer) delete fileTransfer;
    if (vpo) delete vpo;
    if (rpo) delete rpo;
    if (postOffice) delete postOffice;
#endif

#if defined(STEP4) || defined(STEP5)
    delete frameProvider;
    frameProvider = nullptr;
#endif

#ifdef USER_PROGRAM
    delete synchconsole;
    delete machine;
#endif

#ifdef FILESYS_NEEDED
    delete fileSystem;
#endif

#ifdef FILESYS
    delete synchDisk;
    delete sysTable;
#endif

    delete timer;
    delete scheduler;
    delete interrupt;

    Exit(0);
}

// ... (Le reste de vos fonctions Process Init, Upgrade, Alloc, Free reste inchangé) ...

void InitProcessSystem() {
    processTableCap = MAX_INIT_PIDS;
    processTable = new ProcessInfo[processTableCap];
    for (int i = 0; i < processTableCap; i++) {
        processTable[i].valid = false;
        processTable[i].waitSem = nullptr;
    }
    
    nextPid = 1;
    freePids = new List();
    procCount = 0;
    processTableLock = new Lock("Process Table Lock");
}

void UpgradeProcessCapacity(int pid) {
    if (pid < processTableCap) return;
    int newCap = processTableCap;
    while (newCap <= pid) newCap *= 2;
    ProcessInfo* newTab = new ProcessInfo[newCap];
    for (int i = processTableCap; i < newCap; i++) {
        newTab[i].valid = false;
        newTab[i].waitSem = nullptr;
    }
    for (int i = 0; i < processTableCap; i++) {
        newTab[i].waitSem = processTable[i].waitSem;
        newTab[i].valid = processTable[i].valid;
    }
    delete[] processTable;
    processTable = newTab;
    processTableCap = newCap;
}

int AllocPid() {
    processTableLock->Acquire();
    int pid;
    void* v = (freePids != nullptr) ? freePids->Remove() : nullptr;
    if (v != nullptr) {
        pid = (int)((long)v);
    } else {
        pid = nextPid++;
    }
    UpgradeProcessCapacity(pid);
    ProcessInfo* p = &processTable[pid];
    p->valid = true;
    p->exitStatus = 0;
    p->waitSem = new Semaphore("Process Wait Sem", 0);
    processTableLock->Release();
    return pid;
}

void FreePid(int pid) {
    processTableLock->Acquire();
    if (pid < 0 || pid >= processTableCap) {
        processTableLock->Release();
        return;
    }
    ProcessInfo* p = &processTable[pid];
    if (!p->valid) {
        processTableLock->Release();
        return;
    }
    p->valid = false;
    if (p->waitSem != nullptr) {
        delete p->waitSem;
        p->waitSem = nullptr;
    }
    if (freePids != nullptr) {
        freePids->Append((void*)((long)pid));
    }
    processTableLock->Release();
}
