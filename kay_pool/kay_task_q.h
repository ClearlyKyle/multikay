#ifndef __TASK_Q_H__
#define __TASK_Q_H__

#include <stdbool.h>
#include <pthread.h>

#include "kay_task.h"

typedef struct task_queue
{
    pthread_mutex_t mutex;         // Mutex to protect the queue
    pthread_cond_t  work_cond;     // Signals the threads that there is work to be processed
    pthread_cond_t  working_cond;  // Signals when there are no threads processing
    size_t          working_count; // How many threads are actively processing work
    task_t         *head;          // Head of the linked list of tasks
    task_t         *tail;          // Tail of the linked list of tasks
    bool            exit;          // Exit flag
} task_queue_t;

task_queue_t *task_queue_create(void);                 // Create a new task queue
void          task_queue_destroy(task_queue_t *queue); // Destroy the task queue

void    task_queue_add_task(task_queue_t *queue, task_t *task); // Add a task to the task queue
void    task_queue_wait(task_queue_t *queue);                   // Wait for all tasks to finish
task_t *task_queue_next_task(task_queue_t *queue);              // Get the next tast in the queue

task_queue_t *task_queue_create(void)
{
    task_queue_t *queue = (task_queue_t *)malloc(sizeof(task_queue_t));
    if (queue == NULL)
    {
        perror("malloc");
        return NULL;
    }

    if (pthread_mutex_init(&queue->mutex, NULL) != 0)
    {
        perror("pthread_mutex_init");
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->work_cond, NULL) != 0)
    {
        perror("pthread_cond_init : work_cond");
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->working_cond, NULL) != 0)
    {
        perror("pthread_cond_init : working_cond");
        pthread_cond_destroy(&queue->work_cond);
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        return NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->exit = false; // Initialize the exit flag to false
    return queue;
}

void task_queue_destroy(task_queue_t *queue)
{
    // Set the exit flag for all threads
    queue->exit = 1;

    task_t *task = queue->head;
    while (task != NULL)
    {
        task_t *next = task->next;
        free(task);
        task = next;
    }

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->work_cond);
    pthread_cond_destroy(&queue->working_cond);

    free(queue);
}

void task_queue_add_task(task_queue_t *queue, task_t *task)
{
    if (task == NULL)
    {
        perror("task_t *task is NULL");
        return;
    }

    pthread_mutex_lock(&queue->mutex);
    if (queue->head == NULL)
    {
        // The queue is empty
        queue->head = task;
        queue->tail = task;
    }
    else
    {
        // The queue is not empty
        queue->tail->next = task;
        queue->tail       = task;
    }

    pthread_cond_broadcast(&queue->working_cond); // signal all threads that are waiting
    pthread_mutex_unlock(&queue->mutex);
}

void task_queue_wait(task_queue_t *queue)
{
    if (queue == NULL)
        return;

    pthread_mutex_lock(&(queue->mutex));
    while (1)
    {
        if (!queue->exit && (queue->working_count != 0))
            pthread_cond_wait(&(queue->working_cond), &(queue->mutex));
        else
            break;
    }
    pthread_mutex_unlock(&(queue->mutex));
}

task_t *task_queue_next_task(task_queue_t *queue)
{
    if (queue == NULL)
        return NULL;

    task_t *task = queue->head;
    if (task == NULL)
        return NULL;

    if (task->next == NULL)
        queue->head = queue->tail = NULL;
    else
        queue->head = task->next;

    return task;
}

#endif // __TASK_Q_H__