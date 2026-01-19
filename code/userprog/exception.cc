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
static void copyStringFromMachine(int from, char *to, unsigned size)
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
#ifdef FILESYS
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
        #ifdef FILESYS
        case SC_Open: {
            //user address of the name o the file
            int userAddr = machine->ReadRegister(4); 
            //string to store the name of the file
            char name[MAX_STRING_SIZE];
            copyStringFromMachine(userAddr, name, MAX_STRING_SIZE);

            if (currentThread->space == NULL || currentThread->space->fdTable == NULL) 
            { machine->WriteRegister(2, -1); break; }

            //open the file if it exists
            OpenFile *f = fileSystem->Open(name); 
            if (f == NULL){ 
                machine->WriteRegister(2, -1); 
                break; 
            }
            //add file to the process fdtble if there is an available spot
            int fd = currentThread->space->fdTable->Add(f);
            if (fd < 0) {
                delete f;
                machine->WriteRegister(2, -1);
                break;
            }

            machine->WriteRegister(2, fd);
            break;
        }
        case SC_Close: {
            int fd = machine->ReadRegister(4);

            if (currentThread->space == NULL || currentThread->space->fdTable == NULL) {
                machine->WriteRegister(2, -1);
                break;
            }
            //remove the file from fd table
            bool ok = currentThread->space->fdTable->Remove(fd);
            machine->WriteRegister(2, ok ? 0 : -1);
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