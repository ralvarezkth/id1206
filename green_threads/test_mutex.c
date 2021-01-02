#include <stdio.h>
#include <stdlib.h>
#include "green.h"
#include <unistd.h>

int counter = 0;
int loop = 0;

green_mutex_t mutex;

void *test(void *arg) {
    int myid = *(int*)arg;
    for(int i = 0; i < loop; i++) {
        green_mutex_lock(&mutex);
        counter++;
        green_mutex_unlock(&mutex);
    }
}

int main(int argc, char *argv[]) {

    if(argc != 2) {
        fprintf(stderr, "usage: test_lock <loops>\n");
        exit(1);
    }

    loop = atoi(argv[1]);


    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;
    green_mutex_init(&mutex);
    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    printf("counter should be: %d\n", 2*loop);
    printf("counter is: %d\n", counter);
    printf("done\n");
    return 0;
}