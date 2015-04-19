#include "syscall.h"

int A[3]; 

int 
main() 
{
    int i, j, temp;
    for (i = 0; i < 3; ++i)
        A[i] = 2 - i;
    for (i = 0; i < 2; ++i) 
         for (j = 0; j < (2 - i); ++j) {
            if (A[j] > A[j+1]) {
                temp = A[j];
                A[j] = A[j+1];
                A[j+1] = temp;
            }
         }
    Exit(A[0]);
    //Exit(0);
}