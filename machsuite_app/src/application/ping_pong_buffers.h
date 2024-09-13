/**
 * @file ping_pong_buffers.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions that handle the power, traces and 
 *        online data ping-pongvbuffers used for online traces processing 
 *        on-ram.
 * @date March 2023
 * 
 */

#ifndef _PING_PONG_BUFFERS_H_
#define _PING_PONG_BUFFERS_H_


/**
 * @brief Initialize the ping pong buffers and return them by reference
 * 
 * @param power_ptr Address to copy the ping power buffer to
 * @param traces_ptr Address to copy the ping traces buffer to
 * @param online_ptr Address to copy the ping online buffer to
 * @return (int) 0 on success, error code otherwise
 */
int ping_pong_buffers_init(char **power_ptr, char **traces_ptr, char **online_ptr);

/**
 * @brief Clean the ping pong buffers
 * 
 * @param remove_buffers Place a 1 to remove the files that back the ping pong
 *                       buffers. With a 0 the files will remain on the fs
 * @return (int) 0 on success, error code otherwise
 */
int ping_pong_buffers_clean(const int remove_buffers);

/**
 * @brief Toggle the current buffers from ping to pong and vice-versa and
 *        return them by reference
 * 
 * @param power_ptr Address to copy the ping power buffer to
 * @param traces_ptr Address to copy the ping traces buffer to
 * @param online_ptr Address to copy the ping online buffer to
 * @return int 
 */
int ping_pong_buffers_toggle(char **power_ptr, char **traces_ptr, char **online_ptr);


#endif /*_PING_PONG_BUFFERS_H_*/