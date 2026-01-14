// system.cc
//      Nachos initialization and cleanup routines.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "system.h"
#include "copyright.h"
#include "../userprog/frameprovider.h"
// This defines *all* of the global data structures used by Nachos.
// These are all initialized and de-allocated by this file.

Thread *currentThread;       // the thread we are running now
Thread *threadToBeDestroyed; // the thread that just finished
Scheduler *scheduler;        // the ready list
Interrupt *interrupt;        // interrupt status
Statistics *stats;           // performance metrics
Timer *timer;
                // the hardware timer device,
                             // for invoking context switches
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
#endif

#ifdef USER_PROGRAM // requires either FILESYS or FILESYS_STUB
Machine *machine;   // user program memory and registers
SynchConsole *synchconsole;

#endif

#ifdef NETWORK
PostOffice *postOffice;
#endif

// External definition, to allow us to take a pointer to this function
extern void Cleanup();

//----------------------------------------------------------------------
// TimerInterruptHandler
//      Interrupt handler for the timer device.  The timer device is
//      set up to interrupt the CPU periodically (once every TimerTicks).
//      This routine is called each time there is a timer interrupt,
//      with interrupts disabled.
//
//      Note that instead of calling Yield() directly (which would
//      suspend the interrupt handler, not the interrupted thread
//      which is what we wanted to context switch), we set a flag
//      so that once the interrupt handler is done, it will appear as
//      if the interrupted thread called Yield at the point it is
//      was interrupted.
//
//      "dummy" is because every interrupt handler takes one argument,
//              whether it needs it or not.
//----------------------------------------------------------------------
static void TimerInterruptHandler(int dummy) {
    if (interrupt->getStatus() != IdleMode)
        interrupt->YieldOnReturn();
}

//----------------------------------------------------------------------
// Initialize
//      Initialize Nachos global data structures.  Interpret command
//      line arguments in order to determine flags for the initialization.
//
//      "argc" is the number of command line arguments (including the name
//              of the command) -- ex: "nachos -d +" -> argc = 3
//      "argv" is an array of strings, one for each command line argument
//              ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
void Initialize(int argc, char **argv) {
    int argCount;
    const char *debugArgs = "";
    bool randomYield = FALSE;

#ifdef USER_PROGRAM
    bool debugUserProg = FALSE; // single step user program
#endif
#ifdef FILESYS_NEEDED
    bool format = FALSE; // format disk
#endif
#ifdef NETWORK
    double rely = 1; // network reliability
    int netname = 0; // UNIX socket name
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
        argCount = 1;
        if (!strcmp(*argv, "-d")) {
            if (argc == 1)
                debugArgs = "+"; // turn on all debug flags
            else {
                debugArgs = *(argv + 1);
                argCount = 2;
            }
        } else if (!strcmp(*argv, "-rs")) {
            ASSERT(argc > 1);
            RandomInit(atoi(*(argv + 1))); // initialize pseudo-random
            // number generator
            randomYield = TRUE;
            argCount = 2;
        }
#ifdef USER_PROGRAM
        if (!strcmp(*argv, "-s"))
            debugUserProg = TRUE;
#endif
#ifdef FILESYS_NEEDED
        if (!strcmp(*argv, "-f"))
            format = TRUE;
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
    stats = new Statistics();    // collect statistics
    interrupt = new Interrupt;   // start up interrupt handling
    scheduler = new Scheduler(); // initialize the ready queue
    if (randomYield)             // start the timer (if needed)
        timer = new Timer(TimerInterruptHandler, 0, randomYield);

    threadToBeDestroyed = NULL;

    // We didn't explicitly allocate the current thread we are running in.
    // But if it ever tries to give up the CPU, we better have a Thread
    // object to save its state.
    currentThread = new Thread("main");
    currentThread->setStatus(RUNNING);

    interrupt->Enable();
    CallOnUserAbort(Cleanup); // if user hits ctl-C

#ifdef USER_PROGRAM
    machine = new Machine(debugUserProg); // this must come first
    synchconsole = new SynchConsole(NULL, NULL);
#endif

#if defined(STEP4) || defined(STEP5)
    frameProvider = new FrameProvider(NumPhysPages);
    InitProcessSystem();

#endif

#ifdef FILESYS
    synchDisk = new SynchDisk("DISK");
#endif

#ifdef FILESYS_NEEDED
    fileSystem = new FileSystem(format);
#endif

#ifdef NETWORK
    postOffice = new PostOffice(netname, rely, 10);
#endif
}

