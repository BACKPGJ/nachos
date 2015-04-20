#include "syscall.h"

int A[32]; 

int 
main() 
{
    int i, j, temp;
    for (i = 0; i < 32; ++i)
        A[i] = 31 - i;
    for (i = 0; i < 31; ++i) 
         for (j = 0; j < (31 - i); ++j) {
            if (A[j] > A[j+1]) {
                temp = A[j];
                A[j] = A[j+1];
                A[j+1] = temp;
            }
         }
    Exit(A[0]);
    //Exit(0);
}