/**
 * @file setup.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief Main setup application
 * @date September 2022
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <limits.h>
#include <sys/un.h>
#include <sys/mman.h>

#include "setup.h"

#include "debug.h"
#include "support.h"
#include "queue_kernel.h"
#include "queue_traces.h"
#include "queue_online.h"
#include "kernels_support.h"
#include "thread_pool.h"
#include "cpu_usage.h"

#include "client_socket_tcp.h"

#if ONLINE_MODELS
	#include "online_models.h"
#endif

#if MONITOR
	#include "monitor.h"

	#if BOARD == ZCU
		#define MONITOR_POWER_SAMPLES  (131072)
		#define MONITOR_TRACES_SAMPLES (16384)
	#elif BOARD == PYNQ
		#define MONITOR_POWER_SAMPLES  (65536)
		#define MONITOR_TRACES_SAMPLES (16384)
#endif

	#if TRACES_RAM
		#include "ping_pong_buffers.h"
		#include "execution_modes_buffers.h"
	#endif
#endif

// Needed when compiling -lrt statically linked
// TODO: uncomment, its been done for testing with artico3 daemon
// #define SHMDIR "/dev/shm/"
// const char * __shm_directory(size_t * len)
// {
// 	*len = sizeof(SHMDIR) - 1;
// 	return SHMDIR;
// }

/************************ Application Constants ******************************/

// Number of kernels to be executed
#define NUM_KERNELS 20000
// Monitoring period in ms
#define MONITORING_PERIOD_MS 500
// Number of monitoring windows (-1 if forever)
#define MONITORING_WINDOWS_NUMBER -1
// Number of measurements per training
#define MONITORING_MEASUREMENTS_PER_TRAINING 200
// CPU usage monitoring period in ms
#define CPU_USAGE_MONITOR_PERIOD_MS 150
// Number of available ARTICo3 slots
#if BOARD == ZCU
	// #define NUM_SLOTS 8
	// #define NUM_SLOTS 4
	// #define NUM_SLOTS 2
	// #define NUM_SLOTS 1
	#define NUM_SLOTS 8
#elif BOARD == PYNQ
	#define NUM_SLOTS 4
#endif

/*****************************************************************************/


/************************** Global Setup Variables ***************************/
execution_funct_type kernel_execution_functions[TYPES_OF_KERNELS] = {
	aes_execution,
	bulk_execution,
	crs_execution,
	kmp_execution,
	knn_execution,
	merge_execution,
	nw_execution,
	queue_execution,
	stencil2d_execution,
	stencil3d_execution,
	strided_execution};

// For avoiding duplicated kernel executions in ARTICo3
pthread_mutex_t duplicated_kernel_lock = PTHREAD_MUTEX_INITIALIZER;
int duplicated_kernel_variable[TYPES_OF_KERNELS] = {0};

// For knowing and signaling when there are kernels to be executed
pthread_mutex_t kernel_service_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t kernel_service_condition = PTHREAD_COND_INITIALIZER;
int kernels_to_serve_variable = 0;
int kernels_are_executable_variable = 1;
int free_slots_variable = NUM_SLOTS;

// For knowing which ARTICo3 slots in particular are free
pthread_mutex_t check_slots_lock = PTHREAD_MUTEX_INITIALIZER;
int slots_in_use[NUM_SLOTS];

// Kernel queues for tracking execution and writing info at the end
queue kernel_execution_queue, kernel_output_queue;
// For execution kernel queue synchronization
pthread_mutex_t kernel_execution_queue_lock = PTHREAD_MUTEX_INITIALIZER;

// Online queue for handling online processing of traces (one per ARTICo3 slot)
queue_online online_queue[NUM_SLOTS];
pthread_mutex_t online_queue_lock[NUM_SLOTS] = {PTHREAD_MUTEX_INITIALIZER};
// Flag signaling end of monitoring procress
volatile int monitorization_stop_flag = 0;

// Global thread pool variable
thread_pool *tpool;

// System Operating Mode Enum
enum operating_mode_enum {EXECUTION, TRAIN}operating_mode;

// TODO: Condition and variable used to check if last workload has finished
pthread_cond_t workload_finished_condition = PTHREAD_COND_INITIALIZER;
int workload_finished_flag = 0;

// Structure to measure python impact
struct timespec t_python;

// CPU usage measurement global variables
#if CPU_USAGE
	unsigned long long current_cpu_usage[CPU_USAGE_STAT_COLUMNS] = {0};
	unsigned long long previous_cpu_usage[CPU_USAGE_STAT_COLUMNS] = {0};
	float calculated_cpu_usage[CPU_USAGE_STAT_COLUMNS] = {0.0f};
#endif

#if ONLINE_MODELS
	// Online models struct
	online_models_t online_models;
#endif


/*****************************************************************************/


/**
 * @brief Initialize the queues and socket for the online traces acquisition process
 *
 */

void online_setup(void) {

	int i;

	// Initialize online info queue and mutexes
	for(i = 0; i < NUM_SLOTS; i++) {

		// Init queue
		init_queue_online(&(online_queue[i]));

		// Init mutex
		if(pthread_mutex_init(&(online_queue_lock[i]), NULL) < 0) {
			print_error("online_queue_lock_init [%d]\n", i);
			exit(1);
		}
	}

	#if ONLINE_MODELS
		// Initialize online models interaction stuff
		online_models_setup(&online_models, MONITORING_MEASUREMENTS_PER_TRAINING);
	#endif

}

/**
 * @brief Clean the queues and sockets used for the online traces acquisition process
 *
 */
void online_clean(void) {

	int i, ret;

	// Destroy online info queue
	for(i = 0; i < NUM_SLOTS; i++) {
		// Clean the online info queue
		clean_queue_online(&(online_queue[i]));
		// Destroy the mutexes
		if(pthread_mutex_destroy(&(online_queue_lock[i])) < 0) {
			print_error("online_queue_lock_destroy [%d]\n", i);
			exit(1);
		}
	}


	// TODO: Comento esto porque se llama directamente desde el setup a esta función antes
	//       de esperar a los hilos del monitor y el uso de la cpu (por si pasa que el monitor está
	//       dormido casualmente). Si no se comentase se llamaría dos veces, lo que daría un error
	//       al intentar comunicarte (la segunda vez) con un socket que ya no existe (se destruyó e
	//       en la primera llamada).

	// #if ONLINE_MODELS
	// 	// Clean online models interaction stuff
	// 	online_models_clean(&online_models);
	// #endif

}

/**
 * @brief Generates a file containing the information about which kernel is executed in each slots thoughout a whole monitoring window
 *
 * @param monitor_window Pointer to the monitoring window to be processed
 * @param monitorization_count Id of this monitoring window. Used for file naming purposes (TODO:to be removed)
 * @param online_ptr Buffer to store online data in
 * @return (int) 0 on success, error code otherwise
 */
