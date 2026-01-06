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
        case SC_Exit:{
            int x = machine->ReadRegister(4);
            DEBUG('r', "Shutdown, exit called with status %d.\n",x);
            interrupt->Halt();
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
        #ifdef STEP3
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
