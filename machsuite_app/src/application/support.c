/**
 * @file support.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains a collection of functions for different purposes requiered in the main application
 * @date February 2023
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "support.h"

#include "debug.h"
#include "queue_kernel.h"
#include "queue_online.h"
#include "client_socket_udp.h"

#if ARTICO
	#include "artico3.h"
#endif
#if MONITOR
	#if MDC == 0
		#include "monitor.h"
	#endif
#endif


/********************** Data Management functions ************************/

/**
 * @brief Read a binary file
 *
 * @param file_name Path of the file
 * @param buffer Pointer to a buffer (the buffer must be an array)
 */
void read_binary_file(char* file_name, void **buffer) {

	FILE *fd;
	long len_file;
	int ret, num_read;

	// Open file in binary mode
	fd = fopen(file_name,"rb");
	if(fd == NULL) {
		print_error("Error when opening the file %s\n",file_name);
		exit(1);
	}

	// Move the cursor to the end of the file
	ret = fseek(fd, 0, SEEK_END);
	if(ret < 0) {
		print_error("Error when lseek to end the file %s\n",file_name);
		exit(1);
	}

	// Get the current position (the length of the file)
	len_file = ftell(fd);
	if(len_file < 0) {
		print_error("Error when ftell the file %s\n",file_name);
		exit(1);
	}

	// Go back to the start
	ret = fseek(fd, 0, SEEK_SET);
	if(ret < 0) {
		print_error("Error when lseek to beginning the file %s\n",file_name);
		exit(1);
	}

	// Dynamically allocate a buffer for storing the file data
	*buffer = (void*) malloc(len_file);
	if(*buffer == NULL) {
		print_error("Error when doing a malloc\n");
		exit(1);
	}

	// Read the file
	num_read = fread(*buffer, sizeof(unsigned char), len_file, fd);
	if(num_read < 0) {
		print_error("Error when reading from file %s\n", file_name);
		exit(1);
	}

	// Close the file
	fclose(fd);

	// Print usefull information
	print_info("[%s]\tRead -> len_file = %ld, num_read = %d\n", file_name, len_file, num_read);

}

/**
 * @brief Write kernel information to a file in binary
 *
 * @param file_name Path of the file
 * @param buffer Pointer to the kernel data
 */
/*
void write_binary_file(char* file_name, void *buffer) {

	FILE *fd;
	int num_bytes;

	// Open file in binary mode
	fd = fopen(file_name,"wb");
	if(fd == NULL) {
		print_error("Error when opening the file %s\n",file_name);
		exit(1);
	}
	// Write the file
	num_bytes = fwrite(buffer, sizeof(kernel_data), NUM_KERNELS, fd);
	if(num_bytes < 0) {
		print_error("Error when writting to file %s\n", file_name);
		exit(1);
	}

	// Close the file
	fclose(fd);

	// Print usefull information
	print_info("\nnum_bytes = %d\n\n",num_bytes);

}*/

/**
 * @brief Print kernel information (internal function)
 *
 * @param data Pointer to the kernel information to be printed
 */
static void _print_kernel_info(const kernel_data *data) {

	// Print general information
	printf("\nInitial Time: %ld : %ld\n",data->initial_time.tv_sec,data->initial_time.tv_nsec);
	printf("Temp ID: %d\n",data->temp_id);
	printf("kernel label: %d\n",data->kernel_label);
	printf("Number of Executions: %d\n",data->num_executions);
	printf("Number of Compute Units: %d\n",data->cu);
	printf("ARTICo3 Slot used: %X\n",data->slot_id);
	printf("Intended Arrival (ms):  %ld\n",data->intended_arrival_time_ms);
	printf("Commanded Arrival: %ld : %ld\n",data->commanded_arrival_time.tv_sec,data->commanded_arrival_time.tv_nsec);
	printf("Measured Arrival:  %ld : %ld\n",data->measured_arrival_time.tv_sec,data->measured_arrival_time.tv_nsec);
	printf("Measured Finish:   %ld : %ld\n",data->measured_finish_time.tv_sec,data->measured_finish_time.tv_nsec);
	// Print elapsed times
	printf("\nDiff Commanded-Measured: %ld : %ld\n",diff_timespec(data->commanded_arrival_time,data->measured_arrival_time).tv_sec,diff_timespec(data->commanded_arrival_time,data->measured_arrival_time).tv_nsec);
	printf("Diff Start-Stop:         %ld : %ld\n\n",diff_timespec(data->measured_arrival_time,data->measured_finish_time).tv_sec,diff_timespec(data->measured_arrival_time,data->measured_finish_time).tv_nsec);

}

