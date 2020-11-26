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
#define MAXLIST 128

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
struct head *flist_8;
struct head *flist_16;
struct head *flist_32;
struct head *flist_64;
int flist_8_count = 0;
int flist_16_count = 0;
int flist_32_count = 0;
int flist_64_count = 0;
struct head *after(struct head *block);

struct head *getlist(int size, struct head *block);
int mergecheck(int size);

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
    struct head *target = getlist(block->size, block);

    if(target != NULL && block != target) {
        block->next = target;
        block->prev = NULL;
        target->prev = block;
        target = block;
    }
    else if(target != NULL && target->next) {
        ;
    }
    else {
        block->next = NULL;
        block->prev = NULL;
        block->free = TRUE;
        struct head *aft = after(block);
        aft->bfree = TRUE;
        target = block;
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
        flist_8 = find((8 + HEAD) * MAXLIST + 2 * HEAD);
        flist_16 = find((16 + HEAD) * MAXLIST + 2 * HEAD);
        flist_32 = find((32 + HEAD) * MAXLIST + 2 * HEAD);
        flist_64 = find((64 + HEAD) * MAXLIST + 2 * HEAD);
        flist->size = flist->size - HEAD;
        flist_8->size = flist_8->size - HEAD;
        flist_16->size = flist_16->size - HEAD;
        flist_32->size = flist_32->size - HEAD;
        flist_64->size = flist_64->size - HEAD;
        struct head *aft = after(flist);
        aft->bsize = flist->size;
        aft->bfree = TRUE;
        aft->size = 0;
        aft->free = FALSE;
        aft = after(flist_8);
        aft->bsize = flist_8->size;
        aft->bfree = TRUE;
        aft->size = 0;
        aft->free = FALSE;
        aft = after(flist_16);
        aft->bsize = flist_16->size;
        aft->bfree = TRUE;
        aft->size = 0;
        aft->free = FALSE;
        aft = after(flist_32);
        aft->bsize = flist_32->size;
        aft->bfree = TRUE;
        aft->size = 0;
        aft->free = FALSE;
        aft = after(flist_64);
        aft->bsize = flist_64->size;
        aft->bfree = TRUE;
        aft->size = 0;
        aft->free = FALSE;
        flist_8->free = TRUE;
        flist_16->free = TRUE;
        flist_32->free = TRUE;
        flist_64->free = TRUE;
        flist_8->bfree = FALSE;
        flist_16->bfree = FALSE;
        flist_32->bfree = FALSE;
        flist_64->bfree = FALSE;
        flist_8->bsize = 0;
        flist_16->bsize = 0;
        flist_32->bsize = 0;
        flist_64->bsize = 0;
        flist_8->next = NULL;
        flist_8->prev = NULL;
        flist_16->next = NULL;
        flist_16->prev = NULL;
        flist_32->next = NULL;
        flist_32->prev = NULL;
        flist_64->next = NULL;
        flist_64->prev = NULL;
        printf("list8 at %p free %d size %d\n", flist_8, flist_8->free, flist_8->size);
        printf("list16 at %p free %d size %d\n", flist_16, flist_16->free, flist_16->size);
        printf("list32 at %p free %d size %d\n", flist_32, flist_32->free, flist_32->size);
        printf("list64 at %p free %d size %d\n", flist_64, flist_64->free, flist_64->size);
        printf("listall at %p free %d size %d\n", flist, flist->free, flist->size);
    }
    struct head *next = getlist(size, NULL);
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

    if (!mergecheck) {
        return block;
    }

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
        
        if (getlist(block->size, NULL) == flist) {
            block = merge(block);
        }

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

struct head *getlist(int size, struct head *block) {
    if (block != NULL && (block == flist_8 || block == flist_16 || block == flist_32 || block == flist_64)) {
        return block;
    }

    struct head *list = flist;

    if (size <= 8 && flist_8_count <= MAXLIST) {
        list = flist_8;
    } else if (size <= 16 && flist_16_count <= MAXLIST) {
        list = flist_16;
    } else if (size <= 32 && flist_32_count <= MAXLIST) {
        list = flist_32;
    } else if (size <= 64 && flist_64_count <= MAXLIST) {
        list = flist_64;
    }

    return list;
}

int mergecheck(int size) {

    if ((size <= 8 && flist_8_count <= 128) || (size > 8 && size <= 16 && flist_16_count <= 128) ||
        (size > 16 && size <= 32 && flist_32_count <= 128) || (size > 32 && size <= 64 && flist_64_count <= 128)) {
        return FALSE;
    }

    return TRUE;
}

void looksie() {
    struct head *test = flist;

    for (int i = 0; i < 32; i++) {   
        printf("nexxe%d %p size %d free %d bsize %d bfree %d\n", i, test, test->size, test->free, test->bsize, test->bfree);
        test = after(test);
    }
}