#include "syscall.h"

int main()
{
    char buf[10];
    GetString(buf, 10);
    PutString(buf);
    return 0;
}