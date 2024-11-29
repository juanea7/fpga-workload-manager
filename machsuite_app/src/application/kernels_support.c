/**
 * @file kernels_support.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions used to perform the configuration
 * 		  and execution of each kernel in the FPGA using ARTICo3
 * @date September 2022
 *
 */

// TODO: error codes

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include "kernels_support.h"

#include "debug.h"
#include "data_structures.h"


// Includes needed when using ARTICo3
#if ARTICO

	#include "artico3.h"

	#include "kernels/aes_support.h"
	#include "kernels/aes.h"

	#include "kernels/bulk_support.h"
	#include "kernels/bulk.h"

	#include "kernels/crs_support.h"
	#include "kernels/crs.h"

	#include "kernels/kmp_support.h"
	#include "kernels/kmp.h"

	#include "kernels/knn_support.h"
	#include "kernels/knn.h"

	#include "kernels/merge_support.h"
	#include "kernels/merge.h"

	#include "kernels/nw_support.h"
	#include "kernels/nw.h"

	#include "kernels/queue_support.h"
	#include "kernels/queue.h"

	#include "kernels/stencil2d_support.h"
	#include "kernels/stencil2d.h"

	#include "kernels/stencil3d_support.h"
	#include "kernels/stencil3d.h"

	#include "kernels/strided_support.h"
	#include "kernels/strided.h"

#endif


// Typedef for kernel data management
typedef void (*kernel_input_to_data_t)(int,void*);
typedef void (*kernel_output_to_data_t)(int,void*);
typedef int (*kernel_check_data_t)(void*,void*);

// Input to data funtion array
#if ARTICO
static kernel_input_to_data_t _kernel_input_to_data[TYPES_OF_KERNELS] = {
	aes_input_to_data,
	bulk_input_to_data,
	crs_input_to_data,
	kmp_input_to_data,
	knn_input_to_data,
	merge_input_to_data,
	nw_input_to_data,
	queue_input_to_data,
	stencil2d_input_to_data,
	stencil3d_input_to_data,
	strided_input_to_data
};

// Output to data function array
static kernel_output_to_data_t _kernel_output_to_data[TYPES_OF_KERNELS] = {
	aes_output_to_data,
	bulk_output_to_data,
	crs_output_to_data,
	kmp_output_to_data,
	knn_output_to_data,
	merge_output_to_data,
	nw_output_to_data,
	queue_output_to_data,
	stencil2d_output_to_data,
	stencil3d_output_to_data,
	strided_output_to_data
};

// Kernel data checking function array
static kernel_check_data_t _kernel_check_data[TYPES_OF_KERNELS] = {
	aes_check_data,
	bulk_check_data,
	crs_check_data,
	kmp_check_data,
	knn_check_data,
	merge_check_data,
	nw_check_data,
	queue_check_data,
	stencil2d_check_data,
	stencil3d_check_data,
	strided_check_data
};
#endif


// Kernel names
static char _kernel_names [TYPES_OF_KERNELS][15] = {
	"aes",
	"bulk",
	"crs",
	"kmp",
	"knn",
	"merge",
	"nw",
	"queue",
	"stencil2d",
	"stencil3d",
	"strided"
};

// Kernel input size variables
static int _kernel_input_size[TYPES_OF_KERNELS];
// Kernel input and reference data storage array
static char* _kernel_input_data[TYPES_OF_KERNELS];
static char* _kernel_reference_data[TYPES_OF_KERNELS];


/**
 * @brief Enqueue online processing info of a particular kernel in the online
 * 		  queues specific to the slots in which is executed (internal function)
 *
 * @param online_queue Array of all the online queues (one per ARTICo3 slot)
 * @param online_queue_lock Pointer to the mutex used to synchronize the online queue of the slot where the kernel executes
 * @param kernel Pointer to the kernel_data of the kernel executed
 * @return (int) 0 on success, error code otherwise
 */
