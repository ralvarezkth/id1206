#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "green.h"

int flag = 0; 
green_cond_t cond;
volatile int count = 0;
green_mutex_t mutex;
int j = 0;
// test 2

/*void *test(void *arg) {
    int i = *(int*)arg;
    int loop = 4;

    while (loop > 0) {
        printf("thread %d: %d\n", i, loop);
        loop--;
        green_yield();
    }
}*/

// test 3

/* void *test(void *arg) {
    int id = *(int *)arg;
    int loop = 4;

    while (loop > 0) {
        printf("here test loop %d flag %d\n", loop, flag);
        if (flag == id) {
            printf("thread %d: %d\n", id, loop);
            loop--;
            flag = (id + 1) % 2;
            green_cond_signal(&cond);
        } else {
            green_cond_wait(&cond);
        }
    }
} */

// test 4

/*void *test(void *arg) {
    for (volatile int i = 0; i < 10000000; i++) {
        if (i % 10000) {
            printf("thread %d: %d\n", *(int *)arg, i);
        }
    }
}*/

// test 5

/*void *test(void *arg) {
    green_mutex_lock(&mutex);
    for (volatile int i = 0; i < 1700000; i++) {
        printf("thread %d: i %d\n", *(int *)arg, i);
        count++;        
    }
    green_mutex_unlock(&mutex);
} */

// test 6

void *test(void *arg) {
    int id = *(int *)arg;
    int loop = 100;

    while (loop > 0) {
        green_mutex_lock(&mutex);
        
        if (flag != id) {
            green_cond_wait(&cond, &mutex);
        } else {
            printf("thread %d: loop %d\n", id, loop);
            flag = (id + 1) % 2;
            green_cond_signal(&cond);
            green_mutex_unlock(&mutex);
            loop--;
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
    printf("final count: %d\n", count);

    return 0;
}