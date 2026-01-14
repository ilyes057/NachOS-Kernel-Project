
#ifndef USERTHREAD_H
#define USERTHREAD_H

#include "copyright.h"

extern int do_UserThreadCreate(int f, int arg, int finish);
extern void do_UserThreadExit();
extern int do_UserThreadJoin(int tid);
extern int do_sbrk(int n);

#endif
