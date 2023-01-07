#ifndef __POOL_H__
#define __POOL_H__

#include <pthread.h>
#include <stdlib.h>

#include "kay_task_q.h"

// A structure to represent a thread pool
typedef struct thread_pool
{
    int           num_threads; // Number of threads in the pool
    pthread_t    *threads;     // Array of threads
    task_queue_t *task_queue;  // Queue of tasks
} thread_pool_t;

thread_pool_t *create_thread_pool(int num_threads);         // Create a new thread pool
void           add_task(thread_pool_t *pool, task_t *task); // Add a task to the thread pool
static void   *execute_tasks(void *arg);                    // Execute tasks from the task queue
void           destroy_thread_pool(thread_pool_t *pool);    // Destroy the thread pool

thread_pool_t *create_thread_pool(int num_threads)
{
    // Allocate memory for the thread pool
    thread_pool_t *pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if (pool == NULL)
    {
        perror("malloc");
        return NULL;
    }

    // Initialize the fields of the thread pool
    pool->num_threads = num_threads;
    pool->threads     = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    if (pool->threads == NULL)
    {
        perror("malloc");
        free(pool);
        return NULL;
    }

    pool->task_queue = task_queue_create();
    if (pool->task_queue == NULL)
    {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    // Create the threads in the thread pool
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, execute_tasks, (void *)pool) != 0)
        {
            perror("pthread_create");
            destroy_thread_pool(pool);
            return NULL;
        }
        pthread_detach(pool->threads[i]);
    }

    return pool;
}

// Add a task to the thread pool
void add_task(thread_pool_t *pool, task_t *task)
{
    task_queue_add_task(pool->task_queue, task);
}

// Execute tasks from the task queue
static int   thread_checker = 0;
static void *execute_tasks(void *arg)
{
    task_queue_t *queue = ((thread_pool_t *)arg)->task_queue;
    task_t       *task;

    printf("execute_tasks has started : %d\n", thread_checker++);
    while (1)
    {
        pthread_mutex_lock(&(queue->mutex)); // Lock for getting work

        // Check the task queue for a task
        // Wait for a task to be added to the queue
        while (!queue->exit && queue->head == NULL)
            pthread_cond_wait(&(queue->work_cond), &(queue->mutex));

        if (queue->exit)
        {
            printf("Exit was called\n");
            // pthread_mutex_unlock is called at end of function, to make it safe to break
            break;
        }

        task = get_next_task(queue);
        queue->working_count++;

        pthread_mutex_unlock(&(queue->mutex)); // Unlock after work has been retrieved

        if (task == NULL)
            printf("TASK IS NULL\n");

        if (task != NULL)
        {
            printf("performing task\n");
            // A task was available, so execute it
            (*task->func)(task->arg);

            // Free the memory allocated for the task
            free(task);
        }

        pthread_mutex_lock(&(queue->mutex));
        queue->working_count--;

        if (!queue->exit && queue->head == NULL && queue->working_count == 0)
            pthread_cond_signal(&(queue->working_cond));

        pthread_mutex_unlock(&(queue->mutex));
    }

    pthread_cond_signal(&(queue->working_cond));
    pthread_mutex_unlock(&(queue->mutex));
    return NULL;
}

// Destroy the thread pool
void destroy_thread_pool(thread_pool_t *pool)
{
    if (pool == NULL)
        return;

    tpool_wait(pool->task_queue);

    task_t *current  = NULL;
    task_t *previous = NULL;

    task_queue_t *queue = pool->task_queue;
    pthread_mutex_lock(&(queue->mutex));

    printf("clearing queue\n");

    current = queue->head;
    while (current != NULL)
    {
        previous = current->next;
        free(current);
        current = previous;
    }
    queue->exit = true;

    pthread_cond_broadcast(&(queue->work_cond));
    pthread_mutex_unlock(&(queue->mutex));

    pthread_mutex_destroy(&(queue->mutex));
    pthread_cond_destroy(&(queue->work_cond));
    pthread_cond_destroy(&(queue->working_cond));

    free(pool->task_queue);

    printf("destroy_thread_pool COMPLETE!\n");
}

#endif // __POOL_H__