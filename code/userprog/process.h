// userprog/process.h
#ifndef PROCESS_H
#define PROCESS_H

int do_ForkExec(int userFilenameAddr);
void do_ProcessExit(int exitStatus);
int do_Wait(int pid);
#endif