//----------------------------------------------------------------------
// Cleanup
//      Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------
void Cleanup() {
    printf("\nCleaning up...\n");
#ifdef NETWORK
    delete postOffice;
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
#endif

    delete timer;
    delete scheduler;
    delete interrupt;

    Exit(0);
}


void InitProcessSystem() {
    processTableCap = MAX_INIT_PIDS;
    processTable = new ProcessInfo[processTableCap];
    // On initialise tout à vide
    for (int i = 0; i < processTableCap; i++) {
        processTable[i].valid = false;
        processTable[i].waitSem = nullptr;
    }
    
    nextPid = 1;
    freePids = new List();
    procCount = 0;
    processTableLock = new Lock("Process Table Lock");
}

// Fonction utilitaire pour agrandir le tableau (Miroir de UpgradeTidCapacity)
void UpgradeProcessCapacity(int pid) {
    if (pid < processTableCap) return;

    int newCap = processTableCap;
    while (newCap <= pid) newCap *= 2;

    ProcessInfo* newTab = new ProcessInfo[newCap];
    
    // Init de la nouvelle partie
    for (int i = processTableCap; i < newCap; i++) {
        newTab[i].valid = false;
        newTab[i].waitSem = nullptr;
    }

    // Copie de l'ancienne partie
    for (int i = 0; i < processTableCap; i++) {
        newTab[i].waitSem = processTable[i].waitSem;
        newTab[i].valid = processTable[i].valid;
    }

    delete[] processTable;
    processTable = newTab;
    processTableCap = newCap;
}

// ---------------------------------------------------------
// AllocPid : Logique "First Fit" déterministe
// ---------------------------------------------------------
int AllocPid() {
    processTableLock->Acquire(); // PROTECTION CRITIQUE

    int pid;
    // 1. On regarde si on peut recycler un vieux PID
    void* v = (freePids != nullptr) ? freePids->Remove() : nullptr;

    if (v != nullptr) {
        // RECYCLAGE : On prend le PID qui a été libéré le plus tôt (FIFO)
        pid = (int)((long)v); // Cast compatible 32/64 bits
    } else {
        // NOUVEAU : On incrémente le compteur
        pid = nextPid++;
    }
    // 2. Agrandissement si nécessaire
    UpgradeProcessCapacity(pid);

    // 3. Initialisation du slot
    ProcessInfo* p = &processTable[pid];
    p->valid = true;
    p->exitStatus = 0;
    p->waitSem = new Semaphore("Process Wait Sem", 0);
    // p->space sera assigné par l'appelant (do_ForkExec)

    processTableLock->Release();
    return pid;
}

// ---------------------------------------------------------
// FreePid : Libération et mise en recyclage
// ---------------------------------------------------------
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

    // 1. Nettoyage du slot
    p->valid = false;
    if (p->waitSem != nullptr) {
        delete p->waitSem;
        p->waitSem = nullptr;
    }
    
    // Note: p->space est généralement supprimé avant, dans do_Exit ou do_ProcessExit

    // 2. RECYCLAGE : On ajoute ce PID à la fin de la liste des libres
    // Cela garantit que ce PID sera réutilisé plus tard (Déterminisme)
    if (freePids != nullptr) {
        freePids->Append((void*)((long)pid));
    }

    processTableLock->Release();
}