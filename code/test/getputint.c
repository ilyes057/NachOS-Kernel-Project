#include "syscall.h"

int main()
{
    int x=0;
    GetInt(&x);
    PutInt(x);
    PutChar('\n');
    return 0;
}