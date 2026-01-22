// exception.cc
//      Entry point into the Nachos kernel from user programs.
//      There are two kinds of things that can cause control to
//      transfer back to here from user code:
//
//      syscall -- The user code explicitly requests to call a procedure
//      in the Nachos kernel.  Right now, the only function we support is
//      "Halt".
//
//      exceptions -- The user code does something that the CPU can't handle.
//      For instance, accessing memory that doesn't exist, arithmetic errors,
//      etc.
//
//      Interrupts (which can also cause control to transfer from user
//      code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "syscall.h"
#include "system.h"
#include "userthread.h"
#include "usersem.h"
#include "process.h"
#ifdef FILESYS
#include "openfile.h"
#include "filesys.h"
#include "systemTable.h"
//static Lock tablePrintLock("tablePrintLock");
#endif

//----------------------------------------------------------------------
// UpdatePC : Increments the Program Counter register in order to resume
// the user program immediately after the "syscall" instruction.
//----------------------------------------------------------------------
static void UpdatePC() {
    int pc = machine->ReadRegister(PCReg);
    machine->WriteRegister(PrevPCReg, pc);
    pc = machine->ReadRegister(NextPCReg);
    machine->WriteRegister(PCReg, pc);
    pc += 4;
    machine->WriteRegister(NextPCReg, pc);
}

//----------------------------------------------------------------------
// ExceptionHandler
//      Entry point into the Nachos kernel.  Called when a user program
//      is executing, and either does a syscall, or generates an addressing
//      or arithmetic exception.
//
//      For system calls, the following is the calling convention:
//
//      system call code -- r2
//              arg1 -- r4
//              arg2 -- r5
//              arg3 -- r6
//              arg4 -- r7
//
//      The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//      "which" is the kind of exception.  The list of possible exceptions
//      are in machine.h.
//----------------------------------------------------------------------
static bool copyStringFromMachine(int from, char *to, unsigned size)
{
    if (size == 0) return false;
    unsigned i = 0;
    int ch = 0;
    for (; i < size - 1; i++) {
        if (!machine->ReadMem(from + (int)i, 1, &ch)) {
            to[i] = '\0';
            return false;
        }
        to[i] = (char)ch;
        if (to[i] == '\0') {
            return true;
        }
    }
    to[size - 1] = '\0';
    return true;
}
#if defined(FILESYS) || defined(NETWORK)
static bool copyBufferFromMachine(int from, char *to, unsigned size) {
    int val = 0;
    for (unsigned i = 0; i < size; i++) {
        if (!machine->ReadMem(from + (int)i, 1, &val)) return false;
        to[i] = (char)val;
    }
    return true;
}

