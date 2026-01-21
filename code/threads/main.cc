// main.cc
//      Bootstrap code to initialize the operating system kernel.
//
//      Allows direct calls into internal operating system functions,
//      to simplify debugging and testing.  In practice, the
//      bootstrap code would just initialize data structures,
//      and start a user program to print the login prompt.
//
//      Most of this file is not needed until later assignments.
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//              -s -x <nachos file> -c <consoleIn> <consoleOut>
//              -f -cp <unix file> <nachos file>
//              -p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d causes certain debugging messages to be printed (cf. utility.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//
//  USER_PROGRAM
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -c tests the console
//
//  FILESYS
//    -f causes the physical disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system
//    -t tests the performance of the Nachos file system
//
//  NETWORK
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -o runs a simple test of the Nachos network software
//
//  NOTE -- flags are ignored until the relevant assignment.
//  Some of the flags are interpreted here; some in system.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "system.h"
#include "utility.h"

// External functions used by this file
extern void ThreadTest(void), Copy(const char *unixFile, const char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void StartProcess(char *file), ConsoleTest(char *in, char *out);
extern void SynchConsoleTest(char *in, char *out);
extern void MailTest(int networkID);
extern void FileTransferTest(int farAddr);
extern void ReliablePostTest(int farAddr);
extern void VarPostTest(int farAddr);
extern void RingTest();
extern void InteractiveTest(int farAddr);
//----------------------------------------------------------------------
// main
//      Bootstrap the operating system kernel.
//
//      Check command line arguments
//      Initialize data structures
//      (optionally) Call test procedure
//
//      "argc" is the number of command line arguments (including the name
//              of the command) -- ex: "nachos -d +" -> argc = 3
//      "argv" is an array of strings, one for each command line argument
//              ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int main(int argc, char **argv) {
    int argCount;
    #ifdef NETWORK
    double reliability = 1.0; 
    #endif
    DEBUG('t', "Entering main");
    (void)Initialize(argc, argv);

#if defined(THREADS) && !defined(NETWORK)
    ThreadTest();
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
        argCount = 1;
        if (!strcmp(*argv, "-z")) // print copyright
            printf("%s", copyright);
#ifdef USER_PROGRAM
        if (!strcmp(*argv, "-x")) { // run a user program
            ASSERT(argc > 1);
            StartProcess(*(argv + 1));
            argCount = 2;
        } else if (!strcmp(*argv, "-c")) { // test the console
            if (argc == 1)
                ConsoleTest(NULL, NULL);
            else {
                ASSERT(argc > 2);
                ConsoleTest(*(argv + 1), *(argv + 2));
                argCount = 3;
            }
            interrupt->Halt();
        } else if (!strcmp(*argv, "-sc")) { 
            if (argc == 1)
                SynchConsoleTest(NULL, NULL);
            else {
                ASSERT(argc > 2);
                SynchConsoleTest(*(argv + 1), *(argv + 2));
                argCount = 3;
            }
            interrupt->Halt();
        }
#endif // USER_PROGRAM
#ifdef FILESYS
        if (!strcmp(*argv, "-cp")) { // copy from UNIX to Nachos
            ASSERT(argc > 2);
            Copy(*(argv + 1), *(argv + 2));
            argCount = 3;
        } else if (!strcmp(*argv, "-p")) { // print a Nachos file
            ASSERT(argc > 1);
            Print(*(argv + 1));
            argCount = 2;
        } else if (!strcmp(*argv, "-r")) { // remove Nachos file
            ASSERT(argc > 1);
            bool ok = fileSystem->Remove(*(argv + 1));
            if (ok) printf("Remove '%s' : SUCCESS\n", *(argv + 1));
            else    printf("Remove '%s' : FAILED\n", *(argv + 1));
            argCount = 2;
        } else if (!strcmp(*argv, "-l")) { // list Nachos directory
            fileSystem->List();
        } else if (!strcmp(*argv, "-D")) { // print entire filesystem
            fileSystem->Print();
        } else if (!strcmp(*argv, "-t")) { // performance test
            PerformanceTest();
        } else if (!strcmp(*argv, "-mkdir")) { // make a subdirectory
            ASSERT(argc > 1);
            bool ok = fileSystem->MakeDirectory(*(argv + 1));
            if (ok) printf("mkdir '%s' : SUCCESS\n", *(argv + 1));
            else    printf("mkdir '%s' : FAILED\n", *(argv + 1));
            argCount = 2;
        } else if (!strcmp(*argv, "-cd")) { // change current directory
            ASSERT(argc > 1);
            bool ok = fileSystem->ChangeDirectory(*(argv + 1));
            if (ok) printf("cd '%s' : SUCCESS\n", *(argv + 1));
            else    printf("cd '%s' : FAILED\n", *(argv + 1));
            argCount = 2;
        }
#endif // FILESYS

#ifdef NETWORK
        if (!strcmp(*argv, "-rel")) {
            ASSERT(argc > 1);
            reliability = atof(*(argv + 1)); // Convertit string -> double
            argCount = 2;
        }

        if (!strcmp(*argv, "-o")) {
            ASSERT(argc > 1);
            Delay(2);

            // Appliquer la fiabilité demandée
            // On supprime le PostOffice par défaut et on recrée le bon.
            if (postOffice != NULL) {
                int myAddr = postOffice->GetNetAddr();
                delete postOffice;
                postOffice = new PostOffice(myAddr, reliability, 10);
            }

            int testID = 1; 
            int farAddr = atoi(*(argv + 1));

            // On regarde s'il y a un argument "-t" après
            if (argc > 3 && !strcmp(*(argv + 2), "-t")) {
                 testID = atoi(*(argv + 3));
                 argCount = 4; 
            } else {
                 argCount = 2;
            }

            switch(testID) {
                case 1:
                    printf(">>> Lancement FileTransferTest (Rel: %.2f)\n", reliability);
                    FileTransferTest(farAddr);
                    break;
                case 2:
                    printf(">>> Lancement ReliablePostTest (Rel: %.2f)\n", reliability);
                    ReliablePostTest(farAddr);
                    break;
                case 3:
                    printf(">>> Lancement VarPostTest (Rel: %.2f)\n", reliability);
                    VarPostTest(farAddr);
                    break;
                case 4:
                    printf(">>> Lancement RingTest (Rel: %.2f)\n", reliability);
                    RingTest();
                    break;
                case 5:
                    printf(">>> Lancement InteractiveTest (Rel: %.2f)\n", reliability);
                    InteractiveTest(farAddr);
                    break;
                default:
                    printf("Test ID inconnu. 1=File, 2=Reliable, 3=Var, 4=Ring\n");
                    break;
            }
        }
#endif // NETWORK
    }

    currentThread->Finish();
    return (0);
}