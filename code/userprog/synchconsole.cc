
#include "copyright.h"
#include "system.h"
#include "synchconsole.h"

static void ReadAvail(int arg) {
    ((SynchConsole*)arg)->ReadAvailHandler();
}

static void WriteDone(int arg) {
    ((SynchConsole*)arg)->WriteDoneHandler();
}

void SynchConsole::ReadAvailHandler() { readAvail->V(); }
void SynchConsole::WriteDoneHandler() { writeDone->V(); }

SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    readLock = new Lock("console read lock");
    writeLock = new Lock("console write lock");
    console = new Console(readFile, writeFile, ReadAvail, WriteDone, (int)this);
    
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete writeLock;
    delete readLock;
    delete writeDone;
    delete readAvail;
}
void SynchConsole::SynchPutChar(const char ch)
{
    writeLock->Acquire();
    console->PutChar(ch);
    writeDone->P();
    writeLock->Release();
}
char SynchConsole::SynchGetChar()
{
    readLock->Acquire();
    readAvail->P();
    char c = console->GetChar();
    readLock->Release();
    return c;
}

void SynchConsole::SynchPutString(const char s[])
{
    writeLock->Acquire();

    int i=0;
    while(s[i]!='\0'){
        console->PutChar(s[i]);
        writeDone->P();
        i++;
    }

     writeLock->Release();
}

void SynchConsole::SynchGetString(char *s, int n)
{
    readLock->Acquire();

    int i=0;

    while (i < n - 1)
    {
        readAvail->P();
        char c = console->GetChar();
        if (c=='\0'){
            break;
        }
        s[i]=(char)c;
        i++;
        if(c=='\n' ){//EOF
            break;
        }
    }
    s[i]='\0';//end of string
    readLock->Release();
}