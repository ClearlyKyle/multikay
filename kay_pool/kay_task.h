#ifndef __TASK_H__
#define __TASK_H__

#include <stdlib.h>

typedef struct task
{
    void (*func)(void *); // Pointer to the function to be executed
    void        *arg;     // Argument to be passed to the function
    struct task *next;    // Pointer to the next task in the list
} task_t;

task_t *create_task(void (*func)(void *), void *arg)
{
    task_t *task = (task_t *)malloc(sizeof(task_t));
    if (task == NULL)
    {
        perror("malloc");
        return NULL;
    }
    task->func = func;
    task->arg  = arg;
    task->next = NULL;
    return task;
}

#endif // __TASK_H__