int online_processing(float user_cpu, float kernel_cpu, float idle_cpu, const monitor_data *monitor_window, const int monitorization_count, char *online_ptr, int cloud_socket_id) {

	// Make the usage a struct

	#if MONITOR

	int i, ret, cont;

	// Writting to file variables (file mode)
	#if TRACES_RAM
		int online_ram_num_bytes = 0;
	#endif
	#if TRACES_ROM
		int online_fd;
		char online_file_name[40];
		int online_rom_num_bytes = 0;
	#endif
	#if TRACES_SOCKET
		char *online_cloud_ptr;
		int online_cloud_bytes = 0;

		online_cloud_ptr = malloc(ONLINE_FILE_SIZE);
		if (online_cloud_ptr == NULL) {
			printf("Null online cloud ptr\n");
		}
		print_info("online cloud ptr ok init\n");
	#endif

	// Monitoring window kernels data variables
	kernel_data *kernel_tmp;
	int kernels_to_keep;
	int num_kernels_written;
	int num_kernels_in_slot;

	// Write and keep kernel decision variables
	struct timespec t0, tf, m0, mf; // t stands for execution time, m stands for monitorization time

	// Create separation mark
	int separation_mark;

	// Auxiliar struct used to write each kernel data from the online queue
	// ** It is used the structure instead of writing to a file each desired field of the kernel_data structure
	// ** Because that would create a file with unaligned data (unless we introduce some padding) which
	// ** takes more time to be processed afterward and it's harder to handle it with python struct module
	online_data online_tmp;

	// Structure of the file
	//										                                                        | next is a kernel| k | next is a kernel| k | next is a kernel| k | next is a kernel| k | next slot | next is a kernel| k | next is a kernel| k | next slot |  Repeted num_slots times |
	// | user cpu usage | kernel cpu usage | idle cpu usage | monitor_window_data | number_of_slots |        1        | . |        1        | . |        1        | . |        1        | . |     0     |        1        | . |        1        | . |     0     | ..... |

	// Create file with info about last monitoring window (to be process online)
	print_debug("\n[ONLINE] Online Info - Start the online info processing #%d\n", monitorization_count);

	// File creation (file mode) #Note: Shared memories already created in ram mode
	#if TRACES_ROM
		snprintf(online_file_name, 40, "../outputs/online_%d.bin", monitorization_count);

		// Open a file to store online data in binary mode
		online_fd = open(online_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if(online_fd < 0) {
			print_error("[ONLINE] Error when opening the file %s\n", online_file_name);
			exit(1);
		}
	#endif

	// Write monitoring window info to the file
	#if TRACES_RAM
		#if CPU_USAGE
			memcpy(&(online_ptr[online_ram_num_bytes]), &user_cpu, sizeof(user_cpu));
			online_ram_num_bytes += sizeof(user_cpu);
			memcpy(&(online_ptr[online_ram_num_bytes]), &kernel_cpu, sizeof(kernel_cpu));
			online_ram_num_bytes += sizeof(kernel_cpu);
			memcpy(&(online_ptr[online_ram_num_bytes]), &idle_cpu, sizeof(idle_cpu));
			online_ram_num_bytes += sizeof(idle_cpu);
		#endif
		memcpy(&(online_ptr[online_ram_num_bytes]), monitor_window, sizeof(*monitor_window));
		online_ram_num_bytes += sizeof(*monitor_window);
	#endif
	#if TRACES_ROM
		// Test write cpu info TODO DEBUG
		#if CPU_USAGE
		online_rom_num_bytes += write(online_fd, &user_cpu, sizeof(user_cpu));
		online_rom_num_bytes += write(online_fd, &kernel_cpu, sizeof(kernel_cpu));
		online_rom_num_bytes += write(online_fd, &idle_cpu, sizeof(idle_cpu));
		#endif

		online_rom_num_bytes += write(online_fd, monitor_window, sizeof(*monitor_window));
		if(online_rom_num_bytes < 0) {
			print_error("[ONLINE] Error when writting to file %s\n", online_file_name);
			exit(1);
		}
	#endif
	#if TRACES_SOCKET
		#if CPU_USAGE
		memcpy(&(online_cloud_ptr[online_cloud_bytes]), &user_cpu, sizeof(user_cpu));
		online_cloud_bytes += sizeof(user_cpu);
		memcpy(&(online_cloud_ptr[online_cloud_bytes]), &kernel_cpu, sizeof(kernel_cpu));
		online_cloud_bytes += sizeof(kernel_cpu);
		memcpy(&(online_cloud_ptr[online_cloud_bytes]), &idle_cpu, sizeof(idle_cpu));
		online_cloud_bytes += sizeof(idle_cpu);
		#endif

		memcpy(&(online_cloud_ptr[online_cloud_bytes]), monitor_window, sizeof(*monitor_window));
		online_cloud_bytes += sizeof(*monitor_window);
	#endif

	// Write the number of slots
	separation_mark = NUM_SLOTS;

	#if TRACES_RAM
		memcpy(&(online_ptr[online_ram_num_bytes]), &separation_mark, sizeof(separation_mark));
		online_ram_num_bytes += sizeof(separation_mark);
	#endif
	#if TRACES_ROM
		online_rom_num_bytes += write(online_fd, &separation_mark, sizeof(separation_mark));
		if(online_rom_num_bytes < 0) {
			print_error("[ONLINE] Error when writting to file %s\n", online_file_name);
			exit(1);
		}
	#endif
	#if TRACES_SOCKET
		memcpy(&(online_cloud_ptr[online_cloud_bytes]), &separation_mark, sizeof(separation_mark));
		online_cloud_bytes += sizeof(separation_mark);
	#endif

	// Process to decide which kernel to write to online file  and which kernels to keep
	// in the online queues (could appear in future monitoring windows)
	//
	// We have to remove every kernel that has finish its execution previously or withing this monitoring windows
	// Remove = tf <= mf -> (to remove less or equal to) De Morgan -> Remove = not (tf > mf) -> Keep = tf > mf
	//
	// We have to write to online file only the kernels that have been executing while the monitoring window
	// Write = not(tf <= m0 or t0 >= mf) -> De Morgan (to remove less or equal to) -> Write = tf > m0 and t0 < mf
	//
	// * Since t0 and tf are initialize to LONG_MAX by default, if the t0 or/and tf of a kernel has not been set in
	// * the execution thread yet, they will have MAX values, which make the Write decission to be managed by design;
	// * but for the Keep decission it could happen that a kernel has t0 and tf not set. Since we want to keep it
	// * we will have to check if t0 == tf, because they will be set to MAX. (must be ORed with the other condition)
	// * It has been decided to design it this way since there are no t0/tf default values that handle both decission
	// * by design.
	for(i = 0; i < NUM_SLOTS; i++) {

		// Lock to ensure no race condition
		if(pthread_mutex_lock(&(online_queue_lock[i])) < 0) {
			print_error("[ONLINE] online_queue_lock_0 [%d] - Online Info\n", i);
			exit(1);
		}

		// Get num kernels in slot
		num_kernels_in_slot = get_size_queue_online(&(online_queue[i]));

		// Unlock the mutex
		if(pthread_mutex_unlock(&(online_queue_lock[i])) < 0) {
				print_error("[ONLINE] online_queue_unlock_0 [%d]- Online Info\n", i);
			exit(1);
		}

		print_debug(" %*s | %*s | %*s | %*s | %*s | %*s\n", -10,"SLOT",-10,"Kernel",-14,"t0",-14,"m0",-14,"tf",-14,"mf");

		for(cont = 0, num_kernels_written = 0, kernels_to_keep = 0; cont < num_kernels_in_slot; cont++) {

			// Lock to ensure no race condition
			if(pthread_mutex_lock(&(online_queue_lock[i])) < 0) {
				print_error("[ONLINE] online_queue_lock_1 [%d] - Online Info\n", i);
				exit(1);
			}

			if(dequeue_online(&(online_queue[i]), &kernel_tmp) < 0) {
				print_error("[ONLINE] Error getting online #%d from the online info queue of slot #%d\n",cont, i);
				exit(1);
			}

			// Unlock the mutex
			if(pthread_mutex_unlock(&(online_queue_lock[i])) < 0) {
				print_error("[ONLINE] online_queue_unlock_1 [%d]- Online Info\n", i);
				exit(1);
			}

			print_debug("\t[ONLINE] [dequeue] %p\n", kernel_tmp);
			print_debug("\t[ONLINE] [dequeue] %*d:%09ld | %*d:%09ld\n", 3,kernel_tmp->measured_arrival_time.tv_sec,kernel_tmp->measured_arrival_time.tv_nsec,3,kernel_tmp->measured_finish_time.tv_sec,kernel_tmp->measured_finish_time.tv_nsec);
			print_debug("\t[ONLINE] [dequeue] %ld:%09ld | %ld:%09ld\n", kernel_tmp->measured_arrival_time.tv_sec,kernel_tmp->measured_arrival_time.tv_nsec,kernel_tmp->measured_finish_time.tv_sec,kernel_tmp->measured_finish_time.tv_nsec);

			// Usefull variables
			t0 = kernel_tmp->measured_arrival_time;
			tf = kernel_tmp->measured_finish_time;
			m0 = monitor_window->measured_starting_time;
			mf = monitor_window->measured_finish_time;

			print_debug(" %*d | %*d | %*d:%09ld | %*d:%09ld | %*d:%09ld | %*d:%09ld\n", 10,i,10,cont,3,t0.tv_sec,t0.tv_nsec,3,m0.tv_sec,m0.tv_nsec,3,tf.tv_sec,tf.tv_nsec,3,mf.tv_sec,mf.tv_nsec);

			// Decide to write to online or not
			if(greater_than_timespec(tf,m0) && less_than_timespec(t0,mf)) {

				// Write the file

				// Mark to indicate next thing is a kernel
				separation_mark = 1;
				#if TRACES_RAM
					memcpy(&(online_ptr[online_ram_num_bytes]), &separation_mark, sizeof(separation_mark));
					online_ram_num_bytes += sizeof(separation_mark);
				#endif
				#if TRACES_ROM
					online_rom_num_bytes += write(online_fd, &separation_mark, sizeof(separation_mark));
					if(online_rom_num_bytes < 0) {
						print_error("[ONLINE] Error when writting to file %s\n", online_file_name);
						exit(1);
					}
				#endif
				#if TRACES_SOCKET
					memcpy(&(online_cloud_ptr[online_cloud_bytes]), &separation_mark, sizeof(separation_mark));
					online_cloud_bytes += sizeof(separation_mark);
				#endif

				// (warning) Estamos leyendo un puntero que puede ser modificado por otro hilo, podríamos leer basura
				// Escribir solo: tipo de kernel, arrival, finish
				//
				// ** It is used the structure instead of writing to a file each desired field of the kernel_data structure
				// ** Because that would create a file with unaligned data (unless we introduce some padding) which
				// ** takes more time to be processed afterward and it's harder to handle it with python struct module
				//
				online_tmp.kernel_label = kernel_tmp->kernel_label;
				online_tmp.arrival_time = t0;
				online_tmp.finish_time = tf;

				// Write the online data
				#if TRACES_RAM
					memcpy(&(online_ptr[online_ram_num_bytes]), &online_tmp, sizeof(online_tmp));
					online_ram_num_bytes += sizeof(online_tmp);
				#endif
				#if TRACES_ROM
					online_rom_num_bytes += write(online_fd, &online_tmp, sizeof(online_tmp));
					if(online_rom_num_bytes < 0) {
						print_error("[ONLINE] Error when writting to file %s\n", online_file_name);
						exit(1);
					}
				#endif
				#if TRACES_SOCKET
					memcpy(&(online_cloud_ptr[online_cloud_bytes]), &online_tmp, sizeof(online_tmp));
					online_cloud_bytes += sizeof(online_tmp);
				#endif

				num_kernels_written++;
			}

			// Decide to keep
			if(greater_than_timespec(tf,mf) || equal_to_timespec(t0,tf)) {

				// Lock to ensure no race condition
				if(pthread_mutex_lock(&(online_queue_lock[i])) < 0) {
					print_error("[ONLINE] online_queue_lock_2 [%d] - Online Info\n", i);
					exit(1);
				}

				ret = enqueue_online(&(online_queue[i]), kernel_tmp);
				if(ret != 0) {
					print_error("[ONLINE] Error readding kernel pointer to online queue #%d\n",i);
					exit(1);
				}

				// Unlock the mutex
				if(pthread_mutex_unlock(&(online_queue_lock[i])) < 0) {
				print_error("[ONLINE] online_queue_unlock_2 [%d]- Online Info\n", i);
					exit(1);
				}

				kernels_to_keep++;
			}
		}

		// Mark to indicate next thing is the next slot
		separation_mark = 0;

		#if TRACES_RAM
			memcpy(&(online_ptr[online_ram_num_bytes]), &separation_mark, sizeof(separation_mark));
			online_ram_num_bytes += sizeof(separation_mark);
		#endif
		#if TRACES_ROM
			online_rom_num_bytes += write(online_fd, &separation_mark, sizeof(separation_mark));
			if(online_rom_num_bytes < 0) {
				print_error("[ONLINE] Error when writting to file %s\n", online_file_name);
				exit(1);
			}
		#endif
		#if TRACES_SOCKET
			memcpy(&(online_cloud_ptr[online_cloud_bytes]), &separation_mark, sizeof(separation_mark));
			online_cloud_bytes += sizeof(separation_mark);
		#endif

		print_debug("[ONLINE] Online info -> Written %d/%d kernels from [SLOT #%d]\n", num_kernels_written, num_kernels_in_slot, i);
		print_debug("[ONLINE] Online info -> Kept %d/%d kernels from [SLOT #%d]\n", kernels_to_keep, num_kernels_in_slot, i);

		// Print usefull information
		print_debug("[ONLINE] Online info -> Total num_bytes = %d\n\n",online_ram_num_bytes);
	}

	// Mark the online file size (ram mode) / Close the file (file mode)
	#if TRACES_RAM
		// Truqui para escribir el tamaño al final del fichero
		((int *)online_ptr)[ONLINE_FILE_SIZE / 4 - 1] = online_ram_num_bytes;

		// Print usefull information
		print_info("[ONLINE] Online info -> Total num_bytes = %d\n\n",online_ram_num_bytes);
	#endif
	#if TRACES_ROM
		close(online_fd);
	#endif
	#if TRACES_SOCKET
		send_buffer_socket_tcp_inet(cloud_socket_id, online_cloud_ptr, online_cloud_bytes);
		free(online_cloud_ptr);
	#endif

	return 0;

	#else

	return 0;

	#endif

}

/**
 * @brief Function executed by the queue manager thread.
 * 			- Decides which kernel is executed and dispatches the execution to a thread pool worker.
 *
 * @param arg Not used.
 */
void* queue_manager_thread(void *arg) {

	int i, j, ret, kernels_count = 0;

	// Timer
	struct timespec t_aux;

	// Kernel tmp variable
	kernel_data kernel_tmp;

	// Pointer to the element in the queue that stores the information of the kernel to be executed by the execution thread
	// A pointer to the element in the queue is needed since we have to overwrite some information from another thread
	kernel_data *kernel_tmp_ptr;

	// Variables to handle duplicated kernels and not enough slots
	int end_of_queue_flag = 0;
	int free_slots_tmp = 0;
	int duplicated_kernels_tmp[TYPES_OF_KERNELS];

	// TODO: Test num_workloads
	int num_workloads = *((int *) arg);
	int workload_index;

	// TODO: Remove. Used to measure app time
	struct timespec t_start, t_end, t_app_elapsed_time;
	// Get start time
	clock_gettime(CLOCK_MONOTONIC, &t_start);

	// Read from kernel execution queue
	// TODO: kernel_count llegará a NUM_KERNELS * num_worloads, para ejecutar todas las workloads
	for (workload_index = 0; workload_index < num_workloads; workload_index++) {

		printf("Queue Manager - Start Workload -> #%d\n", workload_index);

		kernels_count = 0;

		while(kernels_count < NUM_KERNELS) {

			print_debug("Queue Manager - Pre-add kernel -> #%d\n", kernels_count);
			// Wait until the kernel generation thread adds a new kernel to the queue or any executing kernel finishes
			if(pthread_mutex_lock(&kernel_service_lock) < 0) {
				print_error("Queue Manager - kernel_service_lock_0\n");
				exit(1);
			}
			while(kernels_to_serve_variable == 0 || kernels_are_executable_variable == 0 || free_slots_variable == 0 || operating_mode == TRAIN) {

				if(pthread_cond_wait(&kernel_service_condition, &kernel_service_lock) < 0) {
					print_error("Queue Manage - kernel_service_wait\n");
					exit(1);
				}

			}
			// Get free slots
			free_slots_tmp = free_slots_variable;

			// Set kernels_are_executable_variable to 0
			//
			// This variable indicates whether the thread should check for executables kernels in the queue or not (1 = yes, 0 = no)
			// This variable, within this thread, it should only be modified at the end.
			// * It should be set to 0 when all the waiting kernels are checked (i.e. when we reach to the end)
			// 	 because if they are all checked it means there is no other remaining kernels that could be executable.
			// * And it should be set to 1 when we haven't checked all the waiting kernels
			//   because there could an executable kernel among the remaining unchecked ones.
			//
			// # But this could be a problem because the variable is also set to 1 by another thread whenever a kernel finish its execution
			//   because it means that there are more free slots that could be used by the already checked kernels.
			//   And, it could happen that while this thread check the waiting kernels the other finishes the execution of a kernel and sets the variable to 1
			//   coinciding that, when this thread finished it reaches to the end of the queue and set the variable to 0 hiding the change of the other thread
			//   which could lead to a stall since if there is no other change in the variable (no other kernel finished, or no other new kernel appears) this thread
			//   will not check again the kernels.
			// This case is highly unprobable but it could happen for really fast kernels, so the solution proposed is the following:
			//   - The variable is goind to be set here to 0.
			//   - At the end of the thread:
			//			- It will be set to 1 if we have NOT reached the end of the queue, because there are still kernels in the queue to be checked
			//			- It will be kept as it is if we HAVE reached the end of the queue, because there are two cases:
			//					* The variable is still 0, which means that no other thread have changed its value.
			//					  This means there aren't new kernels and none of the previous have finished its execution.
			//					  In this case, since there aren't executable kernels in the queue the variable must be 0, as it already is.
			//					* The variable is now 1, which means that other thread has changed its value.
			//					  This means that there is either a new kernel or any of the previous have finished its execution.
			// 					  In both cases there are still some kernels in the queue that could be executable, so the variable must be 1, as it already is.
			//
			kernels_are_executable_variable = 0;

			if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
				print_error("Queue Manager - kernel_service_unlock_0\n");
				exit(1);
			}

			// Get duplicated kernels
			print_debug("Get Dup -> #%d\n", kernels_count);
			if(pthread_mutex_lock(&duplicated_kernel_lock) < 0) {
				print_error("duplicated_kernel_lock_0\n");
				exit(1);
			}
			memcpy(duplicated_kernels_tmp, duplicated_kernel_variable, sizeof(duplicated_kernels_tmp));
			if(pthread_mutex_unlock(&duplicated_kernel_lock) < 0) {
				print_error("duplicated_kernel_unlock_0\n");
				exit(1);
			}


			// Get kernel info from pos element of the queue
			print_debug("Queue Manager - Pre-dequeue first executable kernel -> #%d\n", kernels_count);
			if(pthread_mutex_lock(&kernel_execution_queue_lock) < 0) {
				print_error("kernel_execution_queue_lock - get info\n");
				exit(1);
			}
			// If we reach the end of the queue, set a flag
			if(dequeue_first_executable_kernel(&kernel_execution_queue, free_slots_tmp, duplicated_kernels_tmp, &kernel_tmp) < 0)
				end_of_queue_flag = 1;
			if(pthread_mutex_unlock(&kernel_execution_queue_lock) < 0) {
				print_error("kernel_execution_queue_unlock - get info\n");
				exit(1);
			}

			// If we have reached the end of the queue:
			// - Leave kernels_are_executable_variable as it is (explained at the begining of the thread)
			// - Clean the end of queue flag for next iteration
			// - Go back to the front of the queue
			// - Move to the next loop iteration
			// If we have not reached the end of the queue:
			// - Set kernels_are_executable_variable to 1 (explained at the begining of the thread)
			if(end_of_queue_flag == 1) {

				print_debug("Reached the end of the queue #%d -> Move to the front\n", kernels_count);
				end_of_queue_flag = 0;

				continue;
			}
			else{

				if(pthread_mutex_lock(&kernel_service_lock) < 0) {
					print_error("Queue Manager - kernel_service_lock_1\n");
					exit(1);
				}
				kernels_are_executable_variable = 1;
				if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
					print_error("Queue Manager - kernel_service_unlock_1\n");
					exit(1);
				}

			}

			print_debug("Queue Manager - Post-dequeue first executable kernel -> #%d\n", kernels_count);

			// Increment the duplicated_kernel_variable
			if(pthread_mutex_lock(&duplicated_kernel_lock) < 0) {
				print_error("duplicated_kernel_lock_1\n");
				exit(1);
			}
			duplicated_kernel_variable[kernel_tmp.kernel_label]++;
			if(pthread_mutex_unlock(&duplicated_kernel_lock) < 0) {
				print_error("duplicated_kernel_unlock_1\n");
				exit(1);
			}

			// TODO: remove (tmp) Add temporal kernel id
			kernel_tmp.temp_id = kernels_count;

			print_debug("Pre-Slots-Condition -> #%d\n", kernels_count);

			// Decrement the free_slots_variable
			if(pthread_mutex_lock(&kernel_service_lock) < 0) {
				print_error("free_slots_lock_0\n");
				exit(1);
			}
			free_slots_variable -= kernel_tmp.cu;
			if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
				print_error("free_slots_unlock_0\n");
				exit(1);
			}

			print_debug("#%d -> Pre-Mutex #0\n", kernel_tmp.temp_id);

			// Select first "cu" free slots as the loading slots for the actual kernel
			if(pthread_mutex_lock(&check_slots_lock) < 0) {
				print_error("pthread_mutex_lock_0\n");
				exit(1);
			}
			print_debug("#%d -> Critical Section #0\n", kernel_tmp.temp_id);

			for(i = 0, j = 0; i < NUM_SLOTS; i++) {
				if(slots_in_use[i] == 0) {
					slots_in_use[i] = 1;
					kernel_tmp.slot_id |= (1 << i);
					j++;
					if(kernel_tmp.cu == j)
						break;
				}
			}

			if(pthread_mutex_unlock(&check_slots_lock) < 0) {
				print_error("pthread_mutex_unlock_0\n");
				exit(1);
			}

			print_debug("#%d -> Post-Mutex #0 \n", kernel_tmp.temp_id);

			// Add the kernel to the kernel output queue
			kernel_tmp_ptr = enqueue_and_modify(&kernel_output_queue, &kernel_tmp);
			if(kernel_tmp_ptr == NULL) {
				print_error("Error adding kernel #%d to the kernel output queue\n",kernels_count);
				exit(1);
			}

			// Tell a worker of the thread pool to execute the kernel
			ret = dispatch(tpool, &execution_thread, kernel_tmp_ptr);
			if(ret != 0) {
				print_error("Error dispatching a task to the thread pool. k_id: %d\n",kernel_tmp.temp_id);
				exit(1);
			}

			// Kernel is going to be executed:
			// - Decrement kernel_to_serve_variable
			// - Increment kernel count
			// - Go back to the front of the queue
			if(pthread_mutex_lock(&kernel_service_lock) < 0) {
				print_error("Queue Manager - kernel_service_lock_2\n");
				exit(1);
			}
			kernels_to_serve_variable--;
			if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
				print_error("Queue Manager - kernel_service_unlock_2\n");
				exit(1);
			}

			// Print progress
			if(kernels_count%400 == 0) {
				clock_gettime(CLOCK_MONOTONIC, &t_aux);
				print_error("\033[1;33mKernels Executed: (#%d) -> %d (%ld:%09ld)\033[0m\n", workload_index, kernels_count, t_aux.tv_sec, t_aux.tv_nsec);
				// test
				/*#if ONLINE_MODELS
					{
					online_models_features_t features_test = {0,0,1,0,2,0,1,0,1,0,0};
					online_models_prediction_t prediction_test;
					prediction_test = online_models_predict(&online_models, &features_test);
					online_models_print_features(&features_test);
					online_models_print_prediction(&prediction_test);
					}
				#endif*/
			}

			// Increment the kernel counter
			kernels_count++;
		}

		// Clean the kernel execution queue
		clean_queue(&kernel_execution_queue);

		// Ensure each thread in the thread pool has finished before exiting these thread
		while(isdone(tpool) == 0);

		// TODO: Signal last workload has been excuted
		printf("Queue Manager - Last workload has finished\n");
		if(pthread_mutex_lock(&kernel_service_lock) < 0) {
			print_error("Queue Manager - kernel_service_lock (end workload)\n");
			exit(1);
		}
		workload_finished_flag = 1;
		if(pthread_cond_signal(&workload_finished_condition) < 0) {
			print_error("Queue Manager - signal workload_finished_condition (end workload)\n");
			exit(1);
		}
		if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
			print_error("Queue Manager - pthread_mutex_unlock (end workload)\n");
			exit(1);
		}

	}

	// TODO: Remove. Get end time
	clock_gettime(CLOCK_MONOTONIC, &t_end);
	t_app_elapsed_time = diff_timespec(t_start, t_end);
	print_error("\033[1;33mQueue Manager - Start Time: %ld:%09ld\033[0m\n", t_start.tv_sec, t_start.tv_nsec);
	print_error("\033[1;33mQueue Manager - End Time: %ld:%09ld\033[0m\n", t_end.tv_sec, t_end.tv_nsec);
	print_error("\033[1;33mQueue Manager - SETUP Elapsed Time: %ld:%09ld\033[0m\n", t_app_elapsed_time.tv_sec, t_app_elapsed_time.tv_nsec);
	print_error("\033[1;33mQueue Manager - Python Impact: %ld:%09ld (%lf\%)\033[0m\n", t_python.tv_sec, t_python.tv_nsec, calculate_percentage(t_python, t_app_elapsed_time));

	// Exit thread without return argument
	pthread_exit(NULL);
}

