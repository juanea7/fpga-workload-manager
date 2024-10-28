/**
 * @file data_structures.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains specific data structures of the app
 * @date October 2022
 *
 */

#ifndef _DATA_STRUCTURES_H_
#define _DATA_STRUCTURES_H_

#include <time.h>

// On ram file sizes (empirical)
#define POWER_FILE_SIZE 525*1024
#define TRACES_FILE_SIZE 20*1024
#define ONLINE_FILE_SIZE 2*1024

// Number of different kernel functions
#define TYPES_OF_KERNELS 11


/************** Data structures *******************/

// Kernel labels
typedef enum {
	AES,
	BULK,
	CRS,
	KMP,
	KNN,
	MERGE,
	NW,
	QUEUE,
	STENCIL2D,
	STENCIL3D,
	STRIDED
}kernel_label_t;

/**
 * @brief Kernel related information
 *
 */
typedef struct{
	struct timespec initial_time;					// Applicatoin Epoch
	int temp_id;									// kernel order number inside application
	kernel_label_t kernel_label;					// Kernel label
	int num_executions;								// Number of executions to be performed by the kernel
	long int intended_arrival_time_ms;				// Inter-arrival time
	struct timespec commanded_arrival_time;			// Pretended arrival time
	struct timespec measured_arrival_time;			// Real arrival time
	struct timespec measured_finish_time;			// Real execution finished time
	struct timespec measured_pre_execution_time;	// Real pre-execution time
	struct timespec measured_post_execution_time;	// Real post-execution time
	int cu;											// Number of compute units (kernel replicas) to be used
	int slot_id;									// Indicates in which ARTICo3 slots the CUs are placed. Each bit represent an ARTICo3 slot (LSB == 0).
													// A 1 in a bit means that slot holds a cu of the kernel, 0 means no cu of the kernel in that slot.
													// Designed this way to get reed of dynamic memory management and post-processing complexity
}kernel_data;

/**
 * @brief Monitoring window related information
 *
 */
typedef struct{
	struct timespec initial_time;			// Application Epoch
	struct timespec measured_starting_time;	// Real monitoring window start time
	struct timespec measured_finish_time;	// Real monitoring window end time
}monitor_data;

/**
 * @brief Monitor infrastructure configuration arguments
 *
 */
typedef struct{
	struct timespec initial_time;	           // Application Epoch
	unsigned int period_ms;			           // Monitoring period in miliseconds
	int num_monitorizations;  		   // Number of monitoring windows to be performed. -1 means to run indefinitly
	unsigned int measurements_per_training;    // Number of measurements window in an execution stage (per training epoch)
}monitor_arguments;

/**
 * @brief Online measurements information
 *
 * @note This information is written to a file, not liked to a list
 *
 */
typedef struct{
	kernel_label_t kernel_label;	// Kernel label
	struct timespec arrival_time;	// Monitoring window start time
	struct timespec finish_time;	// Monitoring window end time
}online_data;

#endif /* _DATA_STRUCTURES_H_ */