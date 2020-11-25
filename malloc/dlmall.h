#include <stddef.h>

void *dalloc(size_t request);
void dfree(void *memory);
void sanity();
void getBlocks();
void getFlistStats();