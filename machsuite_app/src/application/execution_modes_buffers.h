/**
 * @file execution_modes_buffers.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions that handle the power, traces and 
 *        online data buffers used for online traces processing on-ram, when 
 *        working with different execution modes (training/executing).
 * @date July 2023
 * 
 */

#ifndef _EXECUTION_MODES_BUFFERS_H_
#define _EXECUTION_MODES_BUFFERS_H_


/**
 * @brief Initialize the execution modes buffers and return them by reference
 * 
 * @param measurements_per_training Number of traces to be stores in a training stage
 * @param power_ptr Address to copy power buffer to
 * @param traces_ptr Address to copy the traces buffer to
 * @param online_ptr Address to copy the online buffer to
 * @return (int) 0 on success, error code otherwise
 */
int execution_modes_buffers_init(const int measurements_per_training, char **power_ptr, char **traces_ptr, char **online_ptr);

/**
 * @brief Clean the execution modes buffers
 * 
 * @param remove_buffers Place a 1 to remove the files that back the execution
 *                       modes buffers. With a 0 the files will remain on the fs
 * @return (int) 0 on success, error code otherwise
 */
int execution_modes_buffers_clean(const int remove_buffers);

/**
 * @brief Moves the pointer of the current buffer to the address that has to be
 *        written in the next iteration of the execution stage (by reference)
 * 
 * @param power_ptr Address to copy the ping power buffer to
 * @param traces_ptr Address to copy the ping traces buffer to
 * @param online_ptr Address to copy the ping online buffer to
 * @return int 
 */
int execution_modes_buffers_toggle(char **power_ptr, char **traces_ptr, char **online_ptr);


#endif /*_EXECUTION_MODES_BUFFERS_H_*/