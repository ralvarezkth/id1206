#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "green.h"

#include <stdio.h>

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096
#define PERIOD 100

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, &main_green, NULL, NULL, FALSE};
static green_t *running = &main_green;
static void init() __attribute__((constructor));
static sigset_t block;

static green_t *queue_last = &main_green;

void timer_handler(int);

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

void green_cond_init(green_cond_t *cond) {
    *cond = (green_cond_t){NULL, NULL};
}

int green_mutex_init(green_mutex_t *mutex) {
    *mutex = (green_mutex_t){FALSE, NULL, NULL};

    return 0;
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
    // if (this->next != NULL) 
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
    green_t *next = susp->next;

    // add susp to ready queue
    queue_last = susp;
    
    // select the next thread for execution
    while (next->join != NULL && !next->join->zombie) {
        next = next->next;
        queue_last = queue_last->next;
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
        while (!susp->join->zombie) {
            green_t *next = running->next;
            running = next;
            queue_last = queue_last->next;

            swapcontext(susp->context, next->context);
        }
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

void green_cond_wait(green_cond_t *cond, green_mutex_t *mutex) {
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_t *susp = running;
    green_t *next = running->next;
    
    if (susp == next) {
        
        return;
    }

    if (mutex->first != NULL) {
        
        green_mutex_unlock(mutex);
    }

    if (cond->first == NULL) {
        
        cond->first = susp;
        susp->next = susp;
        cond->last = susp;
    } else if (cond->first == cond->last) {
        
        cond->first->next = susp;
        susp->next = cond->first;
        cond->last = susp;
    } else {
        
        cond->last->next = susp;
        susp->next = cond->first;
        cond->last = susp;
    }

    queue_last->next = next;
    running = next;
    swapcontext(susp->context, next->context);

    if (mutex->first != NULL) {
        
        green_mutex_lock(mutex);
    }

    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

void green_cond_signal(green_cond_t *cond) {
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_t *ready = cond->first;
    green_t *next;
    
    if (ready == NULL) {  
        return;
    }

    next = cond->first == cond->last ? NULL : cond->first->next;
    
    queue_last->next = ready;
    ready->next = running;
    queue_last = ready;
    cond->first = next;

    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

void timer_handler(int sig) {
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_yield();
    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

int green_mutex_lock(green_mutex_t *mutex) {
    sigprocmask(SIG_BLOCK, &block, NULL);
    
    if (mutex == NULL) {
        return -1;
    }

    green_t *susp = running;
    mutex_entity *entity = mutex->first;
    green_t *next;
    green_t *last;
    
    if (mutex->taken) {
        int found = 0;
        int no = 0;
        

        if (entity->thread == susp) {
            
            found = 1;
        }

        entity = entity->next;
        while (entity != mutex->first) {
            
            if (entity->thread == susp) {
                
                found = 1;
                break;
            }
            no++;
            entity = entity->next;
        }      

        if (!found) {
            
            entity = malloc(sizeof(mutex_entity));
            entity->thread = susp;
            mutex->last->next = entity;
            entity->next = mutex->first;
            mutex->last = entity;
        }        
        
        next = entity->next->thread;
        running = next;
        last = running;
        while (last->next != running) {
            last = last->next;
        }
        queue_last = last;
        swapcontext(susp->context, next->context);

        green_mutex_lock(mutex);

    } else {
        
        entity = malloc(sizeof(mutex_entity));
        mutex->taken = TRUE;
        entity->thread = susp;

        if (mutex->first != NULL) {
            
            entity->next = mutex->first;
            mutex->last->next = entity;
            mutex->first = entity;
        } else {
            
            mutex->first = entity;
            mutex->first->next = entity;
            mutex->last = entity;
        }
    }

    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_mutex_unlock(green_mutex_t *mutex) {
    sigprocmask(SIG_BLOCK, &block, NULL);

    if (mutex == NULL || mutex->first == NULL) {
        return -1;
    }

    mutex_entity *entity = mutex->first->next;

    mutex->taken = FALSE;
    if (entity == mutex->first) {
        
        mutex->first = NULL;
        mutex->last = NULL;
        free(entity);
    } else {
        
        mutex->last->next = entity;
        mutex->first = entity;
        free(entity);
    }

    
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}