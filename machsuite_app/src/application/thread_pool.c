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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>

#include "thread_pool.h"

#include "debug.h"

// Macro that get the thread id via a system call
#define gettid() syscall(SYS_gettid)


/**
 * @brief Work function executed by each thread within the thread pool (internal function)
 * 
 * @param p Pointer to the information about the thread that is going to be generated
 */
static void* _do_work(void *p) {

    // Get thread information
    thread_info *t_info = (thread_info *) p;
    thread_pool *pool = t_info->pool;
    int id = t_info->id;

    // Initialize certain variables
    pool->executed_tasks_per_thread[id] = 0;
    pool->t_ids[id] = gettid();
    pool->running[id] = 0;

    // Infinite loop where each thread will be waiting and executing tasks
	while(1) {
    
        work_t *task;

        print_debug("Hilo #%ld vivo\n", pool->t_ids[id]);
    
        // Get the thread pool mutex
        if(pthread_mutex_lock(&(pool->lock)) < 0) {
            print_error("Thread pool mutex lock error. tid: %ld\n", pool->t_ids[id]);
            exit(1);
        }

        while(pool->wake_up == 0) {

            // Indicate the thread is not running
            pool->running[id] = 0;      

            // Finishing each thread when the thread pool is destroyed
            if(pool->shutdown) {

                // Realease the thread pool mutex
                pthread_mutex_unlock(&(pool->lock));
                // Clean its thread info struct
                t_info->pool = NULL;
                free(t_info);
                // Finish thread execution
                pthread_exit(NULL);

            }
   
            print_debug("Hilo #%ld - Waiting on condition variable cond1\n", pool->t_ids[id]);

            // Wait for a new task to execute
            if(pthread_cond_wait(&(pool->cond), &(pool->lock)) < 0) {
            print_error("Thread pool cv wait error. tid: %ld\n", pool->t_ids[id]);
                exit(1);
            }

            print_debug("Hilo #%ld - Wake-up on condition variable cond1\n", pool->t_ids[id]);  
            // Indicate the thread is running
            pool->running[id] = 1;      

        }

        // Get the task to be executed from the thread pool structure
        task = pool->task;
        pool->task = NULL;
        
        print_debug("Hilo #%ld - Task received\n", pool->t_ids[id]); 

        // Signal an ack
        pool->wake_up = 0;
        if(pthread_cond_signal(&(pool->ack)) < 0) {
            print_error("Thread pool ack cv signal error. tid: %ld\n", pool->t_ids[id]);
            exit(1);
        }

        print_debug("Hilo #%ld - Ack sent\n", pool->t_ids[id]);
        
        // Realease the thread pool mutex
        if(pthread_mutex_unlock(&(pool->lock)) < 0) {
            print_error("Thread pool mutex unlock error. tid: %ld\n", pool->t_ids[id]);
            exit(1);
        }

        // Execute the task
		(task->routine)(task->arg);
        free(task); 
        pool->executed_tasks_per_thread[id]++;
        
	}
}


/**
 * @brief Send a task to be executed by one of the thread pool workers
 * 
 * @param pool Pointer to the thread pool
 * @param dispatch_to_here Function to be executed by a worker
 * @param arg Arguments that have to be passed to the function \p dispatch_to_here
 * @return (int) 0 on success, error code otherwise
 */
int dispatch(thread_pool *pool, dispatch_fn dispatch_to_here, void *arg) {
    
	// Create a task.  
	work_t *task = malloc(sizeof(*task));
	if(task == NULL) {
		print_error("[Dispatch] Out of memory creating a work struct!\n");
		return -1;	
	}
	task->routine = dispatch_to_here;
	task->arg = arg;

    // Get the thread pool mutex
    if(pthread_mutex_lock(&(pool->lock)) < 0) {
        print_error("[Dispatch] Mutex lock\n");
		goto dispatch_error;
    }

    // Signal a task to the thread pool
    pool->task = task;
	pool->wake_up = 1;

    if(pthread_cond_signal(&(pool->cond)) < 0) {
        print_error("[Dispatch] Condition signal\n");
		goto dispatch_error;
    }

    // Wait for an ack
    while(pool->wake_up == 1) {

        if(pthread_cond_wait(&(pool->ack), &(pool->lock)) < 0) {
            print_error("[Dispatch] Wait error\n");
            goto dispatch_error;
        }      

    }

    // Release the thread pool mutex
	if(pthread_mutex_unlock(&(pool->lock)) < 0) {
        print_error("[Dispatch] Mutex unlock\n");
		goto dispatch_error;
    }

    return 0;

    // Error handling
    dispatch_error:
    free(task);

    return -1;
}


/**
 * @brief Indicates whether all workers of thread pool have finised their
 *        assigned tasks or not
 * 
 * @param pool Pointer to the thread pool
 * @return (int) 1 when all the workers have finished their assigned tasks, 0 
 *         otherwise
 */
int isdone(const thread_pool *pool) {

    int i;

    // If any thread is running it returns with 0
    for (i = 0; i < pool->num_threads; i++)
        if (pool->running[i] > 0) return 0;

    // If no thread is running it returns with 1
    return 1;
}


/**
 * @brief Print thread pool information (internal function)
 * 
 * @param pool Pointer to the thread pool
 */
static void _print_thread_pool_info(const thread_pool *pool) {

    int i, total_tasks = 0;

    // Print information for each thread in the thread pool
    for (i = 0; i < pool->num_threads; i++) {

        int curr_thread_tasks;

        curr_thread_tasks = pool->executed_tasks_per_thread[i];
        total_tasks += curr_thread_tasks;

        printf("\nid: %d, tid: %ld, executed_task: %d\n", i, pool->t_ids[i], curr_thread_tasks);

    }

    printf("Total tasks executed (distributed in %d threads): %d\n\n", i, total_tasks);   
}


