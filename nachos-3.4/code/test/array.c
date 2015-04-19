#include "syscall.h"

int A[2048];

int main()
{
    int i;
    for (i = 0; i < 2048; ++i)
        A[i] = 2047 - i;
    Exit(A[2047]);
}