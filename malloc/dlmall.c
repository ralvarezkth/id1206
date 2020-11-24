

#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdbool.h>

#define TRUE 1
#define FALSE 0

#define HEAD (sizeof(struct head))
#define MIN(size) (((size)>(8))?(size):(8))
#define LIMIT(size) (MIN(0) + HEAD + size)
#define MAGIC(memory) ((struct head*)memory -1) // buggy, don't use
#define HIDE(block) (void*)((struct head*)block +1) // buggy, don't use
#define ALIGN 8
#define ARENA (64*1024)

struct head {
    uint16_t bfree; // 2 bytes, the status of block before
    uint16_t bsize; // 2 bytes, the size of block before
    uint16_t free;  // 2 bytes, the status of the block
    uint16_t size;  // 2 bytes, the size (max 2^16 i.e. 64 Ki byte)
    struct head *next;  // 8 bytes pointer
    struct head *prev;  // 8 bytes pointer
};

struct head *arena = NULL;
struct head *flist;
void getStats();

void detach(struct head *block) {

    if(block->next != NULL) {
        struct head *next = block->next;
        if(block->prev != NULL) {
            struct head *prev = block->prev;
            prev->next = block->next;
            next->prev = block->prev;
            
            block->next = NULL;
            block->prev = NULL;
        }
        else {
            next->prev = NULL;
            flist = next;

            block->next = NULL;
            block->prev = NULL;
        }
    }

    if(block->prev != NULL) {
        struct head *prev = block->prev;
        prev->next = NULL;

        block->next = NULL;
        block->prev = NULL; 
    }
    else {
        flist = NULL;
    }
}

void insert(struct head *block) {
    block->next = flist;
    block->prev = NULL;
    if(flist != NULL) {
        flist->prev = block;
        flist->bfree = TRUE;
    }
    flist = block;
}

struct head *after(struct head *block) {
    return (struct head*)((char *)block + block->size + HEAD);
}

struct head *before(struct head *block) {
    return (struct head*)((char *)block - HEAD - block->size);
}

struct head *split(struct head *block, uint16_t size) {

    uint16_t rsize = block->size - size - HEAD;
    block->size = rsize;

    struct head *splt = after(block);
    splt->bsize = block->size;
    splt->bfree = TRUE;
    splt->size = size;
    splt->free = FALSE;

    struct head *aft = after(splt);
    aft->bsize = splt->size;
    aft->bfree = FALSE;

    return splt;
}

struct head *new() {

    if(arena != NULL) {
        printf("one arena already allocated\n");
        return NULL;
    }

    // using mmap, but could have used sbrk
    struct head *new = mmap(NULL, ARENA, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(new == MAP_FAILED) {
        printf("mmap failed: error %d\n", errno);
        return NULL;
    }

    /* make room for head and dummy */
    uint16_t size = ARENA - 2*HEAD;

    new->bfree = FALSE;
    new->bsize = 0;
    new->free = TRUE;
    new->size = size;
    new->next = NULL;
    new->prev = NULL;

    struct head *sentinel = after(new);

    /* only touch the status fields */
    sentinel->bfree = TRUE;
    sentinel->bsize = size;
    sentinel->free = FALSE;
    sentinel->size = 0;
    sentinel->next = NULL;
    sentinel->prev = NULL;

    /* this is the only arena we have */
    arena = (struct head *)new;

    return new;
}

int adjust(uint16_t request) {
     if(MIN(request)%ALIGN) {
         return MIN(request) + ALIGN - MIN(request)%ALIGN;
     }
     return (uint16_t)MIN(request);
}

struct head *find(uint16_t size) {
    if(flist == NULL) {
        flist = new();
    }
    struct head *next = flist;
    struct head *block;
    while(next != NULL) {
        if(next->size >= size) {
            printf("find: block found with size: %d\n", (int)next->size);
            if(next->size >= LIMIT(size)) {
                printf("find: splitting the block!\n");
                block = split(next, size);
                printf("find: returning block with size: %d\n", (int)block->size);
                return block;
            }
            else {
                block = next;
                detach(block);
                block->free = FALSE;
                struct head *aft = after(block);
                aft->bfree = FALSE;
                return block;
            }
        }
        next = next->next;
    }

    return NULL;
}
/*
struct head *merge(struct head *block) {

    struct head *aft = after(block);

    if(block->bfree) {
        /* inlink the block before */

        /* calculate and set the total size of the merged blocks */

        /* update the block after the merged blocks */

        /* continue with the merged block */
        //block = ...
   // }

  //  if(aft->free) {
        /* unlink the block */

        /* calculate and set the total size of merged blocks */

        /* update the block after the merged block */
 //   }
 //   return block;
//}

void getStats() {
    int length = 0;
    struct head *block = flist;
    while(block != NULL) {
        length++;
        block = block->next;
    }
    int sizes[length];
    block = flist;
    for(int i = 0; i < length; i++) {
        if(block != NULL){
           sizes[i] = (int)block->size; 
        }
        block = block->next;
    }
    printf("********************************************************************\n");
    printf("   Freelist stats\n");
    printf("---------------------\n\n");
    printf("\tFreelist length: %d\n\n", length);
    printf("\tblock #\t\tsize\t\n");
    for(int i = 0; i < length; i++) {
        printf("\t   %d\t\t %d\n", i, sizes[i]);
    }
    printf("\n********************************************************************\n");
}

void *dalloc(size_t request) {
    if(request <= 0){
        return NULL;
    }
    uint16_t size = adjust((uint16_t)request);
    printf("\ndalloc: new block requested!\n");
    printf("dalloc: adjusted size %d -> %d\n", (int)request, (int)size);
    getStats();
    struct head *taken = find(size);
    if(taken == NULL){
        printf("dalloc: returning NULL!\n");
        return NULL;
    }
    else {
        void *memory = (void *)((char *)taken + HEAD);
        return memory;
    }
}


void dfree(void *memory) {

    if(memory != NULL) {
        struct head *block = (struct head *)((char *)memory - HEAD);

        printf("dfree: block returned, size: %d\n", (int)block->size);
        getStats();
        struct head *aft = after(block);
        //merge(block);
        block->free = TRUE;
        aft->bfree = TRUE;
        aft->bsize = block->size;
        insert(block);
    }   
}

void sanity() {

    bool free_flag = false;
    bool size_flag = false;
    bool consec_flag = false; 

    struct head *current = arena;
    struct head *next;
    // check for errors, set flag if error is found
    while(current != NULL) {
        next = after(current);
        if(next->size == 0) { // header after last block reached
            break;
        }
        
        if(current->free != next->bfree){
            free_flag = true;
        }

        if(current->size != next->bsize) {
            size_flag = true;
        }

        if(current->free == next->free) {
            consec_flag = true;
        }

        current = next;
    }


    // print errors
    printf("\n********************************************************************\n");
    printf("   Sanity check\n");
    printf("-------------------\n\n");
    if (free_flag || size_flag || consec_flag) {
        printf("   Sanity check failed:\n\n");

        if(free_flag){
            printf("   - Consecutive blocks have different values for free and bfree!\n");
        }
        if(size_flag) {
            printf("   - Consecutive blocks have different values for size and bsize!\n");
        }
        if(consec_flag) {
            printf("   - Two consecutive free blocks detected! Should be merged!\n");
        }
    }
    else {
        printf("   Sanity check returned with no errors!\n");
    }
    printf("\n********************************************************************\n");
}

struct head *getFList() {
    return arena;
}