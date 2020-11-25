#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rand.h"
#include "dlmall.h"

#define ROUNDS 100
#define BUFFER 100

int main(int argc, char * argv[]) {
    if(argc < 5) {
        printf("usage: bench <min>, <max>, <blocks>, <concurrent blocks>\n");
        exit(1);
    }
    int min = atoi(argv[1]);
    int max = atoi(argv[2]);
    int blocks = atoi(argv[3]);
    int concurrent = atoi(argv[4]);

    void *buffer[concurrent];
    
    for(int i = 0, j = i; i < blocks; i++, j++) {
        int size = request(min, max);
        int * memory = dalloc(size);
        if(memory == NULL) {
            break;
        }
        if(j == concurrent) {
            j = 0;
        }
        if(i >= concurrent) {
            dfree(buffer[j]);
        }
        buffer[j] = memory;
    }

    getBlocks();
    //getFlistStats();
    sanity();
    
    return 0;
}

