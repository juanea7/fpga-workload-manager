/**
 * @file ping_pong_buffers.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions that handle the power, traces and 
 *        online data ping-pongvbuffers used for online traces processing 
 *        on-ram.
 * @date March 2023
 * 
 */

#include "ping_pong_buffers.h"
#include "debug.h"
#include "data_structures.h"

#include <sys/mman.h>
#include <sys/stat.h>       // For mode constants
#include <fcntl.h>          // For O_* constants
#include <unistd.h>         // f_truncate()
#include <stdlib.h>         // exit()

// Ping-pong files names
#define POWER_PING_FILE_NAME "power_ping_file"
#define POWER_PONG_FILE_NAME "power_pong_file"
#define TRACES_PING_FILE_NAME "traces_ping_file"
#define TRACES_PONG_FILE_NAME "traces_pong_file"
#define ONLINE_PING_FILE_NAME "online_ping_file"
#define ONLINE_PONG_FILE_NAME "online_pong_file"


/**
 * @brief Structure containing each ping and pong buffers for the power, traces
 *        and online buffers, as well as the current one in use
 * 
 */
typedef struct _ping_pong_buffers {
    char *power_ping_ptr;       // Power ping buffer
    char *power_pong_ptr;       // Power pong buffer
    char *power_current_ptr;    // Power current buffer
    char *traces_ping_ptr;      // Traces ping buffer
    char *traces_pong_ptr;      // Traces pong buffer
    char *traces_current_ptr;   // Traces current buffer
    char *online_ping_ptr;      // Online ping buffer
    char *online_pong_ptr;      // Online pong buffer
    char *online_current_ptr;   // Online current buffer
}_ping_pong_buffers_t;

// Static global buffers
static _ping_pong_buffers_t buffers;

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
        print_error("[PING_PONG_BUFFERS] Error when opening the file %s\n", filename);
        return NULL;
    }

    // Truncate the file
    ret = ftruncate(fd, size);
    if(ret < 0) {
        print_error("[PING_PONG_BUFFERS] Error truncating the file %s\n", filename);
        return NULL;
    }

    // Memory map the file in SHARED mode, which means that is visible for
    // other processes
    buffer = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(buffer == NULL) {
        print_error("[PING_PONG_BUFFERS] Error mmap'ing the file %s\n", filename);
        return NULL;
    }

    // Close the file descriptor (not needed anymore)
    ret = close(fd);
    if(ret < 0) {
        print_error("[PING_PONG_BUFFERS] Error closing the file %s\n", filename);
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
        print_error("[PING_PONG_BUFFERS] Error unmapping the ptr %p\n" ,buffer);
        return -1;
    }

    if(filename != NULL) {
		// Remove the shared memory file from the tmpfs
		ret = shm_unlink(filename);
		if(ret < 0) {
			print_error("[PING_PONG_BUFFERS] Error unlinking the file %s\n", filename);
			return -1;
		}
    }

    return 0;
}

/**
 * @brief Initialize the ping pong buffers and return them by reference
 * 
 * @param power_ptr Address to copy the ping power buffer to
 * @param traces_ptr Address to copy the ping traces buffer to
 * @param online_ptr Address to copy the ping online buffer to
 * @return (int) 0 on success, error code otherwise
 */
int ping_pong_buffers_init(char **power_ptr, char **traces_ptr, char **online_ptr) {

    // TODO: Handle errors

    // Power ping buffer
    buffers.power_ping_ptr = _create_buffer_file(POWER_PING_FILE_NAME, POWER_FILE_SIZE);
    if(buffers.power_ping_ptr == NULL) {
        print_error("[PING_PONG_BUFFERS] Error creating %s\n", POWER_PING_FILE_NAME);
        exit(1);
    }

    // Power pong buffer
    buffers.power_pong_ptr = _create_buffer_file(POWER_PONG_FILE_NAME, POWER_FILE_SIZE);
    if(buffers.power_pong_ptr == NULL) {
        print_error("[PING_PONG_BUFFERS] Error creating %s\n", POWER_PONG_FILE_NAME);
        exit(1);
    }

    // Power current buffer is the ping buffer by default
    buffers.power_current_ptr = buffers.power_ping_ptr;

    // Traces ping buffer
    buffers.traces_ping_ptr = _create_buffer_file(TRACES_PING_FILE_NAME, TRACES_FILE_SIZE);
    if(buffers.traces_ping_ptr == NULL) {
        print_error("[PING_PONG_BUFFERS] Error creating %s\n", TRACES_PING_FILE_NAME);
        exit(1);
    }

    // Traces pong buffer
    buffers.traces_pong_ptr = _create_buffer_file(TRACES_PONG_FILE_NAME, TRACES_FILE_SIZE);
    if(buffers.traces_pong_ptr == NULL) {
        print_error("[PING_PONG_BUFFERS] Error creating %s\n", TRACES_PONG_FILE_NAME);
        exit(1);
    }

    // Traces current buffer is the ping buffer by default
    buffers.traces_current_ptr = buffers.traces_ping_ptr;

    // Online ping buffer
    buffers.online_ping_ptr = _create_buffer_file(ONLINE_PING_FILE_NAME, ONLINE_FILE_SIZE);
    if(buffers.online_ping_ptr == NULL) {
        print_error("[PING_PONG_BUFFERS] Error creating %s\n", ONLINE_PING_FILE_NAME);
        exit(1);
    }

    // Online pong buffer
    buffers.online_pong_ptr = _create_buffer_file(ONLINE_PONG_FILE_NAME, ONLINE_FILE_SIZE);
    if(buffers.online_pong_ptr == NULL) {
        print_error("[PING_PONG_BUFFERS] Error creating %s\n", ONLINE_PONG_FILE_NAME);
        exit(1);
    }

    // Power current buffer is the ping buffer by default
    buffers.online_current_ptr = buffers.online_ping_ptr;

    // Actual outside assigment
    *power_ptr = buffers.power_current_ptr;
    *traces_ptr = buffers.traces_current_ptr;
    *online_ptr = buffers.online_current_ptr;

    return 0;
}

