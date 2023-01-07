#ifndef __TASK_Q_H__
#define __TASK_Q_H__

#include <stdbool.h>
#include <pthread.h>

#include "kay_task.h"

typedef struct task_queue
{
    pthread_mutex_t mutex; // Mutex to protect the queue
    pthread_cond_t  cond;  // Condition variable to signal new tasks
    task_t         *head;  // Head of the linked list of tasks
    task_t         *tail;  // Tail of the linked list of tasks
    bool            exit;  // Exit flag
} task_queue_t;

// Create a new task queue
task_queue_t *create_task_queue()
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

    if (pthread_cond_init(&queue->cond, NULL) != 0)
    {
        perror("pthread_cond_init");
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        return NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->exit = false; // Initialize the exit flag to false
    return queue;
}

// Destroy the task queue
void destroy_task_queue(task_queue_t *queue)
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
    pthread_cond_destroy(&queue->cond);
    free(queue);
}

// Add a task to the task queue
void add_task_to_queue(task_queue_t *queue, task_t *task)
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
    pthread_cond_broadcast(&queue->cond); // signal all threads that are waiting
    // pthread_cond_signal(&queue->cond); // wake up single thread
    pthread_mutex_unlock(&queue->mutex);
}

void tpool_wait(task_queue_t *queue)
{
    if (queue == NULL)
        return;

    pthread_mutex_lock(&(queue->mutex));
    while (1)
    {
        if (!queue->exit)
            pthread_cond_wait(&(queue->cond), &(queue->mutex));
        else
            break;
    }
    pthread_mutex_unlock(&(queue->mutex));
}

task_t *get_next_task(task_queue_t *queue)
{
    pthread_mutex_lock(&queue->mutex);

    // Wait for a task to be added to the queue
    while (!queue->exit && queue->head == NULL)
        pthread_cond_wait(&queue->cond, &queue->mutex);

    if (queue == NULL)
        return NULL;

    task_t *task = queue->head;
    if (task == NULL)
        return NULL;

    if (task->next == NULL)
    {
        queue->head = NULL;
        queue->tail = NULL;
    }
    else
    {
        queue->head = task->next;
    }
    pthread_mutex_unlock(&queue->mutex);
    return task;
}

#endif // __TASK_Q_H__