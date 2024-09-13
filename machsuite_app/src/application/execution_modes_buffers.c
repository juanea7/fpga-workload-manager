/**
 * @file execution_modes_buffers.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions that handle the power, traces and
 *        online data buffers used for online traces processing on-ram, when
 *        working with different execution modes (training/executing).
 * @date July 2023
 *
 */

#include "execution_modes_buffers.h"
#include "debug.h"
#include "data_structures.h"

#include <sys/mman.h>
#include <sys/stat.h>       // For mode constants
#include <fcntl.h>          // For O_* constants
#include <unistd.h>         // f_truncate()
#include <stdlib.h>         // exit()

// Execution modes files names
#define POWER_FILE_NAME "power_file"
#define TRACES_FILE_NAME "traces_file"
#define ONLINE_FILE_NAME "online_file"


/**
 * @brief Structure containing each the buffers for the power, traces and
 *        online buffers, as well as the current one in use
 *
 */
typedef struct execution_modes_buffers {
    char *power_base_ptr;       // Power base buffer
    char *power_current_ptr;    // Power current ptr
    char *traces_base_ptr;      // Traces base buffer
    char *traces_current_ptr;   // Traces current ptr
    char *online_base_ptr;      // Online base buffer
    char *online_current_ptr;   // Online current ptr
    int total_iterations;       // Number of traces to store per training stage
    int current_iteration;      // Index of the actual measurement iteration of the training stage
}execution_modes_buffers_t;

// Static global buffers
static execution_modes_buffers_t buffers;

/**
 * @brief Create a memory-mapped ram-backed file
 *
 * @param filename File name
 * @param size Size of file to create and memory-map
 * @return (char*) Pointer to the created buffer
 */
static char * _create_buffer_file(const char *filename, const int size) {

    int fd, ret;
    char *buffer;

    // Open a file
    fd = shm_open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd < 0) {
        print_error("[EXECUTION_MODES_BUFFERS] Error when opening the file %s\n", filename);
        perror("test\n");
        return NULL;
    }

    // Truncate the file
    ret = ftruncate(fd, size);
    if(ret < 0) {
        print_error("[EXECUTION_MODES_BUFFERS] Error truncating the file %s\n", filename);
        return NULL;
    }

    // Memory map the file in SHARED mode, which means that is visible for
    // other processes
    buffer = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(buffer == NULL) {
        print_error("[EXECUTION_MODES_BUFFERS] Error mmap'ing the file %s\n", filename);
        return NULL;
    }

    // Close the file descriptor (not needed anymore)
    ret = close(fd);
    if(ret < 0) {
        print_error("[EXECUTION_MODES_BUFFERS] Error closing the file %s\n", filename);
        return NULL;
    }

    return buffer;

}

/**
 * @brief Close a memory-mapped buffer
 *
 * @param buffer Pointer to the memory-mapped buffer to be closed
 * @param size Size of the memory-mapped region
 * @param filename Name of the file to which the memory-mapped buffer correspond
 *                 Note: if NULL, that file will not be removed from the fs, so
 *                       other processes could access it
 * @return (int) 0 on success, error code otherwise
 */
static int _close_buffer_file(char *buffer, const int size, const char *filename) {

    int ret;

    // Unmap the region
    ret = munmap(buffer, size);
    if(ret < 0) {
        print_error("[EXECUTION_MODES_BUFFERS] Error unmapping the ptr %p\n" ,buffer);
        return -1;
    }

    if(filename != NULL) {
		// Remove the shared memory file from the tmpfs
		ret = shm_unlink(filename);
		if(ret < 0) {
			print_error("[EXECUTION_MODES_BUFFERS] Error unlinking the file %s\n", filename);
			return -1;
		}
    }

    return 0;
}

/**
 * @brief Initialize the execution modes buffers and return them by reference
 *
 * @param measurements_per_training Number of traces to be stores in a training stage
 * @param power_ptr Address to copy power buffer to
 * @param traces_ptr Address to copy the traces buffer to
 * @param online_ptr Address to copy the online buffer to
 * @return (int) 0 on success, error code otherwise
 */