static int _kernel_to_online_queue(queue_online *online_queue, pthread_mutex_t *online_queue_lock, kernel_data *kernel) {

	int i,j,ret;

	print_debug("Execution - Pre-add online info to queue\n");

	// Find in which ARTICo3 slot each cu is executed
	// 		- The slot_id field in the kernel structure has as many bits as ARTICo3 slots (LSB==0).
	//	      It indicates with 1s and 0s if a kernel replica is placed in each slot.
	//		- We check if a replica is placed in each slot, bit by bit, until we find the "cu" number of replicas.
	for(i = 0, j = 0; i < kernel->cu; j++) {

		// When a kernel replica is found in a slot, the kernel data is enqueue in the online queue of that particular slot
		if((kernel->slot_id & (1 << j)) > 0) {

			// Lock the online queue of each ARTICo3 slot where the kernel has a replica executing to place kernel info
			if(pthread_mutex_lock(&(online_queue_lock[j])) < 0) {
				print_error("online_queue_lock [%d] - Execution\n", j);
				exit(1);
			}

			// Enqueue kernel info in the online queue
			ret = enqueue_online(&(online_queue[j]), kernel);
			if(ret != 0) {
				print_error("Error adding kernel pointer to online queue #%d\n",j);
				exit(1);
			}

			// Unlock the online queue
			if(pthread_mutex_unlock(&(online_queue_lock[j])) < 0) {
				print_error("online_queue_unlock [%d] - Execution\n", j);
				exit(1);
			}

			// Increment the kernel replica counter
			i++;
		}
	}

	print_debug("Execution - Post-add online info queue\n");

	return 0;
}

/**
 * @brief Print whether the kernel has been successfully executed or not (internal function)
 *
 * @param success Flag indicating if the kernel has succeded (1 when success, 0 when fail)
 * @param kernel_name Kernal label to be printed
 */
static void _print_kernel_success(const int success, char *kernel_name) {

	if(success)
		print_info("\n[\033[1;32m OK \033[0m] %s\n\n", kernel_name);
	else
		print_error("\n[\033[1;31mFAIL\033[0m] %s\n\n", kernel_name);

}


/**
 * @brief Kernel input size calculation loading
 *
 * @param input_size_array Array containing input sizes of the kernels
 */
static void _kernel_calculate_input_size(int *input_size_array) {

	#if ARTICO
	input_size_array[0] = AES_INPUT_SIZE;
	input_size_array[1] = BULK_INPUT_SIZE;
	input_size_array[2] = CRS_INPUT_SIZE;
	input_size_array[3] = KMP_INPUT_SIZE;
	input_size_array[4] = KNN_INPUT_SIZE;
	input_size_array[5] = MERGE_INPUT_SIZE;
	input_size_array[6] = NW_INPUT_SIZE;
	input_size_array[7] = QUEUE_INPUT_SIZE;
	input_size_array[8] = STENCIL2D_INPUT_SIZE;
	input_size_array[9] = STENCIL3D_INPUT_SIZE;
	input_size_array[10] = STRIDED_INPUT_SIZE;
	#endif
}

/**
 * @brief Kernel input data loading
 *
 * @param input_data Pointer to the input data
 * @param kernel Kernel label enum
 */
static void _kernel_load_input(char **input_data, kernel_label_t kernel) {

	print_debug("Loading Kernel #%s Data...\n", _kernel_names[kernel]);

	#if ARTICO

    char in_file[40];
	sprintf(in_file, "data/%s/input.data", _kernel_names[kernel]);

	*input_data = (char*)malloc(_kernel_input_size[kernel]);
	if(*input_data == NULL) {
		print_error("Error en mmap data\n");
		exit(1);
	}

	int in_fd;
	in_fd = open(in_file, O_RDONLY);
	if(in_fd <= 0) {
		print_error("Error abrir input: %s\n",in_file);
		exit(1);
	}

	_kernel_input_to_data[kernel](in_fd, *input_data);

	#endif
}

/**
 * @brief Kernel reference data loading
 *
 * @param reference_data Pointer to the reference data
 * @param kernel Kernel label enum
 */
static void _kernel_load_reference(char **reference_data, kernel_label_t kernel) {

	print_debug("Loading Kernel #%s Check Data...\n", _kernel_names[kernel]);

	#if ARTICO

    char check_file[40];
	sprintf(check_file, "data/%s/check.data", _kernel_names[kernel]);

	*reference_data = (char*)malloc(_kernel_input_size[kernel]);
	if(*reference_data == NULL) {
		print_error("Error en mmap data\n");
		exit(1);
	}

	int check_fd;
	check_fd = open(check_file, O_RDONLY);
	if(check_fd <= 0) {
		print_error("Error abrir check: %s\n",check_file);
		exit(1);
	}

	_kernel_output_to_data[kernel](check_fd, *reference_data);

	#endif
}

