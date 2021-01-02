#include <stdio.h>
#include <stdlib.h>
#include "green.h"
#include <unistd.h>

int id = -1;
void *test(void *arg) {
    int myid = *(int*)arg;
    int loop = 10000000;
    for(int i = 0; i < loop; i++) {
        if(id == -1){
            printf("thread %d: first to run! wohooo!\n", myid);
            id = myid;
        }  
        if(id != myid) {
            printf("thread %d: i interrupted thread %d\n", myid, id);
            id = myid;
        }
        
    }
}

int main() {
    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;
    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    printf("done\n");
    return 0;
}