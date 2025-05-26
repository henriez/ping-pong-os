#define _XOPEN_SOURCE 600           
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos-disk-manager.h"

#define ALPHA -1
#define MAX_PRIO -20
#define MIN_PRIO 20

#define QUANTUM 20

unsigned int sysTick;
static struct sigaction timer_action;
static struct itimerval timer;

static void timer_interrupt_handler(int signum);
static void timer_init(void);

task_t* scheduler() {
    PPOS_PREEMPT_DISABLE;
    
    if (readyQueue == NULL) {
        PPOS_PREEMPT_ENABLE;
        return NULL;
    }

    task_t* selectedTask  = readyQueue;
    task_t* taskIterator = readyQueue;

    do {
        if ((taskIterator->priority_dynamic < selectedTask->priority_dynamic) || 
            (taskIterator->priority_dynamic == selectedTask->priority_dynamic  &&
             taskIterator->id < selectedTask->id))
        {
            selectedTask = taskIterator;
        }
        taskIterator = taskIterator->next;
    }
    while (taskIterator != readyQueue);

    if (selectedTask->userTask) {
        // increment all other tasks priority (aging)
        taskIterator = readyQueue;
        do {
            if (taskIterator != selectedTask) {
                    taskIterator->priority_dynamic += ALPHA; 
            }
            taskIterator = taskIterator->next;
        }
        while (taskIterator != readyQueue);

        selectedTask->priority_dynamic = selectedTask->priority_default;
    }

    PPOS_PREEMPT_ENABLE;
    return selectedTask;
}

unsigned int systime() {
    return  sysTick;
}

void task_setprio(task_t *task, int prio) {
    if (task == NULL) {
        task = taskExec;
    }
    
    if (prio < MAX_PRIO) prio = MAX_PRIO;
    if (prio > MIN_PRIO) prio = MIN_PRIO;
    
    task->priority_default  = prio;
    task->priority_dynamic = prio;
}

int task_getprio(task_t *task) {
    if (task == NULL) {
        task = taskExec;
    }

    return task->priority_default;
}


void timer_interrupt_handler(int signum) {
    sysTick++;

    if (!taskExec->userTask) {
        return;
    }    
    taskExec->timeSlice--;
    
    if (taskExec->timeSlice <= 0 && PPOS_IS_PREEMPT_ACTIVE) { 
        task_yield();
    }
}

void timer_init() {
    timer_action.sa_handler = timer_interrupt_handler; 
    sigemptyset(&timer_action.sa_mask);           
    timer_action.sa_flags   = 0;            
    
    if (sigaction(SIGALRM, &timer_action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }
    
    // Timer de 1ms
    timer.it_value.tv_usec = 1000;
    timer.it_value.tv_sec  = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_interval.tv_sec  = 0;
    
    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("Erro em setitimer: ");
        exit(1);
    }
}

void before_ppos_init () {
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif    
}

void after_ppos_init () {
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif

    sysTick = 0;
    timer_init();

    taskMain->userTask = 0;

    PPOS_PREEMPT_ENABLE;
}


void before_task_create (task_t *task ) {
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
    PPOS_PREEMPT_DISABLE;
}

void after_task_create (task_t *task ) {
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
    task->priority_default  = 0;
    task->priority_dynamic = 0;
    task->timeSlice = QUANTUM;

    // 0 and 1 are allways not user tasks (main and dispatcher)
    task->userTask = (task->id > 1);

    task->startTime = systime();     
    task->processTime = 0;             
    task->lastProc = 0;            
    task->activationsCount = 0;             
    task->runningTime = 0;             
    
    PPOS_PREEMPT_ENABLE;
}

void before_task_exit () {
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif

    PPOS_PREEMPT_DISABLE;

}

void after_task_exit () {
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif

    unsigned int task_total_time = systime() - taskExec->startTime;

    printf("Task %d exit: execution time %u ms, processor time %u ms, %u activations\n",
        taskExec->id,           
        task_total_time,        
        taskExec->processTime,    
        taskExec->activationsCount);
}

void before_task_switch ( task_t *task ) {
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
    PPOS_PREEMPT_DISABLE;

    if (task == NULL) return;

    if (taskExec && taskExec->userTask == 1 && taskExec->lastProc > 0) {
        unsigned int elapsedTime = systime() - taskExec->lastProc;
        taskExec->processTime += elapsedTime;
    }
}

void after_task_switch ( task_t *task ) {
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
    if (task == NULL) {
        PPOS_PREEMPT_ENABLE;
        return;
    }

    if (task && task->userTask == 1) {
        task->activationsCount++;
        task->lastProc = systime();
        task->timeSlice = QUANTUM;
    }

    PPOS_PREEMPT_ENABLE;
    if (task && task->userTask == 1) {
        task->timeSlice = QUANTUM; 
    }
    PPOS_PREEMPT_ENABLE;
    
}

void before_task_yield () {
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
    PPOS_PREEMPT_DISABLE;
}

void after_task_yield () {
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}

void before_task_suspend( task_t *task ) {
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
    PPOS_PREEMPT_DISABLE;
}

void after_task_suspend( task_t *task ) {

#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
    PPOS_PREEMPT_ENABLE;
}

void before_task_resume(task_t *task) {
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
    PPOS_PREEMPT_DISABLE;
}

void after_task_resume(task_t *task) {
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
    PPOS_PREEMPT_ENABLE;
}

void before_task_sleep () {
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
    PPOS_PREEMPT_DISABLE;    
}

void after_task_sleep () {
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    PPOS_PREEMPT_DISABLE;
    return 0;
}

int after_task_join (task_t *task) {
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    PPOS_PREEMPT_ENABLE;
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}
