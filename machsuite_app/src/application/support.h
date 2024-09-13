/**
 * @file support.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains a collection of functions for different purposes requiered in the main application
 * @date February 2023
 *
 */

#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include <sys/socket.h>
#include <sys/un.h>

#include "data_structures.h"
#include "queue_kernel.h"
#include "queue_online.h"

// Nanosecond in a second
#define NS_PER_SECOND 1000000000L

/********************** Data Management functions ************************/

/**
 * @brief Read a binary file
 *
 * @param file_name Path of the file
 * @param buffer Pointer to a buffer (the buffer must be an array)
 */
void read_binary_file(char* file_name, void **buffer);

/**
 * @brief Write kernel information to a file in binary
 *
 * @param file_name Path of the file
 * @param buffer Pointer to the kernel data
 */
/*
void write_binary_file(char* file_name, void *buffer) {

	FILE *fd;
	int num_write;

	// Open file in binary mode
	fd = fopen(file_name,"wb");
	if(fd == NULL) {
		print_error("Error when opening the file %s\n",file_name);
		exit(1);
	}
	// Write the file
	num_write = fwrite(buffer, sizeof(kernel_data), NUM_KERNELS, fd);
	if(num_write < 0) {
		print_error("Error when writting to file %s\n", file_name);
		exit(1);
	}

	// Close the file
	fclose(fd);

	// Print usefull information
	print_info("\nnum_write = %d\n\n",num_write);

}*/

/**
 * @brief Save the historical data about the kernels executed in the application to a file
 *
 * @param file_name Path of the file
 * @param p Pointer to the queue of kernels to be saved
 */
void save_output(char* file_name, void *p);

/**
 * @brief Print monitoring window information
 *
 * @param data Pointer to the monitoring information to be printed
 */
void print_monitor_info(const monitor_data *data);

/*************************** Timer functions *****************************/

/**
 * @brief Calculate if \p l_val timespec is greater than \p r_val timespec
 *
 * @param l_val Timespec structure (l_value)
 * @param r_val Timespec structure (r_value)
 * @return (int) 1 if l_value is greater than r_value, 0 otherwise
 */
int greater_than_timespec(const struct timespec l_val, const struct timespec r_val);

/**
 * @brief Calculate if \p l_val timespec is less than \p r_val timespec
 *
 * @param l_val Timespec structure (l_value)
 * @param r_val Timespec structure (r_value)
 * @return (int) 1 if l_value is less than r_value, 0 otherwise
 */
int less_than_timespec(const struct timespec l_val, const struct timespec r_val);

/**
 * @brief Calculate if \p l_val timespec is equal to \p r_val timespec
 *
 * @param l_val Timespec structure (l_value)
 * @param r_val Timespec structure (r_value)
 * @return (int) 1 if l_value is equal to r_value, 0 otherwise
 */
int equal_to_timespec(const struct timespec l_val, const struct timespec r_val);

/**
 * @brief Calculate the elapsed time between two timespecs
 *
 * @param start Initial timespec structure
 * @param end Final timespec structure
 * @return (struct timespec) Elapsed time as a timespec structure
 */
struct timespec diff_timespec(struct timespec start, struct timespec end);

/**
 * @brief Add two timespecs
 *
 * @param a First timespec structure
 * @param b Second timespec structure
 * @return (struct timespec) The addition of the two timespec structure
 */
struct timespec add_timespec(struct timespec a, struct timespec b);

/**
 * @brief Timespec division by a number
 *
 * @param divident Timespec structure to be divided
 * @param divisor Number to divide the divident into
 * @return (struct timespec) \p divident divided by \p divisor
 */
struct timespec divide_timespec(struct timespec divident, int divisor);

/**
 * @brief Add a nanoseconds to a timespec structure
 *
 * @param time Pointer to the timespec structure to be updated
 * @param nsec Number of nanoseconds to add to \p time
 */
void update_timer(struct timespec *time, long int nsec);


/************************** ARTICo3 Functions ****************************/

/**
 * @brief Initialize ARTICo3 and create each kernel
 *
 */
void artico_setup(void);

/**
 * @brief Clean ARTICo3 and release each kernel
 *
 */
void artico_cleanup(void);

/************************** Monitor Functions ****************************/

/**
 * @brief Initialize the monitor
 *
 * @param doble_reference_voltage Flag indicating whether the voltage references is doble (5v) or not (2.5v)
 */
void monitor_setup(const int doble_reference_voltage);

/**
 * @brief Clean the monitoring infrastructure
 *
 */
void monitor_cleanup(void);

#endif /*_SUPPORT_H_*/