#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include "green.h"

#include <stdio.h>

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, &main_green, NULL, NULL, FALSE};
static green_t *running = &main_green;
static void init() __attribute__((constructor));

static green_t *queue_last = &main_green;

void init() {
    getcontext(&main_cntx);
}

void green_thread() {
    green_t *this = running;

    void *result = (*this->fun)(this->arg);

    // place waiting (joining) thread in ready queue
    if (this->join != NULL) {
        this->join->next = queue_last->next;
        queue_last->next = this->join;
        queue_last = this->join;
    }
    
    // save result of execution
    this->retval = &result;
     
    // we're a zombie 
    this->zombie = TRUE;
    
    // find the next thread to run
    green_t *next = this->next;
    

    running = next;
    setcontext(next->context);
}

int green_create(green_t *new, void *(*fun)(void *), void *arg) {
    ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(cntx);

    void *stack = malloc(STACK_SIZE);

    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;
    makecontext(cntx, green_thread, 0);

    new->context = cntx;
    new->fun = fun;
    new->arg = arg;
    new->next = NULL;
    new->join = NULL;
    new->retval = NULL;
    new->zombie = FALSE;

    // add new to the ready queue
    new->next = queue_last->next;
    
    queue_last->next = new;
    queue_last = new;
    
    return 0;
}

int green_yield() {
    green_t *susp = running;

    // add susp to ready queue
    queue_last = susp;
    running = running->next;

    // select the next thread for execution
    green_t *next = susp->next;
    while (next->join != NULL && !next->join->zombie) {
        next = next->next;
    }
    
    running = next;
    swapcontext(susp->context, next->context);

    return 0;
}

int green_join(green_t *thread, void **res) {
    if (!thread->zombie) {
        green_t *susp = running;

        // add as joining thread
        susp->join = thread;
        
        // select the next thread for execution
        green_t *next = running->next;
        running = next;
        queue_last = queue_last->next;

        swapcontext(susp->context, next->context);
    }

    // collect result
    res = thread->retval;
    
    // free context
    green_t *prev_clear = queue_last;
    while (prev_clear->next != thread) {
        prev_clear = prev_clear->next;
    }
    prev_clear->next = thread->next;
    thread->next = NULL;
    free(thread->context);

    return 0;
}