/**
 * @brief Create and initialize the thread pool
 * 
 * @param num_threads_in_pool Number of workers to be generated
 * @return (thread_pool*) Pointer to the generated thread pool
 */
thread_pool* create_thread_pool(const int num_threads_in_pool) {

    thread_pool *pool = NULL;
    int i;

    // Sanity check
    if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
        return NULL;

    // Allocate the thread pool structure
    pool = malloc(sizeof(*pool));
    if (pool == NULL) {
        print_error("[Thread pool creation] Error: Out of memory creating the thread pool with malloc.\n");
        return NULL;
    }

    // Allocate the array of threads
    pool->threads = malloc(sizeof(*(pool->threads)) * num_threads_in_pool);
    if (pool->threads == NULL) {
        print_error("[Thread pool creation] Error: Out of memory creating threads for the thread pool with malloc.\n");
		goto thread_malloc_err;
    }

    // Allocate the array of ints used as executed task counter
    pool->executed_tasks_per_thread = malloc(sizeof(*(pool->executed_tasks_per_thread)) * num_threads_in_pool);
    if (pool->executed_tasks_per_thread == NULL) {
        print_error("[Thread pool creation] Error: Out of memory creating threads for the thread pool with malloc.\n");
		goto executed_tasks_per_thread_malloc_err;
    }

    // Allocate the array of thread ids
    pool->t_ids = malloc(sizeof(*(pool->t_ids)) * num_threads_in_pool);
    if (pool->t_ids == NULL) {
        print_error("[Thread pool creation] Error: Out of memory creating t_ids for the thread pool with malloc.\n");
		goto t_ids_malloc_err;
    }

    pool->running = malloc(sizeof(*(pool->running)) * num_threads_in_pool);
    if (pool->running == NULL) {
        print_error("Error: Out of memory creating running for the thread pool with malloc.\n");
		goto running_malloc_err;
    }

    // Initialize some thread pool variables
    pool->num_threads = num_threads_in_pool;
    pool->wake_up = 0;
    pool->shutdown = 0;
    
    // Initialize mutex and condition variables.  
    if(pthread_mutex_init(&pool->lock,NULL)) {
        print_error("[Thread pool creation] Mutex initiation error!\n");
        return NULL;
    }
    if(pthread_cond_init(&(pool->cond),NULL)) {
        print_error("[Thread pool creation] CV cond initiation error!\n");	
        return NULL;
    }
    if(pthread_cond_init(&(pool->ack),NULL)) {
        print_error("[Thread pool creation] CV ack initiation error!\n");	
        return NULL;
    }

    // Create the threads of the thread pool
    for (i = 0; i < num_threads_in_pool; i++) {

        // Create and initiailize each thread info structure
        thread_info *t_info = malloc(sizeof(*t_info));
        t_info->pool = pool;
        t_info->id = i;

        if(pthread_create(&(pool->threads[i]), NULL, _do_work, t_info)) {
            print_error("[Thread pool creation] Thread initiation error. id: %d\n", i);	
		    goto thread_create_err;	
        }
    }

    return (thread_pool*) pool;

    // Error handling
    thread_create_err:
    free(pool->running);

    running_malloc_err:
    free(pool->t_ids);

    t_ids_malloc_err:
    free(pool->executed_tasks_per_thread);

    executed_tasks_per_thread_malloc_err:
    free(pool->threads);

	thread_malloc_err:
    free(pool);

    return NULL;
}


/**
 * @brief Destroy the thread pool
 * 
 * @param pool Pointer to the thread pool
 */
void destroy_threadpool(thread_pool *pool) {
	
    int i; 
        
    // Get thread pool mutex
    if(pthread_mutex_lock(&(pool->lock)) < 0){
        print_error("[Thread pool destruction] Mutex lock\n");
        exit(1);
    }

    // Signal a shutdown message to all threads
	pool->shutdown = 1;

    if(pthread_cond_broadcast(&(pool->cond)) < 0){
        print_error("[Thread pool destruction] Condition broadcast\n");
        exit(1);
    }

    // Release the thread pool mutex
    if(pthread_mutex_unlock(&(pool->lock)) < 0){
        print_error("[Thread pool destruction] Mutex unlock\n");
        exit(1);
    }

	// Wait for each of the thread of the pool to finish
	for(i = 0; i < pool->num_threads; i++) {
        if(pthread_join(pool->threads[i], NULL) < 0){
            print_error("[Thread pool destruction] Join error\n");
            exit(1);
        }
	}

    // Print thread pool info
    #if INFO
    print_info("Print thread pool information\n");
    _print_thread_pool_info(pool);
    #endif

    // Deallocate dynamically allocated thread pool variables
	free(pool->threads);
	free(pool->executed_tasks_per_thread);
	free(pool->t_ids);
	free(pool->running);

    // Destroy mutex and conditional vairables
    if(pthread_mutex_destroy(&(pool->lock)) < 0){
        print_error("[Thread pool destruction] Mutex destroy\n");
        exit(1);
    }
    if(pthread_cond_destroy(&(pool->cond)) < 0){
        print_error("[Thread pool destruction] cond CV destroy\n");
        exit(1);
    }
    if(pthread_cond_destroy(&(pool->ack)) < 0){
        print_error("[Thread pool destruction] ack CV destroy\n");
        exit(1);
    }

    // Clean the thread pool structure
    free(pool);
}