/**
 * @brief Save the historical data about the kernels executed in the application to a file
 *
 * @param file_name Path of the file
 * @param p Pointer to the queue of kernels to be saved
 */
void save_output(char* file_name, void *p) {

	kernel_data kernel_tmp;

	queue *output_queue = (queue *) p;

	FILE *fd;
	int i, num_kernels, num_bytes = 0;

	// Open file in binary mode
	fd = fopen(file_name,"wb");
	if(fd == NULL) {
		print_error("Error when opening the file %s\n",file_name);
		exit(1);
	}

	// Get number of elements to be printed from the queue
	num_kernels = get_size_queue(output_queue);
	print_info("Numero de kernels: %d\n", num_kernels);

	// Print kernels info
	for(i = 0; i < num_kernels; i++) {
		if(dequeue(output_queue, &kernel_tmp) < 0) {
			print_error("Error getting kernel #%d from the kernel output queue\n",i);
			exit(1);
		}
		//_print_kernel_info(&kernel_tmp);
		// Write the file
		// Salta un error en valgrind, vamos a copiarlo por campos
		num_bytes += fwrite(&kernel_tmp, sizeof(kernel_tmp), 1, fd);

		if(num_bytes < 0) {
			print_error("Error when writting to file %s\n", file_name);
			exit(1);
		}
	}
	// Close the file
	fclose(fd);

	// Clean the kernel output queue
	clean_queue(output_queue);

	// Print usefull information
	print_info("\nnum_bytes = %d\n\n",num_bytes);
}

/**
 * @brief Print monitoring window information
 *
 * @param data Pointer to the monitoring information to be printed
 */
void print_monitor_info(const monitor_data *data) {

	// Print general information
	print_info("Measured Start:  %ld : %ld\n",data->measured_starting_time.tv_sec,data->measured_starting_time.tv_nsec);
	print_info("Measured Finish:   %ld : %ld\n",data->measured_finish_time.tv_sec,data->measured_finish_time.tv_nsec);

}

/*************************** Timer functions *****************************/

/**
 * @brief Calculate if \p l_val timespec is greater than \p r_val timespec
 *
 * @param l_val Timespec structure (l_value)
 * @param r_val Timespec structure (r_value)
 * @return (int) 1 if l_value is greater than r_value, 0 otherwise
 */
int greater_than_timespec(const struct timespec l_val, const struct timespec r_val) {
	if(l_val.tv_sec == r_val.tv_sec) {
		return l_val.tv_nsec > r_val.tv_nsec;
	}
	else{
		return l_val.tv_sec > r_val.tv_sec;
	}
}

/**
 * @brief Calculate if \p l_val timespec is less than \p r_val timespec
 *
 * @param l_val Timespec structure (l_value)
 * @param r_val Timespec structure (r_value)
 * @return (int) 1 if l_value is less than r_value, 0 otherwise
 */
int less_than_timespec(const struct timespec l_val, const struct timespec r_val) {
	if(l_val.tv_sec == r_val.tv_sec) {
		return l_val.tv_nsec < r_val.tv_nsec;
	}
	else{
		return l_val.tv_sec < r_val.tv_sec;
	}
}

