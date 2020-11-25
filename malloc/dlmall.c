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
struct head *after(struct head *block);

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

    else if(block->prev != NULL) {
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
    if(flist != NULL && block != flist) {
        block->next = flist;
        block->prev = NULL;
        flist->prev = block;
        flist = block;
    }
    else if(flist != NULL && flist->next) {
        ;
    }
    else {
        block->next = NULL;
        block->prev = NULL;
        block->free = TRUE;
        struct head *aft = after(block);
        aft->bfree = TRUE;
        flist = block;
    }
}

struct head *after(struct head *block) {
    return (struct head*)((char *)block + block->size + HEAD);
}

struct head *before(struct head *block) {
    return (struct head*)((char *)block - (HEAD + block->bsize));
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

    struct head *new = mmap(NULL, ARENA, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
 
    if(new == MAP_FAILED) {
        printf("mmap failed: error %d\n", errno);
        return NULL;
    }

    uint16_t size = ARENA - 2*HEAD;

    new->bfree = FALSE;
    new->bsize = 0;
    new->free = TRUE;
    new->size = size;
    new->next = NULL;
    new->prev = NULL;

    struct head *sentinel = after(new);

    sentinel->bfree = TRUE;
    sentinel->free = FALSE;

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
            if(next->size >= LIMIT(size)) {
                block = split(next, size);
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

struct head *merge(struct head *block) {
    struct head *aft = after(block);
    struct head *bef = before(block);

    if(block->bfree && aft->free) { // merge with blocks before and after
        bef->size += block->size + aft->size + 2*HEAD;
        if(flist == bef){
            if(aft->next){
                bef->next = aft->next;
                aft->next->prev = bef;
            }
            else {
                bef->next = NULL;
            }
        }
        else {
            if(aft->next) {
                bef->prev->next = aft->next;
                aft->next->prev = bef->prev;
            }
            else {
                bef->prev->next = NULL;
            }
        }
        aft->next = NULL;
        aft->prev = NULL;
        block->next = NULL;
        block->prev = NULL;
        block = bef;
    }
    else if (block->bfree) { // merge with block before
        bef->size += block->size + HEAD;
        if(flist == bef && flist->next) {
            flist = flist->next;
        }
        else if(bef->next) {
            bef->prev->next = bef->next;
            bef->next->prev = bef->prev;
        }
        else {
            flist->next = NULL;
        }
        block->next = NULL;
        block->prev = NULL;
        block = bef;
    }
    else if (aft->free) { // merge with block after
        block->size += aft->size + HEAD;
        if(flist == aft && flist->next) {
            flist = flist->next;
        }
        else if(aft->next) {
            bef->next = aft->next;
            aft->next->prev = bef;
        }
        else {
            bef->next = NULL;
        }
        aft->prev = NULL;
        aft->next = NULL;
       
    } 
    return block;
}

void *dalloc(size_t request) {
    if(request <= 0){
        return NULL;
    }
    uint16_t size = adjust((uint16_t)request);
    struct head *taken = find(size);
    if(taken == NULL){
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
        block = merge(block);
        struct head *aft = after(block);
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

        if(current->free == TRUE && next->free == TRUE) {
            consec_flag = true;
        }

        current = next;
    }


    // print errors
    printf("\n##########################################################################\n");
    printf("#   Sanity check\n");
    printf("#####################\n\n");
    if (free_flag || size_flag || consec_flag) {
        printf("#   Sanity check failed:\n\n");

        if(free_flag){
            printf("#   - Consecutive blocks have different values for free and bfree!\n");
        }
        if(size_flag) {
            printf("#   - Consecutive blocks have different values for size and bsize!\n");
        }
        if(consec_flag) {
            printf("#   - Two consecutive free blocks detected! Should be merged!\n");
        }
    }
    else {
        printf("#   Sanity check returned with no errors!\n");
    }
    printf("\n##########################################################################\n");
}

void insertionSort(int arr[], int n){ 
    int i, key, j; 
    for (i = 1; i < n; i++) { 
        key = arr[i]; 
        j = i - 1; 

        while (j >= 0 && arr[j] > key) { 
            arr[j + 1] = arr[j]; 
            j = j - 1; 
        } 
        arr[j + 1] = key; 
    } 
}

void getFlistStats() {
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
    insertionSort(sizes, length);

    printf("\n##########################################################################\n");
    printf("#   Freelist stats\n");
    printf("#####################\n\n");
    printf("#\tFreelist length: %d\n\n", length);
    printf("#\tblock size\t\t#blocks\t\n");
    int count = 1;
    int num = sizes[0];
    for (int i = 1; i <= length; i++) {
        if(sizes[i] == num) {
            count++;
        }
        else {
            printf("\t    %d\t\t\t  %d\n", num, count);
            count = 1;
            num = sizes[i];
        }
    }
    printf("\n##########################################################################\n");
}

void getBlocks() {
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
    struct head *current = arena;
    printf("---------------------\n");
    printf("   Arena stats\n");
    printf("---------------------\n\n");
    printf("\tblock #\tsize\tfree\tbfree\n");
    int arenaCount = 0;
    while(1) {
        printf("\t%d\t %d\t%d\t%d\n", arenaCount, current->size, current->free, current->bfree);
        current = after(current);
        arenaCount++;
        if(current->size == 0)
            break;
    }
    printf("\n********************************************************************\n");

}