/**
 * @brief Funtion executed by each execution thread (performed by the thread pool workers).
 * 		  	- Configures, load, execute, and verify each kernel execution.
 *
 * @param arg Pointer to the kernel data the execution to be performed
 */
void* execution_thread(void *arg) {

	int i,j;

	// Time struct for measuring actual arrival time
	struct timespec temp_time;

	// Done this way because the data contained in the point will be overwritten on the queue manager thread
	kernel_data *kernel = (kernel_data*) arg;

	/********************************************** Kernel Execution *****************************************************/

	// Generate a pointer to the kernel io data
	char* kernel_io_data;

	// Load kernel data
	kernel_copy_input(&kernel_io_data, kernel->kernel_label);

	// Measure pre-execution time
	clock_gettime(CLOCK_MONOTONIC, &temp_time);
	kernel->measured_pre_execution_time = temp_time;

	// Execute kernel
	kernel_execution_functions[kernel->kernel_label](kernel,online_queue, online_queue_lock, kernel_io_data);

	// Measure post-execution time
	clock_gettime(CLOCK_MONOTONIC, &temp_time);
	kernel->measured_post_execution_time = temp_time;

	// Validate kernel output
	kernel_result_validation(&kernel_io_data, kernel->kernel_label);

	/*********************************************************************************************************************/

	// Access shared variable
	print_debug("#%d -> Pre-Mutex #1\n", kernel->temp_id);
	if(pthread_mutex_lock(&check_slots_lock) < 0) {
		print_error("pthread_mutex_lock_1\n");
		exit(1);
	}
	print_debug("#%d -> Critical Section #1\n", kernel->temp_id);
	/************ Critical section ************/

	// Indicate the used slots are again free
	for(i = 0, j = 0; i < kernel->cu; j++) {
		if((kernel->slot_id & (1 << j)) > 0) {		// Check if the j position is set (it is a used slot)
			slots_in_use[j] = 0;
			i++;
		}
	}

	/******************************************/
	if(pthread_mutex_unlock(&check_slots_lock) < 0) {
		print_error("pthread_mutex_unlock_1\n");
		exit(1);
	}
	print_debug("#%d -> Post-Mutex #1 \n", kernel->temp_id);

	// Signal condition
	if(pthread_mutex_lock(&duplicated_kernel_lock) < 0) {
		print_error("duplicated_kernel_lock_1\n");
		exit(1);
	}
	duplicated_kernel_variable[kernel->kernel_label]--;
	if(pthread_mutex_unlock(&duplicated_kernel_lock) < 0) {
		print_error("duplicated_kernel_unlock\n");
		exit(1);
	}
	print_debug("#%d -> Post-DupKer-Condition\n", kernel->temp_id);

	// Increment the free slots_variable (to indicate there are free slots now) and set the kernels_are_executable_variable
	// (to indicate that duplicated kernels might be able to execute now) and signal condition
	if(pthread_mutex_lock(&kernel_service_lock) < 0) {
		print_error("Execution - kernel_service_lock\n");
		exit(1);
	}
	free_slots_variable += kernel->cu;
	kernels_are_executable_variable = 1;
	if(pthread_cond_signal(&kernel_service_condition) < 0) {
		print_error("Execution - kernel_service_signal\n");
		exit(1);
	}
	if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
		print_error("Execution - kernel_service_unlock\n");
		exit(1);
	}
	print_debug("#%d -> Post-kernel-service-condition\n", kernel->temp_id);

	return NULL;

}

