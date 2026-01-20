// system.h
//      All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "interrupt.h"
#include "scheduler.h"
#include "stats.h"
#include "thread.h"
#include "timer.h"
#include "utility.h"
#include "synchconsole.h"

#define MAX_STRING_SIZE 256

#define MAX_INIT_PIDS 10
struct ProcessInfo {
    bool valid;           // Remplace ton compteur : si true, le slot compte
    AddrSpace *space;
    Semaphore *waitSem;   // Pour le Wait(pid)
    int exitStatus;
};

// Variables globales pour la gestion des PIDs
extern ProcessInfo* processTable; // Le tableau dynamique
extern int processTableCap;       // Capacité actuelle
extern int nextPid;               // Le compteur pour les nouveaux PIDs (si rien à recycler)
extern List* freePids;            // La liste des PIDs recyclables
extern Lock* processTableLock;    // Verrou obligatoire (car partagé par tous les processus)

extern int AllocPid();     
extern void FreePid(int pid); 
extern void InitProcessSystem();
// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); // Initialization,
                                               // called before anything else
extern void Cleanup();                         // Cleanup, called when
                                               // Nachos is done.

extern Thread *currentThread;       // the thread holding the CPU
extern Thread *threadToBeDestroyed; // the thread that just finished
extern Scheduler *scheduler;        // the ready list
extern Interrupt *interrupt;        // interrupt status
extern Statistics *stats;           // performance metrics
extern Timer *timer;                // the hardware alarm clock
extern SynchConsole *synchconsole;
extern int procCount;
#if defined(STEP4) || defined(STEP5)
class FrameProvider;
extern FrameProvider *frameProvider;
#endif

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine *machine; // user program memory and registers
#endif

#ifdef USER_PROGRAM
#include "synchconsole.h"
extern SynchConsole *synchconsole;
#endif

#ifdef FILESYS_NEEDED // FILESYS or FILESYS_STUB
#include "filesys.h"
extern FileSystem *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk *synchDisk;
#include "filesys.h"
class systemTable;
extern systemTable *sysTable;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice *postOffice;
#endif

#endif // SYSTEM_H
