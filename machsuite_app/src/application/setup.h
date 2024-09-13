/**
 * @file setup.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief
 * @date September 2022
 * 
 */

#ifndef _SETUP_H_
#define _SETUP_H_

#include <time.h>
#include <pthread.h>

#include "data_structures.h"
#include "queue_kernel.h"
#include "queue_online.h"


// Typedef for kernel execution function pointer
typedef void (*execution_funct_type)(kernel_data*,queue_online*,pthread_mutex_t*,void*);

/**
 * @brief Function executed by the queue manager thread.
 * 			- Decides which kernel is executed and dispatches the execution to a thread pool worker.
 * 
 * @param arg Not used.
 */
void* queue_manager_thread(void *arg);

/**
 * @brief Funtion executed by each execution thread (performed by the thread pool workers).
 * 		  	- Configures, load, execute, and verify each kernel execution.
 * 
 * @param arg Pointer to the kernel data the execution to be performed
 */
void* execution_thread(void *arg);

/**
 * @brief Generates a file containing the information about which kernel is executed in each slots thoughout a whole monitoring window
 * 
 * // pasar el uso de la cpu a struct TODO
 * @param monitor_window Pointer to the monitoring window to be processed
 * @param monitorization_count Id of this monitoring window. Used for file naming purposes (TODO:to be removed)
 * @param online_ptr Buffer to store online data in
 * @param cloud_socket_id Socket id used to send data to the cloud
 * @return (int) 0 on success, error code otherwise
 */
int online_processing(float user_cpu, float kernel_cpu, float idle_cpu, const monitor_data *monitor_window, const int monitorization_count, char *online_ptr, int cloud_socket_id);

/**
 * @brief Function executed by the monitoring thread,
 * 		  	- Obtains power and performance traces periodically with the monitoring infrastructure.
 *        	- Generates online data files that could be used for online training
 * 
 * @param arg Pointer to the arguments passed to this function
 */
void* monitoring_thread(void *arg);

/**
 * @brief Function executed by the cpu usage monitor thread.
 * 		  	- Periodically calculates the usage of the CPU by means of the
 * 			  user, kernel and idle time within a time window
 * 
 * @param arg Pointer to the arguments passed to this function
 */
void* CPU_usage_monitor_thread(void *arg);


#endif /* _SETUP_H_ */