/**
 * @brief Kernel file data clean-up
 *
 * @param input_data Pointer to the input data
 * @param reference_data Pointer to the reference data
 */
static void _kernel_file_data_clean(char **input_data, char **reference_data) {

	free(*input_data);
	free(*reference_data);
	*input_data = NULL;
	*reference_data = NULL;
}

/**
 * @brief Kernel input and reference data initialization
 *
 */
void kernel_init_data() {

	int kernel;

	// Load input size of kernel data
	_kernel_calculate_input_size(_kernel_input_size);

	// Load input and reference data for each kernel
	for(kernel = 0; kernel < TYPES_OF_KERNELS; kernel++) {
		_kernel_load_input(&_kernel_input_data[kernel], kernel);
		_kernel_load_reference(&_kernel_reference_data[kernel], kernel);
	}

	print_info("Loaded input and reference data\n");
}

/**
 * @brief Kernel input and reference data clean-up
 *
 */
void kernel_clean_data() {

	int kernel;

	// Clean input and reference data for each kernel
	for(kernel = 0; kernel < TYPES_OF_KERNELS; kernel++) {
		_kernel_file_data_clean(&_kernel_input_data[kernel], &_kernel_reference_data[kernel]);
	}
}

/**
 * @brief Kernel input data copying
 *
 * @param io_data Pointer to store the copied input data in
 * @param kernel Kernel label enum
 */
void kernel_copy_input(char **io_data, kernel_label_t kernel) {

	print_debug("Copying #%s Data...\n", _kernel_names[kernel]);

	#if ARTICO

	// Allocate memory for the input data
	*io_data = (char*)malloc(_kernel_input_size[kernel]);
	if(*io_data == NULL) {
		print_error("Error en mmap data\n");
		exit(1);
	}

	// Copy the input data
	memcpy(*io_data, _kernel_input_data[kernel], _kernel_input_size[kernel]);

	#endif
}

/**
 * @brief Kernel result validation
 *
 * @param output_data Pointer to the output data
 * @param kernel Kernel label enum
 */
void kernel_result_validation(char **output_data, kernel_label_t kernel) {

	print_debug("Validation #%s Output...\n", _kernel_names[kernel]);

	#if ARTICO

	// Validate HW benchmark results
	int error = 0;
	if(!_kernel_check_data[kernel](*output_data, _kernel_reference_data[kernel]))
	    error = 1;

	// Free the output data pointer
	free(*output_data);
	*output_data = NULL;

	// Print info about success or fail
	_print_kernel_success(error == 0, _kernel_names[kernel]);

	#endif
}

/**
 * @brief AES kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param aes_vargs Pointer to the kernel inputs and outputs buffers
 */
