#ifndef USERSEM_H
#define USERSEM_H

int do_SemCreate(int initialValue);
int do_SemDestroy(int semId);
int do_SemP(int semId);
int do_SemV(int semId);

#endif