/**
 * @brief Function executed by the monitoring thread.
 * 		  	- Obtains power and performance traces periodically with the monitoring infrastructure.
 *        	- Generates online data files that could be used for online training
 *
 * @param arg Pointer to the arguments passed to this function
 */
void* monitoring_thread(void *arg) {

	// TODO: handle the errors

	#if MONITOR

	// DEBUG
	struct timespec start_online, end_online, elapsed_online, interval_online_time;
	struct timespec t_start_online_operation, t_end_online_operation, t_online_operation_elapsed_time;
	struct timespec total_online_time;
	int number_iterations = 0;

	// Timers
	struct timespec schedule_timer, aux_timer;

	// Shared memory handling variables (ram mode) / Monitor traces file handling variables (file mode)
	#if TRACES_RAM
		char *power_ram_ptr, *traces_ram_ptr, *online_ram_ptr;
		int power_ram_bytes, traces_ram_bytes;
	#else
		// Is passed to the online_processing function, but is not used inside
		char *online_ram_ptr = NULL;
	#endif
	#if TRACES_ROM
		int fd_power, fd_traces;
		char power_file_name[25], traces_file_name[25];

		// Monitor general info variables
		int monitor_info_fd;
		int monitor_info_num_bytes = 0;
		char monitor_info_file_name[30] = "../outputs/monitor_info.bin";
	#endif
	#if TRACES_SOCKET
		char *power_cloud_ptr, *traces_cloud_ptr;
		int power_cloud_bytes, traces_cloud_bytes;
		int cloud_socket_id;
		struct sockaddr_in cloud_socket_addr;

		power_cloud_ptr = malloc(POWER_FILE_SIZE);
		if (power_cloud_ptr == NULL) {
			printf("Null power cloud ptr\n");
		}
		traces_cloud_ptr = malloc(TRACES_FILE_SIZE);
		if (traces_cloud_ptr == NULL) {
			printf("Null traces cloud ptr\n");
		}
		printf("power and traces cloud ptrs ok init\n");

		// Create the UDP socket to send data to cloud
		cloud_socket_id = create_socket_tcp_inet(&cloud_socket_addr, "138.100.74.53", 4242);
		if(cloud_socket_id < 0) {
			print_error("Error TCP inet socket creation\n");
			exit(1);
		}
		print_debug("The TCP inet socket has been successfully created\n");
	#else
		// Is passed to the online_processing function, but is not used inside
		int cloud_socket_id = 0;
	#endif

	// Counters
	int i, ret, monitorization_count;

	// Monitor information variables
	monitor_data monitor_window;
	monitor_arguments monitor_args = *((monitor_arguments*) arg);

	// Monitor queue
	queue_monitor monitor_info_queue;

	// Initialize the monitor queue
	init_queue_monitor(&monitor_info_queue);

	print_info("\n[MONITOR] Monitoring Parameters -> period(ms): %u | times: %d | Measurements per Training: %d\n\n", monitor_args.period_ms, monitor_args.num_monitorizations, monitor_args.measurements_per_training);

	// Allocate data buffers for Monitor
	print_debug("[MONITOR] Monitor buffers allocation...\n");
    monitorpdata_t *power  = monitor_alloc(MONITOR_POWER_SAMPLES, "power", MONITOR_REG_POWER);
    monitortdata_t *traces = monitor_alloc(MONITOR_TRACES_SAMPLES, "traces", MONITOR_REG_TRACES);

	// Create shared memories backed on ram (ram mode)
	#if TRACES_RAM
		#if MONITORING_MEASUREMENTS_PER_TRAINING == 1
			ret = ping_pong_buffers_init(&power_ram_ptr, &traces_ram_ptr, &online_ram_ptr);
		#elif MONITORING_MEASUREMENTS_PER_TRAINING > 1
			ret = execution_modes_buffers_init(monitor_args.measurements_per_training, &power_ram_ptr, &traces_ram_ptr, &online_ram_ptr);
		#else
			#error MONITORING_MEASUREMENTS_PER_TRAINING value must be bigger than 0
		#endif
	#endif

	// Initialize a timer
	clock_gettime(CLOCK_MONOTONIC, &schedule_timer);

	// Delay the monitoring start 2 seconds
	schedule_timer.tv_sec +=2;

	// Perform "num_monitorizations" monitoring windows, or until the main thread set the "monitorization_stop_flag"
	monitorization_count = 0;
	while(monitorization_count < monitor_args.num_monitorizations || (monitor_args.num_monitorizations == -1 && monitorization_stop_flag == 0)) {

		print_debug("[MONITOR] Monitorization count #%d wait...\n",monitorization_count);

		// Clean number of bytes (ram mode and socket mode)
		#if TRACES_RAM
			power_ram_bytes = 0;
			traces_ram_bytes = 0;
		#endif
		#if TRACES_SOCKET
			power_cloud_bytes = 0;
			traces_cloud_bytes = 0;
		#endif

		// Wait for an event
	    clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&(schedule_timer), NULL);

		// Log start of the monitorization
		monitor_window.initial_time = monitor_args.initial_time;
		clock_gettime(CLOCK_MONOTONIC, &aux_timer);
		monitor_window.measured_starting_time =aux_timer;

		// CPU usage | TODO:(falta un mutex y tal)
		#if CPU_USAGE
			float user_cpu = calculated_cpu_usage[0];
			float kernel_cpu = calculated_cpu_usage[1];
			float idle_cpu = calculated_cpu_usage[2];
		#else	// Just a place holder for arguments in future functions (not the proper way of doing thing // TODO)
			float user_cpu = 0;
			float kernel_cpu = 0;
			float idle_cpu = 0;
		#endif
		// Start monitor
		monitor_start();

		// Wait for the monitor interrupt
		monitor_wait();

		// Log end of the monitorization
		clock_gettime(CLOCK_MONOTONIC, &aux_timer);
		monitor_window.measured_finish_time = aux_timer;

		// Read number of power consupmtion and traces measurements stored in the BRAMS
		unsigned int number_power_samples = monitor_get_number_power_measurements();
		unsigned int number_traces_samples = monitor_get_number_traces_measurements();
		// Check errors
		// TODO: If the errors surpass a threadhold, invalidate the measurement and reconfigure ADC
		unsigned int number_power_errors = monitor_get_power_errors();

		// Read monitor buffers
		if(monitor_read_power_consumption(number_power_samples + number_power_samples%4) != 0) {
			print_error("[MONITOR] Error reading power\n\r");
			exit(1);
			goto monitor_err;
		}
		if(monitor_read_traces(number_traces_samples + number_traces_samples%4) != 0) {
			print_error("[MONITOR] Error reading traces\n\r");
			exit(1);
			goto monitor_err;
		}

		// Read monitor elapsed time
		unsigned int elapsed_time = monitor_get_time();

		// DEBUG
		clock_gettime(CLOCK_MONOTONIC, &start_online);

		// create power and traces files (file mode)
		#if TRACES_ROM
			// Generate CON file name
			snprintf(power_file_name, 25, "../traces/CON_%d.BIN", monitorization_count);
			// Generate SIG file name
			snprintf(traces_file_name, 25, "../traces/SIG_%d.BIN", monitorization_count);

			fd_power = open(power_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if(fd_power < 0) {
				print_error("[MONITOR] Error! CON file cannot be opened.\n");
				exit(1);
				goto monitor_err;
			}
			fd_traces = open(traces_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if(fd_traces < 0) {
				print_error("[MONITOR] Error! SIG file cannot be opened.\n");
				exit(1);
				goto traces_open_err;
			}
		#endif

		// Write monitor data
		#if TRACES_RAM
			memcpy(&(power_ram_ptr[power_ram_bytes]), power, sizeof(*power) * number_power_samples);
			power_ram_bytes += sizeof(*power) * number_power_samples;
			memcpy(&(power_ram_ptr[power_ram_bytes]), &elapsed_time, sizeof(elapsed_time));
			power_ram_bytes += sizeof(elapsed_time);
			memcpy(&(traces_ram_ptr[traces_ram_bytes]), traces, sizeof(*traces) * number_traces_samples);
			traces_ram_bytes += sizeof(*traces) * number_traces_samples;
		#endif
		#if TRACES_ROM
			write(fd_power, power, sizeof(*power) * (number_power_samples));
			write(fd_power, &elapsed_time, sizeof(elapsed_time));   // para guardar el tiempo
			write(fd_traces, traces, sizeof(*traces) * number_traces_samples);
		#endif
		#if TRACES_SOCKET
			memcpy(&(power_cloud_ptr[power_cloud_bytes]), power, sizeof(*power) * number_power_samples);
			power_cloud_bytes += sizeof(*power) * number_power_samples;
			memcpy(&(power_cloud_ptr[power_cloud_bytes]), &elapsed_time, sizeof(elapsed_time));
			power_cloud_bytes += sizeof(elapsed_time);
			memcpy(&(traces_cloud_ptr[traces_cloud_bytes]), traces, sizeof(*traces) * number_traces_samples);
			traces_cloud_bytes += sizeof(*traces) * number_traces_samples;

			send_buffer_socket_tcp_inet(cloud_socket_id, power_cloud_ptr, power_cloud_bytes);
			send_buffer_socket_tcp_inet(cloud_socket_id, traces_cloud_ptr, traces_cloud_bytes);
		#endif

		print_info("\n[MONITOR] Monitoring Window #%d Successful\n",monitorization_count);
		print_info("[MONITOR] %*s %*u\n", -26, "Number of power samples:", 10, number_power_samples);
		print_info("[MONITOR] %*s %*u\n", -26, "Number of traces samples:", 10, number_traces_samples);
		print_info("[MONITOR] %*s %*u\n", -26, "Elapsed time (cycles):", 10, elapsed_time);

		// Mark the power and traces file sizes (ram mode) / Close the files (file mode)
		#if TRACES_RAM
			// Add the number of written bytes to the end of each ping-pong buffer
			// En el online el truqui se hace en su función de procesado
			// Truqui para escribir el tamaño al final del fichero
			((int *)power_ram_ptr)[POWER_FILE_SIZE / 4 - 1] = power_ram_bytes;
			// Truqui para escribir el tamaño al final del fichero
			((int *)traces_ram_ptr)[TRACES_FILE_SIZE / 4 - 1] = traces_ram_bytes;
		#endif
		#if TRACES_ROM
			close(fd_power);
			traces_open_err:
			close(fd_traces);
		#endif

		monitor_err:

		// Clean monitor brams and counters
		monitor_clean();

		// Check errors
		// TODO: If the errors surpass a threadhold, invalidate the measurement and reconfigure ADC
		if(number_power_errors >= number_power_samples){
			print_error("There have been %u power errors when trying to read %u samples\n", number_power_errors, number_power_samples);
			// Reconfigure ADC just in case
			monitor_config_2vref();
		}

		// Enqueue monitor data
		enqueue_monitor(&monitor_info_queue, &monitor_window);

		// Online processing
		online_processing(user_cpu, kernel_cpu, idle_cpu, &monitor_window, monitorization_count, online_ram_ptr, cloud_socket_id);

		// Toggle shared memory ping pong buffers for next monitoring window (ram mode) (no ping pong in file mode)
		#if TRACES_RAM
			#if MONITORING_MEASUREMENTS_PER_TRAINING == 1
				// Toggle ping-pong buffers
				ret = ping_pong_buffers_toggle(&power_ram_ptr, &traces_ram_ptr, &online_ram_ptr);
			#elif MONITORING_MEASUREMENTS_PER_TRAINING > 1
				ret = execution_modes_buffers_toggle(&power_ram_ptr, &traces_ram_ptr, &online_ram_ptr);
			#else
				#error MONITORING_MEASUREMENTS_PER_TRAINING value must be bigger than 0
			#endif
		#endif

		// Increment monitorization_count
		monitorization_count++;

		// Check if measuring mode has finished
		if (monitorization_count % monitor_args.measurements_per_training == 0) {

			// Variable storing observations to wait commanded from the Python
			int obs_to_wait = 0;

			// Indicate we go to TRAIN mode
			operating_mode = TRAIN;
			printf("[EXECUTION] -> [TRAIN]\n");

			#if ONLINE_MODELS
				// Signal the python code that there is new online data to train/test the models on
				// TODO: handle errors

				// TODO: Remove. Get end time
				clock_gettime(CLOCK_MONOTONIC, &t_start_online_operation);

				ret = online_models_operation(&online_models, monitor_args.measurements_per_training, &obs_to_wait);

				clock_gettime(CLOCK_MONOTONIC, &t_end_online_operation);
				t_online_operation_elapsed_time = diff_timespec(t_start_online_operation, t_end_online_operation);
				t_python =  add_timespec(t_python, t_online_operation_elapsed_time);

			#endif

			// Indicate back to execution
			operating_mode = EXECUTION;
			printf("[TRAIN] -> [EXECUTION]\n");

			// Wait or not based on the return of the models from the python side
			if (obs_to_wait > 0) {

				// Update timer to wait the idle phase
				// TODO: Generate a define with the time to wait
				float idle_windows_to_obs_factor = 1.72;  // aprox 1.72 obs per windows
				float windows_to_wait = obs_to_wait / idle_windows_to_obs_factor;
				long time_to_wait_ms = (long) (windows_to_wait * monitor_args.period_ms);

				printf("[Monitor] Obs to wait: %d, Windows to wait: %f, time to wait (ms): %ld\n", obs_to_wait, windows_to_wait, time_to_wait_ms);

				// Update schedule timer with actual time
				clock_gettime(CLOCK_MONOTONIC, &schedule_timer);

				// Increase timer with time_to_wait
				update_timer_ms(&schedule_timer, time_to_wait_ms);

				// TODO: Remove. Used for ensuring idle time is properly waited
				struct timespec t_aux_0;
				clock_gettime(CLOCK_MONOTONIC, &t_aux_0);
				printf("[Monitor] Idle start time: %ld:%09ld\n", t_aux_0.tv_sec, t_aux_0.tv_nsec);

				// Indicate back to execution
				if(pthread_mutex_lock(&kernel_service_lock) < 0) {
					print_error("Monitor - kernel_service_lock\n");
					exit(1);
				}
				if(pthread_cond_signal(&kernel_service_condition) < 0) {
					print_error("Monitor - kernel_service_signal\n");
					exit(1);
				}
				if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
					print_error("Monitor - kernel_service_unlock\n");
					exit(1);
				}

				// Wait until measurement idle time passes
				clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&(schedule_timer), NULL);

				// TODO: Remove. Used for ensuring idle time is properly waited
				struct timespec t_aux_1;
				clock_gettime(CLOCK_MONOTONIC, &t_aux_1);
				printf("[Monitor] Idle end time: %ld:%09ld\n", t_aux_1.tv_sec, t_aux_1.tv_nsec);

				// TODO: Remove. Used for ensuring idle time is properly waited
				struct timespec elapsed_online_aux = diff_timespec(t_aux_0, t_aux_1);
				printf("[Monitor] Idle elapsed time: %ld:%09ld\n", elapsed_online_aux.tv_sec, elapsed_online_aux.tv_nsec);
			}
			else {
				// Indicate back to execution
				if(pthread_mutex_lock(&kernel_service_lock) < 0) {
					print_error("Monitor - kernel_service_lock\n");
					exit(1);
				}
				if(pthread_cond_signal(&kernel_service_condition) < 0) {
					print_error("Monitor - kernel_service_signal\n");
					exit(1);
				}
				if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
					print_error("Monitor - kernel_service_unlock\n");
					exit(1);
				}
			}

			printf("[MONITOR] Back to take measurements\n");
			// Update schedule timer with actual time
			clock_gettime(CLOCK_MONOTONIC, &schedule_timer);
		}

		// DEBUG
		number_iterations++;
		clock_gettime(CLOCK_MONOTONIC, &end_online);
		elapsed_online = diff_timespec(start_online, end_online);
		total_online_time = add_timespec(total_online_time, elapsed_online);
		// printf("[ONLINE TIME] %ld:%09ld\n", elapsed_online.tv_sec, elapsed_online.tv_nsec);

		// Update timer
		update_timer_ms(&schedule_timer, monitor_args.period_ms);

	}

	// Remove the ping-pong buffers (ram mode)
	#if TRACES_RAM
		#if MONITORING_MEASUREMENTS_PER_TRAINING == 1
			// We pass a 0, because we want to keep the files that back the buffers
			// They will be cleaned-up at the python program, which creates them
			// in the first place
			ret = ping_pong_buffers_clean(0);
		#elif MONITORING_MEASUREMENTS_PER_TRAINING > 1
			ret = execution_modes_buffers_clean(0);
		#else
			#error MONITORING_MEASUREMENTS_PER_TRAINING value must be bigger than 0
		#endif
	#endif

	// Free monitor data buffers
	monitor_free("power");
	monitor_free("traces");

	#if TRACES_ROM
		// Write monitor info
		monitor_info_fd = open(monitor_info_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if(monitor_info_fd < 0) {
			print_error("[MONITOR] Error when opening the file %s\n",monitor_info_file_name);
			exit(1);
		}


		// Print monitor info
		for(i = 0; i < monitorization_count; i++) {
			if(dequeue_monitor(&monitor_info_queue, &monitor_window) < 0) {
				print_error("[MONITOR] Error getting monitorization #%d from the monintor info queue\n",i);
				exit(1);
			}

			//print_monitor_info(&monitor_window);

			// Write to the file
			monitor_info_num_bytes += write(monitor_info_fd, &monitor_window, sizeof(monitor_window));
			if(monitor_info_num_bytes < 0) {
				print_error("[MONITOR] Error when writting to file %s\n", monitor_info_file_name);
				exit(1);
			}
		}

		// Close the file
		close(monitor_info_fd);
	#endif

	// Print usefull information
	// print_info("\n[MONITOR] Monitor info -> num_bytes = %d\n\n",online_ram_num_bytes);

	// DEBUG
	interval_online_time = divide_timespec(total_online_time, number_iterations);
	printf("[ONLINE] Iterations: %d | Total online time: %ld:%09ld | Average online time: %ld:%09ld\n", number_iterations, total_online_time.tv_sec, total_online_time.tv_nsec, interval_online_time.tv_sec, interval_online_time.tv_nsec);

	// Clean queue
	clean_queue_monitor(&monitor_info_queue);

	// Clean socket
	close_socket_tcp(cloud_socket_id);

	#endif

	// Exit thread without return argument
	pthread_exit(NULL);

}

