/*
* gcc -c dlmall.c
* gcc -o test dlmall.o test.c
*/

#include "dlmall.h"
#include <stdio.h>

int main() {

    int *num = (int *)dalloc(sizeof(int));
    *num = 5;
    printf("num is: %d (at: %p)\n", *num, &num);
    dfree(num);
    int *num2 = (int *)dalloc(10*sizeof(int));
    *num2 = 10;
    printf("num2 is: %d (at: %p)\n", *num2, &num2);
    dfree(num2);
    char *text = (char *)dalloc(sizeof(char));
    *text = 'A';
    printf("text contains: %c (at: %p)\n", *text, &text);
    dfree(text);

    sanity();

    return 0;
}