int execution_modes_buffers_init(const int measurements_per_training, char **power_ptr, char **traces_ptr, char **online_ptr) {

    // TODO: Handle errors

    // Save the number of measurements to store per training
    buffers.total_iterations = measurements_per_training;
    // Initialize the actual iteration of the training process
    buffers.current_iteration = 0;

    // Power buffer
    buffers.power_base_ptr = _create_buffer_file(POWER_FILE_NAME, POWER_FILE_SIZE * buffers.total_iterations);
    if(buffers.power_base_ptr == NULL) {
        print_error("[EXECUTION_MODES_BUFFERS] Error creating %s\n", POWER_FILE_NAME);
        exit(1);
    }
    buffers.power_current_ptr = buffers.power_base_ptr;

    // Traces buffer
    buffers.traces_base_ptr = _create_buffer_file(TRACES_FILE_NAME, TRACES_FILE_SIZE * buffers.total_iterations);
    if(buffers.traces_base_ptr == NULL) {
        print_error("[EXECUTION_MODES_BUFFERS] Error creating %s\n", TRACES_FILE_NAME);
        exit(1);
    }
    buffers.traces_current_ptr = buffers.traces_base_ptr;

    // Online buffer
    buffers.online_base_ptr = _create_buffer_file(ONLINE_FILE_NAME, ONLINE_FILE_SIZE * buffers.total_iterations);
    if(buffers.online_base_ptr == NULL) {
        print_error("[EXECUTION_MODES_BUFFERS] Error creating %s\n", ONLINE_FILE_NAME);
        exit(1);
    }
    buffers.online_current_ptr = buffers.online_base_ptr;

    // Actual outside assigment
    *power_ptr = buffers.power_current_ptr;
    *traces_ptr = buffers.traces_current_ptr;
    *online_ptr = buffers.online_current_ptr;

    return 0;
}

/**
 * @brief Clean the execution modes buffers
 *
 * @param remove_buffers Place a 1 to remove the files that back the execution
 *                       modes buffers. With a 0 the files will remain on the fs
 * @return (int) 0 on success, error code otherwise
 */
int execution_modes_buffers_clean(const int remove_buffers) {

    int ret;
    char *tmp_file = NULL;

    // Power file
    if(remove_buffers) tmp_file = POWER_FILE_NAME;
    ret = _close_buffer_file(buffers.power_base_ptr, POWER_FILE_SIZE * buffers.total_iterations, tmp_file);
    if(ret < 0) {
        print_error("[EXECUTION_MODES_BUFFERS] Error cleaning %s\n", POWER_FILE_NAME);
        exit(1);
    }

    // Traces file
    if(remove_buffers) tmp_file = TRACES_FILE_NAME;
    ret = _close_buffer_file(buffers.traces_base_ptr, TRACES_FILE_SIZE * buffers.total_iterations, tmp_file);
    if(ret < 0) {
        print_error("[EXECUTION_MODES_BUFFERS] Error cleaning %s\n", TRACES_FILE_NAME);
        exit(1);
    }

    // Online file
    if(remove_buffers) tmp_file = ONLINE_FILE_NAME;
    ret = _close_buffer_file(buffers.online_base_ptr, ONLINE_FILE_SIZE * buffers.total_iterations, tmp_file);
    if(ret < 0) {
        print_error("[EXECUTION_MODES_BUFFERS] Error cleaning %s\n", ONLINE_FILE_NAME);
        exit(1);
    }

    // Clean variables
    buffers.power_base_ptr = NULL;
    buffers.power_current_ptr = NULL;
    buffers.traces_base_ptr = NULL;
    buffers.traces_current_ptr = NULL;
    buffers.online_base_ptr = NULL;
    buffers.online_current_ptr = NULL;
    buffers.current_iteration = 0;

    return 0;
}

/**
 * @brief Moves the pointer of the current buffer to the address that has to be
 *        written in the next iteration of the execution stage (by reference)
 *
 * @param power_ptr Address to copy the ping power buffer to
 * @param traces_ptr Address to copy the ping traces buffer to
 * @param online_ptr Address to copy the ping online buffer to
 * @return int
 */
int execution_modes_buffers_toggle(char **power_ptr, char **traces_ptr, char **online_ptr) {

    // Increment the current iteration counter until it reaches total iterations
    buffers.current_iteration = (buffers.current_iteration + 1) % buffers.total_iterations;

    // Power buffer
    buffers.power_current_ptr = buffers.power_base_ptr + (POWER_FILE_SIZE * buffers.current_iteration);
    // Traces buffer
    buffers.traces_current_ptr = buffers.traces_base_ptr + (TRACES_FILE_SIZE * buffers.current_iteration);
    // Online buffer
    buffers.online_current_ptr = buffers.online_base_ptr + (ONLINE_FILE_SIZE * buffers.current_iteration);

    // Actual outside assigment
    *power_ptr = buffers.power_current_ptr;
    *traces_ptr = buffers.traces_current_ptr;
    *online_ptr = buffers.online_current_ptr;

    return 0;
}

