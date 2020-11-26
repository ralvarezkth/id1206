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

      int *numm = (int *)dalloc(sizeof(int));
      *numm = 5;
      printf("num is: %d (at: %p)\n", *numm, &numm);
      int *numm2 = (int *)dalloc(sizeof(int));
      *numm2 = 6;
      printf("num is: %d (at: %p)\n", *numm2, &numm2);
      int *numm3 = (int *)dalloc(2*sizeof(int));
      *numm3 = 55;
      printf("num is: %d (at: %p)\n", *numm3, &numm3);
      int *num4 = (int *)dalloc(3*sizeof(int));
      *num4 = 533;
      printf("num is: %d (at: %p)\n", *num4, &num4);
      int *num5 = (int *)dalloc(4*sizeof(int));
      *num5 = 5444;
      printf("num is: %d (at: %p)\n", *num5, &num5);
      int *num6 = (int *)dalloc(5*sizeof(int));
      *num6 = 55555;
      printf("num is: %d (at: %p)\n", *num6, &num6);
      int *num7 = (int *)dalloc(6*sizeof(int));
      *num7 = 566666;
      printf("num is: %d (at: %p)\n", *num7, &num7);
      int *num8 = (int *)dalloc(7*sizeof(int));
      *num8 = 5777777;
      printf("num is: %d (at: %p)\n", *num8, &num8);
      int *num9 = (int *)dalloc(8*sizeof(int));
      *num9 = 58888888;
      printf("num is: %d (at: %p)\n", *num9, &num9);
      int *num10 = (int *)dalloc(9*sizeof(int));
      *num10 = 599999999;
      printf("num is: %d (at: %p)\n", *num10, &num10);
      long int *num11 = (long int *)dalloc(7*sizeof(long int));
      *num11 = 511111111111111;
      printf("num is: %ld (at: %p)\n", *num11, &num11);
      long int *num12 = (long int *)dalloc(8*sizeof(long int));
      *num12 = 5111111111111112;
      printf("num is: %ld (at: %p)\n", *num12, &num12);
      long int *num13 = (long int *)dalloc(9*sizeof(long int));
      *num13 = 51111111111111122;
      printf("num is: %ld (at: %p)\n", *num13, &num13);
      long int *num14 = (long int *)dalloc(24*sizeof(long int));
      *num14 = 511111111111111223;
      printf("num is: %ld (at: %p)\n", *num14, &num14);
      printf("LOOKSIE TIME\n");
      looksie();
      printf("LOOKSIE DONE\n");
      dfree(numm);
      dfree(numm2);
      dfree(numm3);
      dfree(num4);
      dfree(num5);
      dfree(num6);
      dfree(num7);
      dfree(num8);
      dfree(num9);
      dfree(num10);
      dfree(num11);
      dfree(num12);
      dfree(num13);
      dfree(num14);

      printf("shitstain\n");
      looksie();

      printf("REALLOC\n");
      
      long int *num111 = (long int *)dalloc(7*sizeof(long int));
      *num111 = 511111111111111;
      printf("num is: %d (at: %p)\n", *num11, &num11);
      looksie();

      int *num15 = (int *)dalloc(sizeof(int));
      *num15 = 7;
      printf("num is: %d (at: %p)\n", *num15, &num15);

    //  looksie();
      // END CRAP

    sanity();

    return 0;
}