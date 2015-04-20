#include "syscall.h"

int A[32][32];

int main()
{
    int i, j;
    for (i = 0; i < 32; ++i)
        for (j = 0; j < 32; ++j)
            A[j][i] = i * j;
    Exit(A[10][10]);
}