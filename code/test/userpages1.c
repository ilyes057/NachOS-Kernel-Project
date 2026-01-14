#include "syscall.h"
#define THIS "111"
#define THAT "222"
const int N = 3;

void puts(char *s)
{
    char *p;
    for (p = s; *p != '\0'; p++) PutChar(*p);
}

void f(void *s)
{
    int i;
    for (i = 0; i < N; i++) puts((char *)s);
}

int main()
{
    UserThreadCreate(f, (void *) THIS);
    f((void*) THAT);
    return 0;
}
