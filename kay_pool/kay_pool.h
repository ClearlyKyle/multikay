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

    pool->task_queue = create_task_queue();
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
    add_task_to_queue(pool->task_queue, task);
}

// Execute tasks from the task queue
static int   thread_checker = 0;
static void *execute_tasks(void *arg)
{
    thread_pool_t *pool = (thread_pool_t *)arg;
    task_t        *task;

    printf("execute_tasks has started : %d\n", thread_checker++);
    while (1)
    {
        // Check the task queue for a task
        task = get_next_task(pool->task_queue);
        if (task == NULL)
            printf("TASK IS NULL\n");

        if (pool->task_queue->exit)
        {
            printf("Exit was called\n");
            break;
        }

        if (task != NULL)
        {
            printf("performing task\n");
            // A task was available, so execute it
            (*task->func)(task->arg);

            // Free the memory allocated for the task
            free(task);
        }

        pthread_mutex_lock(&(pool->task_queue->mutex));
        if (!pool->task_queue->exit && pool->task_queue->head == NULL)
            pthread_cond_signal(&(pool->task_queue->cond));
        pthread_mutex_unlock(&(pool->task_queue->mutex));
    }
    return NULL;
}

// Destroy the thread pool
void destroy_thread_pool(thread_pool_t *pool)
{
    //// Set the exit flag for all threads
    // pool->task_queue->exit = 1;

    //// Wait for all threads to finish their current tasks
    // pthread_mutex_lock(&pool->task_queue->mutex);
    // pthread_cond_broadcast(&pool->task_queue->cond);
    // pthread_mutex_unlock(&pool->task_queue->mutex);

    //// Wait for the threads to terminate
    // for (int i = 0; i < pool->num_threads; i++)
    //{
    //     printf("Freeing Thread %ld\n", i);
    //     pthread_join(pool->threads[i], NULL);
    // }

    // printf("pthread_join comeplete\n");

    //// Destroy the task queue
    // destroy_task_queue(pool->task_queue);

    //// Terminate the threads
    //// for (int i = 0; i < pool->num_threads; i++)
    ////{
    ////    pthread_cancel(pool->threads[i]);
    ////}

    // free(pool->threads);

    //// Free the memory allocated for the thread pool
    // free(pool);

    if (pool == NULL)
        return;

    task_t *current  = NULL;
    task_t *previous = NULL;

    task_queue_t *queue = pool->task_queue;
    pthread_mutex_lock(&(queue->mutex));

    current = queue->head;
    while (current != NULL)
    {
        previous = current->next;
        free(current);
        current = previous;
    }
    queue->exit = true;

    pthread_cond_broadcast(&(queue->cond));
    pthread_mutex_unlock(&(queue->mutex));

    tpool_wait(queue);

    pthread_mutex_destroy(&(queue->mutex));
    pthread_cond_destroy(&(queue->cond));

    free(queue);
}

#endif // __POOL_H__