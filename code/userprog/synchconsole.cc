
#include "copyright.h"
#include "system.h"
#include "synchconsole.h"
#include "synch.h"
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    console = new Console(readFile, writeFile, ReadAvail, WriteDone, 0);
}
SynchConsole::~SynchConsole()
{
    delete console;
    delete writeDone;
    delete readAvail;
}
void SynchConsole::SynchPutChar(const char ch)
{
    console->PutChar(ch);
    writeDone->P();
}
char SynchConsole::SynchGetChar()
{
    readAvail->P();
    return console->GetChar();
}
void SynchConsole::SynchPutString(const char s[])
{
    int i=0;
    while(s[i]!='\0'){
        SynchPutChar(s[i]);
        i++;
    }
}

void SynchConsole::SynchGetString(char *s, int n)
{
    int i=0;

    while (i < n - 1)
    {
        /* code */
        char c =SynchGetChar();
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
    
}