#if CPU_USAGE
/**
 * @brief Function executed by the cpu usage monitor thread.
 * 		  	- Periodically calculates the usage of the CPU by means of the
 * 			  user, kernel and idle time within a time window
 *
 * @param arg Pointer to the arguments passed to this function
 */
void* CPU_usage_monitor_thread(void *arg){

	struct timespec event_timer;
	//struct timespec t_aux;
	int period_in_ms = *((int*) arg);

	// Start event timer
	clock_gettime(CLOCK_MONOTONIC, &event_timer);

	// Keep gathering CPU usage data until the monitorizatio process finishes
	while(monitorization_stop_flag == 0){

		// Wait for an event
	    clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&(event_timer), NULL);

		// Get actual CPU info
		cpu_usage_parse(current_cpu_usage);

		// Compute CPU usage and update CPU indo
		calculate_and_update_cpu_usage(current_cpu_usage, previous_cpu_usage, calculated_cpu_usage);

		// Update timer
		update_timer_ms(&event_timer, period_in_ms);

	}

	// Exit thread without return argument
	pthread_exit(NULL);
}
#endif

/**
 * @brief Main application.
 *
 * @return (int) 0 on sucess, error code otherwise
 */
int main(int argc, char* argv[]) {

	int i, ret;

	// Timers
	struct timespec initial_time, schedule_timer;

	// Initial conditions valiables
	float *inter_arrival_buffer;
	int *kernel_label_buffer;
	int *num_executions_buffer;

	// Queue manager thread id
	pthread_t queue_manager_thread_id;

	// Kernel generation queue
	// TODO:will be remove in the real scenario
	queue kernel_generation_queue;

	kernel_data aux_kernel_data;

	// Set Initial Operating Mode
	operating_mode = EXECUTION;
	/*************************** Print Setup Parameters **********************************/
	char str1[]  = "Board (PYNQ:1|ZCU:2)";
	char str2[]  = "Monitorization (1/0)";
	char str3[]  = "Online Modeling (1/0)";
	char str4[]  = "Execution Modes (1/0)";
	char str5[]  = "Traces on RAM (1/0)";
	char str6[]  = "Traces on ROM (1/0)";
	char str7[]  = "Traces over Socket (1/0)";
	char str8[]  = "Types of Kernels";
	char str9[]  = "Number of Slots";
	char str10[] = "Amount of Kernels to Execute";
	char str11[] = "Monitoring Period in ms";
	char str12[] = "Monitoring Power Samples";
	char str13[] = "Monitoring Traces Samples";
	char str14[] = "Amount of Monitoring Windows";
	char str15[] = "Executions before Training";
	char str16[]  = "CPU Usage (1/0)";
	char str17[] = "CPU Usage Period in ms";

	int title_padding = (sizeof(str10)/sizeof(char)) - 1;
	int parameter_padding = 8;

	printf("\n");
	for(i = 0; i < (title_padding + parameter_padding + 5); i++) {
		printf("-");
	}
	printf("\n");
	printf(" %*s : %*d\n",   -title_padding, str1,  parameter_padding, BOARD);
	printf(" %*s : %*d\n",   -title_padding, str2,  parameter_padding, MONITOR);
	printf(" %*s : %*d\n",   -title_padding, str3,  parameter_padding, ONLINE_MODELS);
	printf(" %*s : %*d\n",   -title_padding, str5,  parameter_padding, TRACES_RAM);
	printf(" %*s : %*d\n",   -title_padding, str6,  parameter_padding, TRACES_ROM);
	printf(" %*s : %*d\n\n", -title_padding, str7,  parameter_padding, TRACES_SOCKET);
	printf(" %*s : %*d\n",   -title_padding, str8,  parameter_padding, TYPES_OF_KERNELS);
	printf(" %*s : %*d\n",   -title_padding, str9,  parameter_padding, NUM_SLOTS);
	printf(" %*s : %*d\n\n", -title_padding, str10, parameter_padding, NUM_KERNELS);
	#if MONITOR
	printf(" %*s : %*d\n",   -title_padding, str11, parameter_padding, MONITORING_PERIOD_MS);
	printf(" %*s : %*d\n",   -title_padding, str12, parameter_padding, MONITOR_POWER_SAMPLES);
	printf(" %*s : %*d\n",   -title_padding, str13, parameter_padding, MONITOR_TRACES_SAMPLES);
	printf(" %*s : %*d\n",   -title_padding, str14, parameter_padding, MONITORING_WINDOWS_NUMBER);
	printf(" %*s : %*d (%.1fs)\n",   -title_padding, str15, parameter_padding, MONITORING_MEASUREMENTS_PER_TRAINING, MONITORING_MEASUREMENTS_PER_TRAINING * MONITORING_PERIOD_MS / 1000.0F);
	printf(" %*s : %*d\n",   -title_padding, str16,  parameter_padding, CPU_USAGE);
	printf(" %*s : %*d\n",   -title_padding, str17, parameter_padding, CPU_USAGE_MONITOR_PERIOD_MS);
	#endif
	for(i = 0; i < (title_padding + parameter_padding + 5); i++) {
		printf("-");
	}
	printf("\n\n");

	// Check if the user have enter any option via argument
	// TODO: Test
	int num_workloads = 0;
	if (argc == 1) {
		printf("Number of setup iterations not indicated\n");
		return 0;
	}
	else if (argc > 1) {
		printf("Execution option: %s\n", argv[1]);
		if (strcmp(argv[1], "info") == 0)
			return 0;
		else{
			printf("argv : %s\n", argv[1]);
			num_workloads = atoi(argv[1]);
		}

		printf("The argument passed is not an available option {info}\n");
		printf("The program continues its execution normally.\n");
	}
	// TODO: Test
	printf("Number of workloads: %d\n", num_workloads);

	/*************************************************************************************/

	// Initialize ARTICo3 slots tracking variable
	for(i = 0; i < NUM_SLOTS; i++)
		slots_in_use[i] = 0;

	// Initialize the thread pool
    print_debug("Create thread pool\n");
    tpool = create_thread_pool(NUM_SLOTS + 1);
	if(tpool == NULL) {
		print_error("Thread pool creation\n");
		exit(1);
	}

	// Initialize the kernels input and reference data
	kernel_init_data();

	// Initialize the queues
	init_queue(&kernel_generation_queue);
	init_queue(&kernel_execution_queue);
	init_queue(&kernel_output_queue);

	// Just to simulate execution time (TODO: remove when artico available)
	srand(42);

	// Initialize ARTICo3
	artico_setup();

	// Initialize Monitor (set Vret to Vref)
	#if BOARD == ZCU
		monitor_setup(0);
	#elif BOARD == PYNQ
		monitor_setup(1);
	#endif

	#if MONITOR
	// Initialize the Online processing logic
	online_setup();
	#endif

	// Create the thread that launches the execution of each kernel
	ret = pthread_create(&queue_manager_thread_id, NULL, &queue_manager_thread, &num_workloads);
	if(ret != 0) {
		print_error("Error creating the execution thread\n");
		exit(1);
	}

	// Get actual time, used for constructing the series of events
	clock_gettime(CLOCK_MONOTONIC, &initial_time);
	// Increment the initial time to get enough time to do some preprocessing
	initial_time.tv_sec += 1;
	schedule_timer = initial_time;

	#if MONITOR

		// Monitor thread variables
		monitor_arguments monitor_args;
		pthread_t monitor_thread_id;

		// Store initial time
		monitor_args.initial_time = initial_time;
		// Set Monitor period in ms
		monitor_args.period_ms = MONITORING_PERIOD_MS;
		// Set number of monitorizations to perform
		monitor_args.num_monitorizations = MONITORING_WINDOWS_NUMBER;
		// Set number of mesurements per training interval
		monitor_args.measurements_per_training = MONITORING_MEASUREMENTS_PER_TRAINING;
		// Set stop_flag to 0
		monitorization_stop_flag = 0;

		// Create the thread that launches the monitorization
		ret = pthread_create(&monitor_thread_id, NULL, &monitoring_thread, &monitor_args);
		if(ret != 0) {
			print_error("Error creating the monitoring thread\n");
			exit(1);
		}
		print_debug("Monitoring thread created\n");

		// CPU Usage
		#if CPU_USAGE
			pthread_t cpu_usage_pthread_id;
			int cpu_usage_monitor_period_ms = CPU_USAGE_MONITOR_PERIOD_MS;
			// Create the thread that calculates the CPU usage
			ret = pthread_create(&cpu_usage_pthread_id, NULL, &CPU_usage_monitor_thread, &cpu_usage_monitor_period_ms);
			if(ret != 0){
				print_error("Error creating the cpu usage thread\n");
				exit(1);
			}
			print_debug("CPU usage thread created\n");
			//pthread_detach(cpu_usage_pthread_id);
		#endif

	#endif

	// Repite the kernel generation for each workload
	for (int workload_index = 0; workload_index < num_workloads; workload_index++) {

		// TODO: Test
		char init_cond_arrival[60];
		char init_cond_id[60];
		char init_cond_executions[60];

		sprintf(init_cond_arrival, "../synthetic_workload/inter_arrival_%d.bin", workload_index);
		sprintf(init_cond_id, "../synthetic_workload/kernel_id_%d.bin", workload_index);
		sprintf(init_cond_executions, "../synthetic_workload/num_executions_%d.bin", workload_index);

		// Reading initial conditions binary files generated by python script
		// read_binary_file("../synthetic_workload/inter_arrival.bin", (void**) &inter_arrival_buffer);
		// read_binary_file("../synthetic_workload/kernel_id.bin", (void**) &kernel_label_buffer);
		// read_binary_file("../synthetic_workload/num_executions.bin", (void**) &num_executions_buffer);

		read_binary_file(init_cond_arrival, (void**) &inter_arrival_buffer);
		read_binary_file(init_cond_id, (void**) &kernel_label_buffer);
		read_binary_file(init_cond_executions, (void**) &num_executions_buffer);

		#if ONLINE_MODELS
		// TODO: Remove. Signal python there will be a new workload
		if (workload_index != 0) {
			int tmp = -1;
			ret = send_data_to_socket_tcp(online_models.training_socket_fd, &tmp, sizeof(tmp));
			if(ret < 0) {
				print_error("Error TCP training socket send operation\n");
				exit(1);
			}
		}
		#endif

		// Fill kernel data structures queue
		for(i = 0; i < NUM_KERNELS; i++) {

			// Initialize each element of the structure to 0
			memset(&aux_kernel_data, 0, sizeof(aux_kernel_data));
			// Info from binary files
			aux_kernel_data.initial_time = initial_time;
			aux_kernel_data.temp_id = i;
			//aux_kernel_data.kernel_label = 3; // Always use crs kernel
			aux_kernel_data.kernel_label = kernel_label_buffer[i];
			aux_kernel_data.num_executions = num_executions_buffer[i];
			aux_kernel_data.intended_arrival_time_ms = (long int)(inter_arrival_buffer[i]);
			aux_kernel_data.slot_id = 0;

			// Initialize arrival and finish time to MAX, this helps managing kernels not finished in online
			// If a kernel has started but not finish the if on the monitoing thread will see like it finished way after
			// so it will write its info and keep it in the queue
			// If a kernel has not started its execution yet the if on the monitoring thread will check if t0==tf
			aux_kernel_data.measured_arrival_time.tv_sec = LONG_MAX;
			aux_kernel_data.measured_arrival_time.tv_nsec = LONG_MAX;
			aux_kernel_data.measured_finish_time.tv_sec = LONG_MAX;
			aux_kernel_data.measured_finish_time.tv_nsec = LONG_MAX;

			// The model should define this
			// Choose the number of compute units for the kernel (this would be selected by the model, now is hardcoded)
			// Remove when model exist
			// aux_kernel_data.cu = ((int)(rand()%8)) + 1; // Random [1,8]
			#if BOARD == ZCU
			int tmp_cu[4] = {1,2,4,8};          // ZCU
			int rand_value = ((int)(rand()%4)); // ZCU
			#elif BOARD == PYNQ
			int tmp_cu[3] = {1,2,4}; 			// PYNQ
			int rand_value = ((int)(rand()%3)); // PYNQ
			#endif
			// aux_kernel_data.cu = 1;  // FIFO 8 acc/tarea no interaccion
			// aux_kernel_data.cu = 1;  // FIFO 1 acc/tarea multiples tareas en paralelo
			aux_kernel_data.cu = tmp_cu[rand_value];

			// Generate the arrival timer for each kernel
			update_timer_ms(&schedule_timer, aux_kernel_data.intended_arrival_time_ms);
			aux_kernel_data.commanded_arrival_time = schedule_timer;

			// Add the kernel to the kernel generation queue
			if(enqueue(&kernel_generation_queue, &aux_kernel_data) < 0) {
				print_error("Error adding kernel #%d to the kernel generation queue\n",i);
				exit(1);
			}
		}

		for(i = 0; i < NUM_KERNELS; i++) {

			// Dequeue next kernel
			if(dequeue(&kernel_generation_queue, &aux_kernel_data) < 0) {
				print_error("Error getting kernel #%d from the kernel generation queue\n",i);
				exit(1);
			}

			// Wait until the next kernel arrival time
			clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&(aux_kernel_data.commanded_arrival_time), NULL);

			// Add the new kernel to the queue
			print_debug("Main - Pre-add execution queue -> #%d\n", i);
			if(pthread_mutex_lock(&kernel_execution_queue_lock) < 0) {
				print_error("kernel_execution_queue_lock - main\n");
				exit(1);
			}

			ret = enqueue(&kernel_execution_queue, &aux_kernel_data);
			if(ret != 0) {
				print_error("Error creating kernel #%d\n",i);
				exit(1);
			}

			if(pthread_mutex_unlock(&kernel_execution_queue_lock) < 0) {
				print_error("kernel_execution_queue_unlock - main\n");
				exit(1);
			}
			print_debug("Main - Post-add execution queue -> #%d\n", i);

			// Increment kernels_to_serve_variable and set kernels_are_executable_variable and signal condition
			if(pthread_mutex_lock(&kernel_service_lock) < 0) {
				print_error("Main - kernel_service_lock\n");
				exit(1);
			}
			kernels_to_serve_variable++;
			kernels_are_executable_variable = 1;
			// Only signal when there are free slots
			if(free_slots_variable > 0) {
				if(pthread_cond_signal(&kernel_service_condition) < 0) {
					print_error("Main - kernel_service_signal\n");
					exit(1);
				}
			}
			if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
				print_error("Main - kernel_service_unlock\n");
				exit(1);
			}

		}

		// TODO: Wait until all kernels of this workload have been executed
		if(pthread_mutex_lock(&kernel_service_lock) < 0) {
			print_error("Main - kernel_service_lock (wait new workload)\n");
			exit(1);
		}
		while(workload_finished_flag == 0) {

			printf("Main - Waiting workload to finished\n");

			if(pthread_cond_wait(&workload_finished_condition, &kernel_service_lock) < 0) {
				print_error("Main - workload_finished_condition wait\n");
				exit(1);
			}

		}
		// Clean new workload flga
		workload_finished_flag = 0;

		printf("Main - Last workload has finished\n");

		if(pthread_mutex_unlock(&kernel_service_lock) < 0) {
			print_error("Main - kernel_service_unlock (wait new workload)\n");
			exit(1);
		}

	}

	// Clean the kernel generation queue
	clean_queue(&kernel_generation_queue);

	// Wait for the queue manager thread
	pthread_join(queue_manager_thread_id, NULL);

	// TODO: Esto se pone aquí porque puede ocurrir que el hilo del monitor esté en sleep() pero
	//       todas las workloads hayan acabado. Por lo que se informa al python para que no siga esperando
	//       Lo suyo sería enviar una señar para parar el contador
	#if ONLINE_MODELS
		// Clean online models interaction stuff
		online_models_clean(&online_models);
	printf("Main - Se avisa a python de que han acabado todas las workloas. Util solo si justo el monitor estaba en nanosleep()\n");
	////////////////////////////////
	#endif

	#if MONITOR
		// Signal de monitor to stop the monitorization
		monitorization_stop_flag = 1;
		pthread_join(monitor_thread_id, NULL);
		#if CPU_USAGE
			pthread_join(cpu_usage_pthread_id, NULL);
		#endif
	#endif

	// Free dynamically allocated variables
	free(inter_arrival_buffer);
	free(kernel_label_buffer);
	free(num_executions_buffer);

	// Clean the kernels input and reference data
	kernel_clean_data();

	#if MONITOR
	// Clean the Online processing logic
	online_clean();
	#endif

	// Clean Monitor
	monitor_cleanup();

	// Clean ARTICo3
	artico_cleanup();

	// Destroy the thread pool
	print_debug("Destroy pool\n");
    destroy_threadpool(tpool);

	// Save the kernels information thoughout the whole application
	save_output("../outputs/kernels_info.bin", &kernel_output_queue);

	return 0;
}