/**
 * @brief Calculate if \p l_val timespec is equal to \p r_val timespec
 *
 * @param l_val Timespec structure (l_value)
 * @param r_val Timespec structure (r_value)
 * @return (int) 1 if l_value is equal to r_value, 0 otherwise
 */
int equal_to_timespec(const struct timespec l_val, const struct timespec r_val) {
	if((l_val.tv_sec == r_val.tv_sec) && (l_val.tv_nsec == r_val.tv_nsec)) {
		return 1;
	}
	return 0;
}

/**
 * @brief Calculate the elapsed time between two timespecs
 *
 * @param start Initial timespec structure
 * @param end Final timespec structure
 * @return (struct timespec) Elapsed time as a timespec structure
 */
struct timespec diff_timespec(struct timespec start, struct timespec end) {

    struct timespec tmp;

    if((end.tv_nsec-start.tv_nsec)<0) {
        tmp.tv_sec = end.tv_sec-start.tv_sec-1;
        tmp.tv_nsec = NS_PER_SECOND+end.tv_nsec-start.tv_nsec;
    }
    else {
        tmp.tv_sec = end.tv_sec-start.tv_sec;
        tmp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return tmp;

}

/**
 * @brief Add two timespecs
 *
 * @param a First timespec structure
 * @param b Second timespec structure
 * @return (struct timespec) The addition of the two timespec structure
 */
struct timespec add_timespec(struct timespec a, struct timespec b) {

    struct timespec tmp;

	tmp.tv_sec = a.tv_sec + b.tv_sec;
	tmp.tv_nsec = a.tv_nsec + b.tv_nsec;

    if(tmp.tv_nsec > NS_PER_SECOND) {
        tmp.tv_sec++;
        tmp.tv_nsec -= NS_PER_SECOND;
    }

    return tmp;

}

/**
 * @brief Timespec division by a number
 *
 *
 * @param divident Timespec structure to be divided
 * @param divisor Number to divide the divident into
 * @return (struct timespec) \p divident divided by \p divisor
 */
struct timespec divide_timespec(struct timespec divident, int divisor) {

    struct timespec tmp;
	long int nsec;

	nsec = (divident.tv_sec * NS_PER_SECOND + divident.tv_nsec) / divisor;

	tmp.tv_sec = nsec / NS_PER_SECOND;
	tmp.tv_nsec = nsec % NS_PER_SECOND;

