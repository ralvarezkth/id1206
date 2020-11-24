#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "rand.h"
#include "dlmall.h"

#define ROUNDS 100
#define BUFFER 100

/*
struct head {
    uint16_t bfree; // 2 bytes, the status of block before
    uint16_t bsize; // 2 bytes, the size of block before
    uint16_t free;  // 2 bytes, the status of the block
    uint16_t size;  // 2 bytes, the size (max 2^16 i.e. 64 Ki byte)
    struct head *next;  // 8 bytes pointer
    struct head *prev;  // 8 bytes pointer
};

int getFLength(){
    int length;
    struct head *block = getFList();
    while(block != NULL && block->size != 1) {
        length++;
        block = block->next;
    }
    return length;
}
*/
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
            printf("dalloc returned NULL!\n\n");
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
    getStats();
    
        
    /*
    int length = getFLength();
    int total;
    // get sizes
    int sizes[length];
    struct head *block = getFList();
    for(int i = 0; i < length; i++) {
        sizes[i] = (int)block->size;
        block = block->next;
        if(block == NULL || block->size == 1) {
            printf("end of freelist reached!\n");
            break;
        }
    }
    // end of get sizes

    printf("*********************************\n*\n");
    printf("* Freelist length: %d elements\n", length);
    printf("* Freelist sizes:\n\n");
    for(int i = 0; i < length; i++) {
        printf("* block %d: %d\n", i, sizes[i]);
        total += sizes[i];
    }
    printf("* total size: %d\n", total);
    printf("*********************************\n\n");

    */

    //sanity();
    
    return 0;
}