void aes_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *aes_vargs) {

	print_debug("Execution AES...\n");

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" aes accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("aes", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *aes_key = artico3_alloc(sizeof(*aes_key) * executions * 32, "aes", "aes_key", A3_P_I);
	a3data_t *aes_enckey = artico3_alloc(sizeof(*aes_enckey) * executions * 32, "aes", "aes_enckey", A3_P_I);
	a3data_t *aes_deckey = artico3_alloc(sizeof(*aes_deckey), "aes", "aes_deckey", A3_P_I);
	a3data_t *aes_k = artico3_alloc(sizeof(*aes_k) * executions * 32, "aes", "aes_k", A3_P_I);
	a3data_t *aes_buf = artico3_alloc(sizeof(*aes_buf) * executions * 16, "aes", "aes_buf", A3_P_IO);

	if(aes_key == NULL || aes_enckey == NULL || aes_deckey == NULL || aes_k == NULL || aes_buf == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct aes_bench_args_t *aes_args = (struct aes_bench_args_t *)aes_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < 32; j++) {
			aes_k[i*32 + j] = aes_args->k[j];
		}
		for(j = 0; j < 16; j++) {
			aes_buf[i*16 + j] = aes_args->buf[j];
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("aes", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("aes");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < 16; j++) {
		aes_args->buf[j] = aes_buf[j];
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("aes", "aes_key");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("aes", "aes_enckey");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("aes", "aes_deckey");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("aes", "aes_k");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("aes", "aes_buf");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief BULK kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param bulk_vargs Pointer to the kernel inputs and outputs buffers
 */
void bulk_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *bulk_vargs) {

	print_debug("Execution BULK...\n");
	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" bulk accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("bulk", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *bulk_edges = artico3_alloc(sizeof(*bulk_edges) * executions * BULK_N_EDGES, "bulk", "bulk_edges", A3_P_I);
	a3data_t *bulk_pack = artico3_alloc(sizeof(*bulk_pack) * executions * (3 * BULK_N_NODES + BULK_N_LEVELS + 1), "bulk", "bulk_pack", A3_P_IO);

	if(bulk_edges == NULL || bulk_pack == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// TODO: Cambiar por memset

	// Copy Inputs
	struct bulk_bench_args_t *bulk_args = (struct bulk_bench_args_t *)bulk_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < BULK_N_EDGES; j++) {
			bulk_edges[i*BULK_N_EDGES + j] = bulk_args->edges[j].dst;
		}
		for(j = 0; j < BULK_N_NODES; j++) {
			bulk_pack[i*(3*BULK_N_NODES + BULK_N_LEVELS + 1) + (0*BULK_N_NODES) + j] = bulk_args->nodes[j].edge_begin;
			bulk_pack[i*(3*BULK_N_NODES + BULK_N_LEVELS + 1) + (1*BULK_N_NODES) + j] = bulk_args->nodes[j].edge_end;
			bulk_pack[i*(3*BULK_N_NODES + BULK_N_LEVELS + 1) + (2*BULK_N_NODES) + 1 + j] = bulk_args->level[j];
		}
		bulk_pack[i*(3*BULK_N_NODES + BULK_N_LEVELS + 1) + (2*BULK_N_NODES)] = bulk_args->starting_node;

		// Initialize output (sometimes the output value is kept as when the execution started, expecting it to be zero, but the artico3_alloc()
		// sometimes put trash on it, so it's important to initialize the values to 0 (the struct bulk_bench_args_t is memset to 0 in input_to_data()))
		for(j = 0; j < BULK_N_LEVELS; j++) {
			bulk_pack[i*(3*BULK_N_NODES + BULK_N_LEVELS + 1) + (2*BULK_N_NODES) + 1 + j] = bulk_args->level[j];
			bulk_pack[i*(3*BULK_N_NODES + BULK_N_LEVELS + 1) + (3*BULK_N_NODES) + 1 + j] = bulk_args->level_counts[j];
		}

	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("bulk", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("bulk");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < BULK_N_LEVELS; j++) {
		bulk_args->level[j] = bulk_pack[(2*BULK_N_NODES) + 1 + j];
		bulk_args->level_counts[j] = bulk_pack[(3*BULK_N_NODES) + 1 + j];
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("bulk", "bulk_edges");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("bulk", "bulk_pack");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief CRS kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param crs_vargs Pointer to the kernel inputs and outputs buffers
 */
void crs_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *crs_vargs) {

	print_debug("Execution CRS... executions = %d\n", kernel->num_executions);

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" crs accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("crs", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *crs_val = artico3_alloc(sizeof(*crs_val) * executions * CRS_NNZ, "crs", "crs_val", A3_P_I);
	a3data_t *crs_cols = artico3_alloc(sizeof(*crs_cols) * executions * CRS_NNZ, "crs", "crs_cols", A3_P_I);
	a3data_t *crs_rowDelimiters = artico3_alloc(sizeof(*crs_rowDelimiters) * executions * (CRS_N + 1), "crs", "crs_rowDelimiters", A3_P_I);
	a3data_t *crs_vec = artico3_alloc(sizeof(*crs_vec) * executions * CRS_N, "crs", "crs_vec", A3_P_I);
	a3data_t *crs_out = artico3_alloc(sizeof(*crs_out) * executions * CRS_N, "crs", "crs_out", A3_P_O);

	if(crs_val == NULL || crs_cols == NULL || crs_rowDelimiters == NULL || crs_vec == NULL || crs_out == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct crs_bench_args_t *crs_args = (struct crs_bench_args_t *)crs_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < CRS_NNZ; j++) {
			crs_val[i*CRS_NNZ + j] = ftoa3(crs_args->val[j]);
			crs_cols[i*CRS_NNZ + j] = crs_args->cols[j];
		}
		for(j = 0; j < CRS_N; j++) {
			crs_vec[i*CRS_N + j] = ftoa3(crs_args->vec[j]);
		}
		for(j = 0; j < (CRS_N + 1); j++) {
			crs_rowDelimiters[i*(CRS_N + 1) + j] = crs_args->rowDelimiters[j];
		}

		//test
		for(j = 0; j < CRS_N; j++) {
			crs_out[i*CRS_N + j] = 0;
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("crs", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("crs");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < CRS_N; j++) {
		crs_args->out[j] = a3tof(crs_out[j]);
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("crs", "crs_val");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("crs", "crs_cols");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("crs", "crs_rowDelimiters");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("crs", "crs_vec");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("crs", "crs_out");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief KMP kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param kmp_vargs Pointer to the kernel inputs and outputs buffers
 */
void kmp_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *kmp_vargs) {

	print_debug("Execution KMP... executions = %d\n", kernel->num_executions);

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" kmp accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("kmp", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *kmp_input = artico3_alloc(sizeof(*kmp_input) * executions * KMP_STRING_SIZE, "kmp", "kmp_input", A3_P_I);
	a3data_t *kmp_pack = artico3_alloc(sizeof(*kmp_pack) * executions * (2 * KMP_PATTERN_SIZE + 1), "kmp", "kmp_pack", A3_P_IO);

	if(kmp_input == NULL || kmp_pack == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct kmp_bench_args_t *kmp_args = (struct kmp_bench_args_t *)kmp_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < KMP_STRING_SIZE; j++) {
			kmp_input[i*KMP_STRING_SIZE + j] = kmp_args->input[j];
		}
		for(j = 0; j < KMP_PATTERN_SIZE; j++) {
			kmp_pack[i*(2*KMP_PATTERN_SIZE+1) + (0*KMP_PATTERN_SIZE) + j] = kmp_args->pattern[j];
			kmp_pack[i*(2*KMP_PATTERN_SIZE+1) + (1*KMP_PATTERN_SIZE) + j] = kmp_args->kmpNext[j];
		}
		//test
		kmp_pack[i*(2*KMP_PATTERN_SIZE+1) + 2*KMP_PATTERN_SIZE] = kmp_args->n_matches[0];
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("kmp", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("kmp");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	kmp_args->n_matches[0] = kmp_pack[2*KMP_PATTERN_SIZE];

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("kmp", "kmp_input");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("kmp", "kmp_pack");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief KNN kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param knn_vargs Pointer to the kernel inputs and outputs buffers
 */
void knn_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *knn_vargs) {

	print_debug("Execution KNN... -> slot#%d\n",kernel->slot_id);

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" knn accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("knn", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *KNN_NL = artico3_alloc(sizeof(*KNN_NL) * executions * knn_nAtoms * knn_maxNeighbors, "knn", "KNN_NL", A3_P_I);
	a3data_t *knn_pack = artico3_alloc(sizeof(*knn_pack) * executions * 6 * knn_nAtoms, "knn", "knn_pack", A3_P_IO);

	if(KNN_NL == NULL || knn_pack == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy inputs
  	struct knn_bench_args_t *knn_args = (struct knn_bench_args_t *)knn_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < (knn_nAtoms * knn_maxNeighbors); j++) {
			KNN_NL[i*knn_nAtoms*knn_maxNeighbors + j] = knn_args->NL[j];
		}
		for(j = 0; j < knn_nAtoms; j++) {
			knn_pack[i*(6*knn_nAtoms) + (0*knn_nAtoms) + j] = ftoa3(knn_args->force_x[j]);
			knn_pack[i*(6*knn_nAtoms) + (1*knn_nAtoms) + j] = ftoa3(knn_args->force_y[j]);
			knn_pack[i*(6*knn_nAtoms) + (2*knn_nAtoms) + j] = ftoa3(knn_args->force_z[j]);
			knn_pack[i*(6*knn_nAtoms) + (3*knn_nAtoms) + j] = ftoa3(knn_args->position_x[j]);
			knn_pack[i*(6*knn_nAtoms) + (4*knn_nAtoms) + j] = ftoa3(knn_args->position_y[j]);
			knn_pack[i*(6*knn_nAtoms) + (5*knn_nAtoms) + j] = ftoa3(knn_args->position_z[j]);
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("knn", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("knn");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

  	// Copy Outputs
	for(j = 0; j < knn_nAtoms; j++) {
		knn_args->force_x[j] = a3tof(knn_pack[0*knn_nAtoms + j]);
		knn_args->force_y[j] = a3tof(knn_pack[1*knn_nAtoms + j]);
		knn_args->force_z[j] = a3tof(knn_pack[2*knn_nAtoms + j]);
		knn_args->position_x[j] = a3tof(knn_pack[3*knn_nAtoms + j]);
		knn_args->position_y[j] = a3tof(knn_pack[4*knn_nAtoms + j]);
		knn_args->position_z[j] = a3tof(knn_pack[5*knn_nAtoms + j]);
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("knn", "KNN_NL");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("knn", "knn_pack");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief MERGE kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param merge_vargs Pointer to the kernel inputs and outputs buffers
 */
void merge_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *merge_vargs) {

	print_debug("Execution MERGE...\n");

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" merge accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("merge", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *merge_a = artico3_alloc(sizeof(*merge_a) * executions * MERGE_SIZE, "merge", "merge_a", A3_P_IO);

	if(merge_a == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct merge_bench_args_t *merge_args = (struct merge_bench_args_t *)merge_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < MERGE_SIZE; j++) {
			merge_a[i*MERGE_SIZE + j] = merge_args->a[j];
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("merge", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("merge");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < MERGE_SIZE; j++) {
		merge_args->a[j] = merge_a[j];
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("merge", "merge_a");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief NW kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param nw_vargs Pointer to the kernel inputs and outputs buffers
 */
void nw_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *nw_vargs) {

	print_debug("Execution NW...\n");

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" nw accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("nw", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *NW_M = artico3_alloc(sizeof(*NW_M) * executions * (NW_ALEN + 1) * (NW_BLEN + 1), "nw", "NW_M", A3_P_I);
	a3data_t *nw_ptr = artico3_alloc(sizeof(*nw_ptr) * executions * (NW_ALEN + 1) * (NW_BLEN + 1), "nw", "nw_ptr", A3_P_I);
	a3data_t *nw_pack = artico3_alloc(sizeof(*nw_pack) * executions * (3 * NW_ALEN + 3 * NW_BLEN), "nw", "nw_pack", A3_P_IO);

	if(NW_M == NULL || nw_ptr == NULL || nw_pack == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct nw_bench_args_t *nw_args = (struct nw_bench_args_t *)nw_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < ((NW_ALEN+1) * (NW_BLEN+1)); j++) {
			NW_M[i*(NW_ALEN+1)*(NW_BLEN+1) + j] = nw_args->M[j];
			nw_ptr[i*(NW_ALEN+1)*(NW_BLEN+1) + j] = nw_args->ptr[j];
		}
		for(j = 0; j < NW_ALEN; j++) {
			nw_pack[i*(3*NW_ALEN + 3*NW_BLEN) + j] = nw_args->seqA[j];
		}
		for(j = 0; j < NW_BLEN; j++) {
			nw_pack[i*(3*NW_ALEN + 3*NW_BLEN) + NW_ALEN + j] = nw_args->seqB[j];
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("nw", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("nw");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < (NW_ALEN + NW_BLEN); j++) {
		nw_args->alignedA[j] = nw_pack[(NW_ALEN + NW_BLEN) + j];
		nw_args->alignedB[j] = nw_pack[(2*NW_ALEN + 2*NW_BLEN) + j];
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("nw", "NW_M");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("nw", "nw_ptr");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("nw", "nw_pack");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief QUEUE kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param queue_vargs Pointer to the kernel inputs and outputs buffers
 */
void queue_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *queue_vargs) {

	print_debug("Execution QUEUE...\n");

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" queue accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("queue", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *queue_edges = artico3_alloc(sizeof(*queue_edges) * executions * QUEUE_N_EDGES, "queue", "queue_edges", A3_P_I);
	a3data_t *queue_pack = artico3_alloc(sizeof(*queue_pack) * executions * (3 * QUEUE_N_NODES + QUEUE_N_LEVELS + 1), "queue", "queue_pack", A3_P_IO);

	if(queue_edges == NULL || queue_pack == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct queue_bench_args_t *queue_args = (struct queue_bench_args_t *)queue_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < QUEUE_N_EDGES; j++) {
			queue_edges[i*QUEUE_N_EDGES + j] = queue_args->edges[j].dst;
		}
		for(j = 0; j < QUEUE_N_NODES; j++) {
			queue_pack[i*(3*QUEUE_N_NODES + QUEUE_N_LEVELS + 1) + (0*QUEUE_N_NODES) + j] = queue_args->nodes[j].edge_begin;
			queue_pack[i*(3*QUEUE_N_NODES + QUEUE_N_LEVELS + 1) + (1*QUEUE_N_NODES) + j] = queue_args->nodes[j].edge_end;
			queue_pack[i*(3*QUEUE_N_NODES + QUEUE_N_LEVELS + 1) + (2*QUEUE_N_NODES) + 1 + j] = queue_args->level[j];
		}
		queue_pack[i*(3*QUEUE_N_NODES + QUEUE_N_LEVELS + 1) + (2*QUEUE_N_NODES)] = queue_args->starting_node;

		// Initialize output (sometimes the output value is kept as when the execution started, expecting it to be zero, but the artico3_alloc()
		// sometimes put trash on it, so it's important to initialize the values to 0 (the struct bulk_bench_args_t is memset to 0 in input_to_data()))
		for(j = 0; j < QUEUE_N_LEVELS; j++) {
			queue_pack[i*(3*QUEUE_N_NODES + QUEUE_N_LEVELS + 1) + (2*QUEUE_N_NODES) + 1 + j] = queue_args->level[j];
			queue_pack[i*(3*QUEUE_N_NODES + QUEUE_N_LEVELS + 1) + (3*QUEUE_N_NODES) + 1 + j] = queue_args->level_counts[j];
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("queue", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("queue");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < QUEUE_N_LEVELS; j++) {
		queue_args->level[j] = queue_pack[(2*QUEUE_N_NODES) + 1 + j];
		queue_args->level_counts[j] = queue_pack[(3*QUEUE_N_NODES) + 1 + j];
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("queue", "queue_edges");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("queue", "queue_pack");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief STENCIL2D kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param stencil2d_vargs Pointer to the kernel inputs and outputs buffers
 */
void stencil2d_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *stencil2d_vargs) {

	print_debug("Execution STENCIL2D...\n");

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" stencil2d accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("stencil2d", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *stencil2d_orig = artico3_alloc(sizeof(*stencil2d_orig) * executions * stencil2d_row_size * stencil2d_col_size, "stencil2d", "stencil2d_orig", A3_P_I);
	a3data_t *stencil2d_sol = artico3_alloc(sizeof(*stencil2d_sol) * executions * stencil2d_row_size * stencil2d_col_size, "stencil2d", "stencil2d_sol", A3_P_O);
	a3data_t *stencil2d_filter = artico3_alloc(sizeof(*stencil2d_filter) * executions * stencil2d_f_size, "stencil2d", "stencil2d_filter", A3_P_I);

	if(stencil2d_orig == NULL || stencil2d_sol == NULL || stencil2d_filter == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct stencil2d_bench_args_t *stencil2d_args = (struct stencil2d_bench_args_t *)stencil2d_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < (stencil2d_row_size * stencil2d_col_size); j++) {
			stencil2d_orig[i*stencil2d_row_size*stencil2d_col_size + j] = stencil2d_args->orig[j];
		}
		for(j = 0; j < stencil2d_f_size; j++) {
			stencil2d_filter[i*stencil2d_f_size + j] = stencil2d_args->filter[j];
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("stencil2d", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("stencil2d");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < (stencil2d_row_size * stencil2d_col_size); j++) {
		stencil2d_args->sol[j] = stencil2d_sol[j];
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("stencil2d", "stencil2d_orig");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("stencil2d", "stencil2d_sol");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("stencil2d", "stencil2d_filter");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief STENCIL3D kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param stencil3d_vargs Pointer to the kernel inputs and outputs buffers
 */
void stencil3d_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *stencil3d_vargs) {

	print_debug("Execution STENCIL3D...\n");

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" stencil3d accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("stencil3d", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *stencil3d_orig = artico3_alloc(sizeof(*stencil3d_orig) * executions * STENCIL3D_SIZE, "stencil3d", "stencil3d_orig", A3_P_I);
	a3data_t *stencil3d_sol = artico3_alloc(sizeof(*stencil3d_sol) * executions * STENCIL3D_SIZE, "stencil3d", "stencil3d_sol", A3_P_O);
	a3data_t *STENCIL3D_C = artico3_alloc(sizeof(*STENCIL3D_C) * executions * 2 , "stencil3d", "STENCIL3D_C", A3_P_I);

	if(stencil3d_orig == NULL || stencil3d_sol == NULL || STENCIL3D_C == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct stencil3d_bench_args_t *stencil3d_args = (struct stencil3d_bench_args_t *)stencil3d_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < STENCIL3D_SIZE; j++) {
			stencil3d_orig[i*STENCIL3D_SIZE + j] = stencil3d_args->orig[j];
		}
		for(j = 0; j < 2; j++) {
			STENCIL3D_C[i*2 + j] = stencil3d_args->C[j];
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("stencil3d", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("stencil3d");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < STENCIL3D_SIZE; j++) {
		stencil3d_args->sol[j] = stencil3d_sol[j];
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("stencil3d", "stencil3d_orig");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("stencil3d", "stencil3d_sol");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("stencil3d", "STENCIL3D_C");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}

/**
 * @brief STRIDED kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param strided_vargs Pointer to the kernel inputs and outputs buffers
 */
void strided_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *strided_vargs) {

	print_debug("Execution STRIDED...\n");

	/* Execution */

	#if ARTICO

	int i,j,ret;

	// Internal variable
	int cu = kernel->cu;
	int slot_id = kernel->slot_id;
	int executions = kernel->num_executions;
	struct timespec *start = &(kernel->measured_arrival_time);
	struct timespec *finish = &(kernel->measured_finish_time);

	// Load "cu" strided accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_load("strided", j, 0, 0, 1);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when loading kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	// Allocate ARTICo3 Buffers
	a3data_t *strided_real = artico3_alloc(sizeof(*strided_real) * executions * 1024, "strided", "strided_real", A3_P_IO);
	a3data_t *strided_img = artico3_alloc(sizeof(*strided_img) * executions * 1024, "strided", "strided_img", A3_P_IO);
	a3data_t *strided_real_twid = artico3_alloc(sizeof(*strided_real_twid) * executions * 512, "strided", "strided_real_twid", A3_P_I);
	a3data_t *strided_img_twid = artico3_alloc(sizeof(*strided_img_twid) * executions * 512, "strided", "strided_img_twid", A3_P_I);

	if(strided_real == NULL || strided_img == NULL || strided_real_twid == NULL || strided_img_twid == NULL) {
		print_error("[Kernel - ARTICo3] Error when allocating memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Copy Inputs
	struct strided_bench_args_t *strided_args = (struct strided_bench_args_t *)strided_vargs;

	for(i = 0; i < executions; i++) {
		for(j = 0; j < 1024; j++) {
			strided_real[i*1024 + j] = ftoa3(strided_args->real[j]);
			strided_img[i*1024 + j] = ftoa3(strided_args->img[j]);
		}
		for(j = 0; j < 512; j++) {
			strided_real_twid[i*512 + j] = ftoa3(strided_args->real_twid[j]);
			strided_img_twid[i*512 + j] = ftoa3(strided_args->img_twid[j]);
		}
	}

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif

	// Measure start time
	clock_gettime(CLOCK_MONOTONIC, start);

	// Execute kernel
  	ret = artico3_kernel_execute("strided", executions, 1);
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when executing kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Wait kernel
  	ret = artico3_kernel_wait("strided");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Measure finish time
	clock_gettime(CLOCK_MONOTONIC, finish);

	// Copy Outputs
	for(j = 0; j < 1024; j++) {
		strided_args->real[j] = a3tof(strided_real[j]);
		strided_args->img[j] = a3tof(strided_img[j]);
	}

	// ARTICo3 Free Buffer and release kernels
	ret = artico3_free("strided", "strided_real");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("strided", "strided_img");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("strided", "strided_real_twid");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when waiting for kernel (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}
	ret = artico3_free("strided", "strided_img_twid");
	if(ret != 0) {
		print_error("[Kernel - ARTICo3] Error when freeing memory (%d). k_id: %d\n", ret, kernel->temp_id);
		exit(1);
	}

	// Unload "cu" accelerators
	for(i = 0, j = 0; i < cu; j++) {
		if((slot_id & (1 << j)) > 0) {
			ret = artico3_unload(j);
			if(ret != 0) {
				print_error("[Kernel - ARTICo3] Error when unloading a kernel (%d). k_id: %d\n", ret, kernel->temp_id);
				exit(1);
			}
			i++;
		}
	}

	#else

	#if MONITOR
	// Insert kernel in online queue
	_kernel_to_online_queue(online_queue, online_queue_lock, kernel);
	#endif
	// Simulate execution (remove when using artico)
	int execution_rand = ((rand()%4)+7)*1000;  // [7,11) ms
	print_debug("Tiempo Ejecucion: %d\n",execution_rand);
	usleep(execution_rand);

	#endif
}