    return tmp;

}

/**
 * @brief Add miliseconds to a timespec structure
 *
 * @param time Pointer to the timespec structure to be updated
 * @param msec Number of miliseconds to add to \p time
 */
void update_timer_ms(struct timespec *time, long int msec) {
    // Split ms into seconds and remaining milliseconds
    time->tv_sec += msec / 1000;         // Add full seconds from milliseconds
    long int remaining_ms = msec % 1000; // Remaining milliseconds

    // Convert remaining milliseconds to nanoseconds
    long int nsec = remaining_ms * 1000000L;

    // Add the nanoseconds to the timespec
    time->tv_nsec += nsec;

    // Handle overflow (more than a second)
    if (time->tv_nsec >= NS_PER_SECOND) {
        time->tv_sec += time->tv_nsec / NS_PER_SECOND;
        time->tv_nsec = time->tv_nsec % NS_PER_SECOND;
    }
    // Handle underflow (negative nanoseconds)
    else if (time->tv_nsec < 0) {
		print_error("Error: Negative nanoseconds on update_times_ms\n");
        // time->tv_sec += (time->tv_nsec / NS_PER_SECOND) - 1;
        // time->tv_nsec = NS_PER_SECOND + (time->tv_nsec % NS_PER_SECOND);
    }
}

/**
 * @brief Calculate percentage of t1 relative to t2
 *
 * @param t1 Small time
 * @param t2 Big time (to which the calculation is relative to)
 * @return The percentage of \p t2 represented by \p t1
 */
double calculate_percentage(struct timespec t1, struct timespec t2) {
    // Convert t1 and t2 times to nanoseconds as double values to prevent overflow (32-bit systems)
    double total_nsecs_t1 = (double)t1.tv_sec * 1e9 + (double)t1.tv_nsec;
    double total_nsecs_t2 = (double)t2.tv_sec * 1e9 + (double)t2.tv_nsec;

	//todo: que no falle cuando long no es de 64bits (en la pynq)

    // Calculate the percentage
    if (total_nsecs_t2 == 0) {
        return 0.0;  // Avoid division by zero
    }
    double percentage = (total_nsecs_t1 / total_nsecs_t2) * 100.0;

    return percentage;
}

/************************** ARTICo3 Functions ****************************/

/**
 * @brief Initialize ARTICo3 and create each kernel
 *
 */
void artico_setup(void) {

	print_debug("\nARTICo3 Setup...\n");

	#if ARTICO

	// ARTICo3 Init
	artico3_init();

	// Create kernels (everyone)
	artico3_kernel_create("aes", 640, 5, 0);
	artico3_kernel_create("bulk", 32768, 2, 0);
	artico3_kernel_create("crs", 33320, 5, 0);
	artico3_kernel_create("kmp", 65536, 2, 0);
	artico3_kernel_create("knn", 32768, 2, 0);
	artico3_kernel_create("merge", 8192, 1, 0);
	artico3_kernel_create("nw", 49152, 3, 0);
	artico3_kernel_create("queue", 32768, 2, 0);
	artico3_kernel_create("stencil2d", 49152, 3, 0);
	artico3_kernel_create("stencil3d", 49152, 3, 0);
	artico3_kernel_create("strided", 16384, 4, 0);

	#else

	// Needed to configure the main .bit which contains monitor HW
	// ARTICo3 Init
	// artico3_init();

	#endif

}

/**
 * @brief Clean ARTICo3 and release each kernel
 *
 */
void artico_cleanup(void) {

	print_debug("\nCleaning ARTICo3...\n");

	#if ARTICO

	// Release all the kernels
	artico3_kernel_release("aes");
	artico3_kernel_release("bulk");
	artico3_kernel_release("crs");
	artico3_kernel_release("kmp");
	artico3_kernel_release("knn");
	artico3_kernel_release("merge");
	artico3_kernel_release("nw");
	artico3_kernel_release("queue");
	artico3_kernel_release("stencil2d");
	artico3_kernel_release("stencil3d");
	artico3_kernel_release("strided");

	// ARTICo3 exit
	artico3_exit();

	#else

	// Needed to remove the HW
	// ARTICo3 exit
	// artico3_exit();

	#endif
}

/************************** Monitor Functions ****************************/

/**
 * @brief Initialize the monitor
 *
 * @param doble_reference_voltage Flag indicating whether the voltage references is doble (5v) or not (2.5v)
 */
void monitor_setup(const int doble_reference_voltage) {

	print_debug("\nMonitor Setup...\n");

	#if MONITOR && MDC == 0

    // Load Monitor overlay and driver
	system("./setup_monitor/setup_monitor.sh");

	// Monitor Init
	monitor_init();

	// Clean monitor hw
	monitor_stop();
	monitor_clean();

	// Config Vref
	if(doble_reference_voltage == 1)
		monitor_config_2vref();
	else
		monitor_config_vref();

	#endif
}

/**
 * @brief Clean the monitoring infrastructure
 *
 */
void monitor_cleanup(void) {

	print_debug("\nCleaning Monitor...\n");

	#if MONITOR && MDC == 0

	// Clean monitor hw
	monitor_stop();
	monitor_clean();

	// Clean monitor setup
    monitor_exit();

    // Load Monitor overlay and driver
	system("./setup_monitor/remove_monitor.sh");

	#endif
}