/**
 * @brief Clean the ping pong buffers
 * 
 * @param remove_buffers Place a 1 to remove the files that back the ping pong
 *                       buffers. With a 0 the files will remain on the fs
 * @return (int) 0 on success, error code otherwise
 */
int ping_pong_buffers_clean(const int remove_buffers) {

    int ret;
    char *tmp_file = NULL;

    // Power ping file
    if(remove_buffers) tmp_file = POWER_PING_FILE_NAME;
    ret = _close_buffer_file(buffers.power_ping_ptr, POWER_FILE_SIZE, tmp_file);
    if(ret < 0) {
        print_error("[PING_PONG_BUFFERS] Error cleaning %s\n", POWER_PING_FILE_NAME);
        exit(1);
    }

    // Power pong file
    if(remove_buffers) tmp_file = POWER_PONG_FILE_NAME;
    ret = _close_buffer_file(buffers.power_pong_ptr, POWER_FILE_SIZE, tmp_file);
    if(ret < 0) {
        print_error("[PING_PONG_BUFFERS] Error cleaning %s\n", POWER_PONG_FILE_NAME);
        exit(1);
    }

    // Traces ping file
    if(remove_buffers) tmp_file = TRACES_PING_FILE_NAME;
    ret = _close_buffer_file(buffers.traces_ping_ptr, TRACES_FILE_SIZE, tmp_file);
    if(ret < 0) {
        print_error("[PING_PONG_BUFFERS] Error cleaning %s\n", TRACES_PING_FILE_NAME);
        exit(1);
    }

    // Traces pong file
    if(remove_buffers) tmp_file = TRACES_PONG_FILE_NAME;
    ret = _close_buffer_file(buffers.traces_pong_ptr, TRACES_FILE_SIZE, tmp_file);
    if(ret < 0) {
        print_error("[PING_PONG_BUFFERS] Error cleaning %s\n", POWER_PING_FILE_NAME);
        exit(1);
    }

    // Online ping file
    if(remove_buffers) tmp_file = ONLINE_PING_FILE_NAME;
    ret = _close_buffer_file(buffers.online_ping_ptr, ONLINE_FILE_SIZE, tmp_file);
    if(ret < 0) {
        print_error("[PING_PONG_BUFFERS] Error cleaning %s\n", ONLINE_PING_FILE_NAME);
        exit(1);
    }

    // Online pong file
    if(remove_buffers) tmp_file = ONLINE_PONG_FILE_NAME;
    ret = _close_buffer_file(buffers.online_pong_ptr, ONLINE_FILE_SIZE, tmp_file);
    if(ret < 0) {
        print_error("[PING_PONG_BUFFERS] Error cleaning %s\n", ONLINE_PONG_FILE_NAME);
        exit(1);
    }

    // Clean ping-pong module variables
    buffers.power_current_ptr = NULL;
    buffers.power_ping_ptr = NULL;
    buffers.power_pong_ptr = NULL;
    buffers.traces_current_ptr = NULL;
    buffers.traces_ping_ptr = NULL;
    buffers.traces_pong_ptr = NULL;
    buffers.online_current_ptr = NULL;
    buffers.online_ping_ptr = NULL;
    buffers.online_pong_ptr = NULL;

    return 0;
}

/**
 * @brief Toggle the current buffers from ping to pong and vice-versa and
 *        return them by reference
 * 
 * @param power_ptr Address to copy the ping power buffer to
 * @param traces_ptr Address to copy the ping traces buffer to
 * @param online_ptr Address to copy the ping online buffer to
 * @return int 
 */
int ping_pong_buffers_toggle(char **power_ptr, char **traces_ptr, char **online_ptr) {

    // Power buffer
    if(buffers.power_current_ptr != buffers.power_ping_ptr && buffers.power_current_ptr != buffers.power_pong_ptr ) {
        print_error("[PING_PONG_BUFFERS] Error toggling power ptr\n");
        exit(1);
    }
    if(buffers.power_current_ptr == buffers.power_ping_ptr)
        buffers.power_current_ptr = buffers.power_pong_ptr;
    else
        buffers.power_current_ptr = buffers.power_ping_ptr;

    // Traces buffer
    if(buffers.traces_current_ptr != buffers.traces_ping_ptr && buffers.traces_current_ptr != buffers.traces_pong_ptr ) {
        print_error("[PING_PONG_BUFFERS] Error toggling power ptr\n");
        exit(1);
    }
    if(buffers.traces_current_ptr == buffers.traces_ping_ptr)
        buffers.traces_current_ptr = buffers.traces_pong_ptr;
    else
        buffers.traces_current_ptr = buffers.traces_ping_ptr;

    // Online buffer
    if(buffers.online_current_ptr != buffers.online_ping_ptr && buffers.online_current_ptr != buffers.online_pong_ptr ) {
        print_error("[PING_PONG_BUFFERS] Error toggling power ptr\n");
        exit(1);
    }
    if(buffers.online_current_ptr == buffers.online_ping_ptr)
        buffers.online_current_ptr = buffers.online_pong_ptr;
    else
        buffers.online_current_ptr = buffers.online_ping_ptr;

    // Actual outside assigment
    *power_ptr = buffers.power_current_ptr;
    *traces_ptr = buffers.traces_current_ptr;
    *online_ptr = buffers.online_current_ptr;

    return 0;
}
