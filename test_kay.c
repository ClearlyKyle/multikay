#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "./kay_pool/kay_pool.h"

#ifdef _WIN32
// Code to include the necessary headers for Windows goes here
#include <windows.h>
#define SLEEP(ms) Sleep(ms)
#else
// Code to include the necessary headers for Linux goes here
#include <unistd.h>
#define SLEEP(ms) sleep(ms / 1000)
#endif

// A global variable to hold the result of the test function
static int result = 0;

// A function that sets the global result to a specified value
void set_result(void *value)
{
    // printf("set_result : tid=%p\n", pthread_self());
    result = *((int *)value);
}

// A function that sleeps for a specified number of seconds
void sleep_func(void *arg)
{
    // printf("sleep_func : tid=%p\n", pthread_self());
    int seconds = *((int *)arg);
    SLEEP(seconds);
}

// A test function that checks the return value of create_thread_pool
void test_create_thread_pool()
{
    thread_pool_t *pool = create_thread_pool(4);
    if (pool == NULL)
    {
        printf("create_thread_pool returned NULL\n");
        exit(1);
    }
    if (pool->num_threads != 4)
    {
        printf("create_thread_pool returned a pool with the wrong number of threads\n");
        exit(1);
    }
    if (pool->threads == NULL)
    {
        printf("create_thread_pool returned a pool with a NULL threads array\n");
        exit(1);
    }
    if (pool->task_queue == NULL)
    {
        printf("create_thread_pool returned a pool with a NULL task queue\n");
        exit(1);
    }
    SLEEP(3000);

    destroy_thread_pool(pool);
}

// A test function that checks the return value of create_task
void test_create_task()
{
    task_t *task = create_task(set_result, &result);
    if (task == NULL)
    {
        printf("create_task returned NULL\n");
        exit(1);
    }
    if (task->func != set_result)
    {
        printf("create_task returned a task with the wrong function\n");
        exit(1);
    }
    if (task->arg != &result)
    {
        printf("create_task returned a task with the wrong argument\n");
        exit(1);
    }
    free(task);
}

// A test function that checks the execution of tasks
void test_execute_tasks()
{
    thread_pool_t *pool = create_thread_pool(1);
    int            arg  = 3;
    task_t        *task = create_task(sleep_func, &arg);

    add_task(pool, task);

    SLEEP(1000);
    if (result != 0)
    {
        printf("Task did not execute correctly\n");
        exit(1);
    }

    destroy_thread_pool(pool);
}

int main()
{
    printf("Starting tests...\n");

    printf("[---START] test_create_thread_pool\n");
    test_create_thread_pool();
    printf("[COMPLETE] test_create_thread_pool\n");

    printf("[---START] test_create_task\n");
    test_create_task();
    printf("[COMPLETE] test_create_task\n");

    printf("[---START] test_execute_tasks\n");
    test_execute_tasks();
    printf("[COMPLETE] test_execute_tasks\n");

    printf("All tests passed!\n");
    return 0;
}
