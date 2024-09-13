/**
 * @file thread_pool.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains functions to implement a thread pool software 
 *        design pattern to efficiently handle a multitude of asynchronous
 *        concurrent tasks in a scalable and stable manner.
 * @warning This design pattern has been implemented to overcome and issue
 *          where at some point the execution of the application in linux
 *          hungs after a large amount of different threads have generated.
 *          Using this approach there are only a few threads within the
 *          application life-time, drastically mitigating the issue.
 *          Seems like linux cannot handle that many threads by with no aparent
 *          reason...
 * @date February 2023
 * 
 */

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <pthread.h>

// Maximum number of workers allowed in the thread pool
#define MAXT_IN_POOL 20

/**
 * @brief Task data
 * 
 */
typedef struct work{
	void* (*routine) (void*);   // Pointer to the function that represents the task
	void *arg;                  // Pointer to the arguments to be passed to the function
} work_t;

/**
 * @brief Thread pool data.
 *        
 * @note When (array) is indicated, it means that field is an array containing
 *       an instance of the specified variable for each worker
 * 
 */
typedef struct threadpool {
    int num_threads;                // Number of workers within in the pool
    pthread_t *threads;             // pthread_t id of each worker (array)
    long int *t_ids;                // UNIX thread id of each worker (array)
    int *executed_tasks_per_thread; // Number of executed tasks per thread (array)
    int *running;                   // Flag indicating if each thread is busy (array)
	pthread_mutex_t lock;	        // Mutex used to synchronize the threads
	pthread_cond_t cond;            // CV used by the user to signal the workers that a task is waiting
    pthread_cond_t ack;             // CV used by a worker to indicate the user it is handling the task
    int wake_up;                    // Flag associated with the field "cond" used to command a worker to wake up
    int shutdown;                   // Flag associated with the field "cond" used to command a worker to shutdown
	work_t* task;                   // Pointer to a task that needs to be executed
}thread_pool;

/**
 * @brief Information passed to each worker at initialization
 * 
 */
typedef struct thread_info {
    thread_pool *pool;  // Pointer to the thread pool
    int id;             // Id of the particular worker
}thread_info;

/**
 * @brief Type of which must be each function to be executed by a worker
 * 
 * @note The functions must have the following signature:
 *       void dispatch_function(void *arg)
 * 
 */
typedef void* (*dispatch_fn)(void *);

/**
 * @brief Send a task to be executed by one of the thread pool workers
 * 
 * @param pool Pointer to the thread pool
 * @param dispatch_to_here Function to be executed by a worker
 * @param arg Arguments that have to be passed to the function \p dispatch_to_here
 * @return (int) 0 on success, error code otherwise
 */
int dispatch(thread_pool *pool, dispatch_fn dispatch_to_here, void *arg);

/**
 * @brief Indicates whether all workers of thread pool have finised their
 *        assigned tasks or not
 * 
 * @param pool Pointer to the thread pool
 * @return (int) 1 when all the workers have finished their assigned tasks, 0 
 *         otherwise
 */
int isdone(const thread_pool *pool);

/**
 * @brief Create and initialize the thread pool
 * 
 * @param num_threads_in_pool Number of workers to be generated
 * @return (thread_pool*) Pointer to the generated thread pool
 */
thread_pool* create_thread_pool(const int num_threads_in_pool);

/**
 * @brief Destroy the thread pool
 * 
 * @param pool Pointer to the thread pool
 */
void destroy_threadpool(thread_pool *pool);


#endif /*_THREAD_POOL_H_*/