static bool copyBufferToMachine(int to, const char *from, unsigned size) {
    for (unsigned i = 0; i < size; i++) {
        if (!machine->WriteMem(to + (int)i, 1, (int)(unsigned char)from[i])) return false;
    }
    return true;
}
#endif

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);

    if (which == SyscallException) {
        switch (type) {
        case SC_Halt: {
            AddrSpace *space = currentThread->space;
            ASSERT(space != NULL);
            space->userLock->Acquire();
            while (space->nbThreads > 0) {
                space->userLock->Release();
                space->userThreadSem->P();
                space->userLock->Acquire();
            }
            space->userLock->Release();
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;
        }
        case SC_PutChar: {
            ASSERT(synchconsole != NULL);
            int x = machine->ReadRegister(4);
            char c = (char)x;
            synchconsole->SynchPutChar(c);
            break;
        }
        case SC_PutString: {
            int userAddr = machine->ReadRegister(4);
            char *buf = new char[MAX_STRING_SIZE];
            copyStringFromMachine(userAddr, buf, MAX_STRING_SIZE);
            synchconsole->SynchPutString(buf);
            delete[] buf;
            break;
        }
        case SC_Exit: {
            #if !defined(STEP4) && !defined(STEP5)
                int x = machine->ReadRegister(4);
                DEBUG('r', "Shutdown, exit called with status %d.\n", x);
                interrupt->Halt();
            #else
                int status = machine->ReadRegister(4);
                do_ProcessExit(status);  
            #endif
            break;
        }
        case SC_GetChar:{
            char c=synchconsole->SynchGetChar();
            machine->WriteRegister(2, (int)c);
            break;
        }
        case SC_GetString:{
            int userAddr = machine->ReadRegister(4);
            int length = machine->ReadRegister(5);
            char *buf = new char[length + 1];
            synchconsole->SynchGetString(buf, length);
            //copy to machine memory
            for (int i = 0; i < length-1; i++) {
                machine->WriteMem(userAddr + i, 1, buf[i]);
                if(buf[i]=='\0'){break;}
            }
            delete[] buf;
            break;
        }
        case SC_PutInt:{
            int value = machine->ReadRegister(4);
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", value);
            synchconsole->SynchPutString(buf);
            break;
        }
        case SC_GetInt:{
            int userPtr = machine->ReadRegister(4);
            int buf_size =32;
            char buf[buf_size];
            synchconsole->SynchGetString(buf,buf_size);
            int value=0;
            int ok = sscanf(buf, "%d", &value);
            if (ok != 1) value = 0;

            machine->WriteMem(userPtr, 4, value);
            break;
        }
        #if defined(STEP3) || defined(STEP4) || defined(STEP5)
        case SC_UserThreadCreate: {
            int f =  machine->ReadRegister(4);
            int arg = machine->ReadRegister(5);
            int finish = machine->ReadRegister(6);  // adresse wrapper
            int ret = do_UserThreadCreate(f, arg, finish);
            machine->WriteRegister(2, ret);
            break;
        }
        case SC_UserThreadExit: {
            do_UserThreadExit();
            break;
        }
        case SC_UserThreadJoin: {
            int tid = machine->ReadRegister(4);
            int ret = do_UserThreadJoin(tid);
            machine->WriteRegister(2, ret);
            break;
        }
        case SC_SemCreate:{
            int init = machine->ReadRegister(4);
            int id = do_SemCreate(init);
            machine->WriteRegister(2, id);
            break;
        }
        case SC_SemDestroy:{
            int id = machine->ReadRegister(4);
            do_SemDestroy(id);
            break;
        }
        case SC_P:{
            int id = machine->ReadRegister(4);
            do_SemP(id);
            break;
        }
        case SC_V:{
            int id = machine->ReadRegister(4);
            do_SemV(id);
            break;
        }
        #endif
        #if defined(STEP4) || defined(STEP5)
        case SC_ForkExec: {
            int exec = machine->ReadRegister(4);   
            int ret = do_ForkExec(exec);         
            machine->WriteRegister(2, ret);
            break;
        }
        case SC_Wait :{
            int pid = machine->ReadRegister(4);
            int result = do_Wait(pid);
            machine->WriteRegister(2, result);
            break;
        }
        case SC_SBRK:{
            int n = machine->ReadRegister(4);
            int addr = do_sbrk(n);
            machine->WriteRegister(2, addr);
            break;
        }
        #endif
        #ifdef NETWORK
        
        // --- Bas Niveau (VarPost) ---
        case SC_ReseauSend: {
            int to = machine->ReadRegister(4);
            int addrBuf = machine->ReadRegister(5);
            int size = machine->ReadRegister(6);

            char *kBuf = new char[size];
            copyBufferFromMachine(addrBuf, kBuf, size);
            
            vpo->SendLong(to, kBuf, size);
            
            delete[] kBuf;
            break;
        }
        case SC_ReseauReceive: {
            int addrBuf = machine->ReadRegister(4);
            int maxSize = machine->ReadRegister(5);
            int addrFrom = machine->ReadRegister(6);

            int from = -1;
            char *kBuf = NULL;
            // Bloquant
            int recSize = vpo->ReceiveLongAlloc(&from, &kBuf);

            if (recSize > 0) {
                int copySize = (recSize < maxSize) ? recSize : maxSize;
                copyBufferToMachine(addrBuf, kBuf, copySize);
                // Ecrire l'ID de l'expéditeur
                machine->WriteMem(addrFrom, 4, from); 
                
                delete[] kBuf;
                machine->WriteRegister(2, copySize);
            } else {
                machine->WriteRegister(2, -1);
            }
            break;
        }

        // --- FTP (FileTransfer) ---
        case SC_LocalList: {
            printf("\n--- NACHOS FILESYSTEM LIST (DISK) ---\n");
            fileSystem->List(); // Affiche le contenu de l'index du disque simulé
            break;
        }
        case SC_FtpPut: {
            int to = machine->ReadRegister(4);
            int addrL = machine->ReadRegister(5);
            int addrR = machine->ReadRegister(6);
            char l[128], r[128];
            copyStringFromMachine(addrL, l, 128);
            copyStringFromMachine(addrR, r, 128);
            
            fileTransfer->RequestPut(to, l, r);
            break;
        }
        case SC_FtpGet: {
            int to = machine->ReadRegister(4);
            int addrR = machine->ReadRegister(5);
            int addrL = machine->ReadRegister(6);
            char l[128], r[128];
            copyStringFromMachine(addrR, r, 128);
            copyStringFromMachine(addrL, l, 128);
            
            fileTransfer->RequestGet(to, r, l);
            break;
        }
        case SC_FtpList: {
            int to = machine->ReadRegister(4);
            int addrP = machine->ReadRegister(5);
            char p[128];
            copyStringFromMachine(addrP, p, 128);
            fileTransfer->RequestList(to, p);
            break;
        }
        case SC_FtpMkdir: {
            int to = machine->ReadRegister(4);
            int addrP = machine->ReadRegister(5);
            char p[128];
            copyStringFromMachine(addrP, p, 128);
            
            fileTransfer->RequestMkdir(to, p);
            break;
        }
        case SC_FtpRmdir: {
            int to = machine->ReadRegister(4);
            int addrP = machine->ReadRegister(5);
            char p[128];
            copyStringFromMachine(addrP, p, 128);
            
            fileTransfer->RequestRmdir(to, p);
            break;
        }
        case SC_FtpDelete: {
            int to = machine->ReadRegister(4);
            int addrP = machine->ReadRegister(5);
            char p[128];
            copyStringFromMachine(addrP, p, 128);
            
            fileTransfer->RequestDelete(to, p);
            break;
        }
        case SC_FtpStartServer: {
            // Attention: Fonction bloquante (boucle infinie)
            fileTransfer->StartServer(); 
            break;
        }
        
        case SC_LocalCat: {
            int userAddr = machine->ReadRegister(4);
            char fileName[MAX_STRING_SIZE];
            copyStringFromMachine(userAddr, fileName, MAX_STRING_SIZE);

            OpenFile *openFile = fileSystem->Open(fileName); // Ouverture via Nachos
            if (openFile == NULL) {
                printf("Erreur: Fichier '%s' introuvable sur le DISK.\n", fileName);
                break;
            }
            int fileLen = openFile->Length();
            char *buffer = new char[fileLen + 1];
            openFile->Read(buffer, fileLen); // Lecture des secteurs du DISK
            buffer[fileLen] = '\0';

            printf("\n--- Contenu de %s ---\n%s\n", fileName, buffer);

            delete openFile;
            delete[] buffer;
            break;
        }
        #endif // NETWORK
        #ifdef FILESYS
        case SC_Open: {
            int userAddr = machine->ReadRegister(4);
            char name[MAX_STRING_SIZE];

            if (!copyStringFromMachine(userAddr, name, MAX_STRING_SIZE)) {
                machine->WriteRegister(2, -1);
                break;
            }

            if (currentThread->space == NULL || currentThread->space->fdTable == NULL) {
                machine->WriteRegister(2, -1);
                break;
            }

            int sector = fileSystem->FindSector(name);
            if (sector < 0) {
                machine->WriteRegister(2, -1);
                break;
            }

            ////tablePrintLock.Acquire();
            if (!sysTable->Open(sector)) {
                //tablePrintLock.Release();
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *f = new OpenFile(sector);
            if (f == NULL) {
                sysTable->Close(sector);
                //tablePrintLock.Release();
                machine->WriteRegister(2, -1);
                break;
            }

            int fd = currentThread->space->fdTable->Add(f, sector);
            if (fd < 0) {
                delete f;
                sysTable->Close(sector);
                //tablePrintLock.Release();
                machine->WriteRegister(2, -1);
                break;
            }

            sysTable->Print();
            currentThread->space->fdTable->Print();
            //tablePrintLock.Release();
            machine->WriteRegister(2, fd);
            break;
        }
        case SC_Close: {
            int fd = machine->ReadRegister(4);

            if (currentThread->space == NULL || currentThread->space->fdTable == NULL) {
                machine->WriteRegister(2, -1);
                break;
            }

            //tablePrintLock.Acquire();
            int sector = currentThread->space->fdTable->Close(fd);
            if (sector < 0) {
                //tablePrintLock.Release();
                machine->WriteRegister(2, -1);
                break;
            }
            sysTable->Close(sector);
            sysTable->Print();
            currentThread->space->fdTable->Print();
            //tablePrintLock.Release();
            machine->WriteRegister(2, 0);
            break;
        }

        case SC_Read: {
            int userBuf = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);

            if (size < 0) { machine->WriteRegister(2, -1); break; }
            if (size == 0) { machine->WriteRegister(2, 0); break; }

            if (currentThread->space == NULL || currentThread->space->fdTable == NULL) {
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *f = currentThread->space->fdTable->Get(fd);
            if (f == NULL) {
                machine->WriteRegister(2, -1);
                break;
            }

            char *kbuf = new char[size];
            int n = f->Read(kbuf, size); 

            if (n > 0) {
                if (!copyBufferToMachine(userBuf, kbuf, (unsigned)n)) {
                    delete[] kbuf;
                    machine->WriteRegister(2, -1);
                    break;
                }
            }

            delete[] kbuf;
            machine->WriteRegister(2, n);
            break;
        }
        case SC_Write: {
            int userBuf = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);

            if (size < 0) { machine->WriteRegister(2, -1); break; }
            if (size == 0) { machine->WriteRegister(2, 0); break; }

            if (currentThread->space == NULL || currentThread->space->fdTable == NULL) {
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *f = currentThread->space->fdTable->Get(fd);
            if (f == NULL) {
                machine->WriteRegister(2, -1);
                break;
            }

            char *kbuf = new char[size];
            if (!copyBufferFromMachine(userBuf, kbuf, (unsigned)size)) {
                delete[] kbuf;
                machine->WriteRegister(2, -1);
                break;
            }

            int n = f->Write(kbuf, size);
            delete[] kbuf;

            machine->WriteRegister(2, n);
            break;
        }
        case SC_Create: {
            int userAddr = machine->ReadRegister(4);   // name
            int initialSize = machine->ReadRegister(5);

            char name[MAX_STRING_SIZE];
            copyStringFromMachine(userAddr, name, MAX_STRING_SIZE);

            bool ok = fileSystem->Create(name, initialSize);
            machine->WriteRegister(2, ok ? 0 : -1);
            break;
        }


        case SC_Mkdir: {
            int userAddr = machine->ReadRegister(4);

            char name[MAX_STRING_SIZE];
            copyStringFromMachine(userAddr, name, MAX_STRING_SIZE);

            bool ok = fileSystem->MakeDirectory(name);
            machine->WriteRegister(2, ok ? 0 : -1);
            break;
        }

        case SC_Chdir: {
            int userAddr = machine->ReadRegister(4);

            char name[MAX_STRING_SIZE];
            copyStringFromMachine(userAddr, name, MAX_STRING_SIZE);

            bool ok = fileSystem->ChangeDirectory(name);
            machine->WriteRegister(2, ok ? 0 : -1);
            break;
        }
        #endif
        default: {
            printf("Unexpected user mode exception %d %d\n", which, type);
            ASSERT(FALSE);
        }
    }
}

    // LB: Do not forget to increment the pc before returning!
    UpdatePC();
}
