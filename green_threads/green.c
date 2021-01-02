#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include "green.h"

#define FALSE 0
#define TRUE 1
#define STACK_SIZE 4096
#define PERIOD 100

static sigset_t block;

void timer_handler(int);

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE};

static green_t *running = &main_green;
green_t *rq_head; // ready queue head

static void init() __attribute__((constructor));

void init() {
    getcontext(&main_cntx);

    sigemptyset(&block);
    sigaddset(&block, SIGVTALRM);

    struct sigaction act = {0};
    struct timeval interval;
    struct itimerval period;

    act.sa_handler = timer_handler;
    assert(sigaction(SIGVTALRM, &act, NULL) == 0);
    interval.tv_sec = 0;
    interval.tv_usec = PERIOD;
    period.it_interval = interval;
    period.it_value = interval;
    setitimer(ITIMER_VIRTUAL, &period, NULL);
}

void timer_handler(int sig) {
    green_t *susp = running;

    // add the running to the ready queue
    queue_ready_push(susp);

    // find the next thread for execution
    green_t *next = queue_ready_pop();

    running = next;
    swapcontext(susp->context, next->context);
}

// push thread to ready queue 
void queue_ready_push(green_t *new) {
    new->next = rq_head;
    rq_head = new;
}

// pop thread from ready queue
green_t *queue_ready_pop() {

    if(rq_head == NULL) {
        return NULL;
    }

    green_t *current = rq_head;
    if(current->next == NULL) {
        rq_head = NULL;
        return current;
    }

    green_t *prev;
    while(current->next != NULL) {
        prev = current;
        current = current->next;
    }
    prev->next = NULL;
    return current;
}

// push thread to suspended threads queue on provided cond 
void queue_cond_push(green_t *susp, green_cond_t *cond) {
    susp->next = cond->susp_threads;
    cond->susp_threads = susp;
}

// pop thread from suspended threads queue on provided cond
green_t *queue_cond_pop(green_cond_t *cond) {

    if(cond->susp_threads == NULL) {
        return NULL;
    }

    green_t *current = cond->susp_threads;
    if(current->next == NULL) {
        cond->susp_threads = NULL;
        return current;
    }

    green_t *prev;
    while(current->next != NULL) {
        prev = current;
        current = current->next;
    }
    prev->next = NULL;
    return current;
}

// push thread to suspended threads queue on provided mutex 
void queue_mutex_push(green_t *susp, green_mutex_t *mutex) {
    susp->next = mutex->susp_threads;
    mutex->susp_threads = susp;
}

// pop thread from suspended threads queue on provided mutex
green_t *queue_mutex_pop(green_mutex_t *mutex) {

    if(mutex->susp_threads == NULL) {
        return NULL;
    }

    green_t *current = mutex->susp_threads;
    if(current->next == NULL) {
        mutex->susp_threads = NULL;
        return current;
    }

    green_t *prev;
    while(current->next != NULL) {
        prev = current;
        current = current->next;
    }
    prev->next = NULL;
    return current;
}

void green_thread() {
    green_t *this = running;

    void *result = (*this->fun)(this->arg);

    // place waiting (joining) thread in ready queue
    if(this->join != NULL) {
        queue_ready_push(this->join);
    }
    
    // save result of execution
    this->retval = result;

    // we're a zombie
    this->zombie = TRUE;

    // find the next thread to run
    green_t *next = queue_ready_pop();
    assert(next != NULL);

    running = next;
    setcontext(next->context);
}

int green_create(green_t *new, void *(*fun)(void*), void *arg) {
    //sigprocmask(SIG_BLOCK, &block, NULL);
    
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
    queue_ready_push(new);

    //sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_yield() {
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_t *susp = running;
    // add susp to ready queue
    queue_ready_push(susp);

    // select the next thread for execution
    green_t *next = queue_ready_pop();
    assert(next != NULL);
    running = next;
    swapcontext(susp->context, next->context);

    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_join(green_t *thread, void **res) {
    sigprocmask(SIG_BLOCK, &block, NULL);

    if(!thread->zombie) {
        green_t *susp = running;
        // add as joining thread
        thread->join = susp;
        // select the next thread for execution
        green_t *next = queue_ready_pop();
        assert(next != NULL);
        running = next;
        swapcontext(susp->context, next->context);
    }
    // collect result
    if(res != NULL) {
        *res = thread->retval;
    }

    // free context
    free(thread->context);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

void green_cond_init(green_cond_t* cond) {
    cond->susp_threads = NULL;
}

void green_cond_wait(green_cond_t* cond) {
    green_t *susp = running;
    queue_cond_push(susp, cond);

    green_t *next = queue_ready_pop();
    assert(next != NULL);
    running = next;
    swapcontext(susp->context, next->context);
}

void green_cond_signal(green_cond_t* cond) {
    green_t *ready = queue_cond_pop(cond);
    if(ready != NULL) {
        queue_ready_push(ready);
    }   
}

int green_mutex_init(green_mutex_t *mutex) {
    mutex->taken = FALSE;
    mutex->susp_threads = NULL;
}

int green_mutex_lock(green_mutex_t *mutex) {
    // block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_t *susp = running;
    if(mutex->taken) {
        // suspend the running thread
        green_t *susp = running;
        queue_mutex_push(susp, mutex);

        // find the next thread
        green_t *next = queue_mutex_pop(mutex);
        assert(next != NULL);

        running = next;
        swapcontext(susp->context, next->context);
    } else {
        // take the lock
        mutex->taken = TRUE;
    }
    // unblock
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_mutex_unlock(green_mutex_t *mutex) {
    // block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);

    if(mutex->susp_threads != NULL) {
        // move suspended thread to ready queue
        green_t *susp = queue_mutex_pop(mutex);
        queue_ready_push(susp);
    } else {
        // release lock
        mutex->taken = FALSE;
    }
    // unblock
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}
