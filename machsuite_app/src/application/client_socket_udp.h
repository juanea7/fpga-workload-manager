/**
 * @file client_socket_udp.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions handle a client-side UDP Unix-domain socket
 * @date October 2022
 * 
 */

#ifndef _CLIENT_SOCKET_UDP_H_
#define _CLIENT_SOCKET_UDP_H_

#include <sys/un.h>
#include <netinet/in.h>


/**
 * @brief Create a UNIX-domain UDP socket
 * 
 * @param name Pointer to the structure describing the socket
 * @param path Path to the socket
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
int create_socket_udp_unix(struct sockaddr_un *name, const char *path);

/**
 * @brief Create a INET-domain UDP socket
 * 
 * @param name Pointer to the structure describing the socket
 * @param ip String indicating the ip of the server
 * @param port Port the server is listening to
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
int create_socket_udp_inet(struct sockaddr_in *name, const char *ip, const int port);

/**
 * @brief Send data to the server via the socket
 * 
 * @param socket_udp Socket file descriptor
 * @param data Data to send
 * @param data Size of the data to send
 * @param name Pointer to the structure describing the socket
 * @return (int) Number of bytes sent on success, -1 otherwise
 */
int send_data_to_socket_udp(const int socket_udp, const void *data, const size_t size, const struct sockaddr *name);

/**
 * @brief Close the socket
 * 
 * @param socket_udp Socket file descriptor
 */
void close_socket_udp(const int socket_udp);

/**
 * @brief Send buffer to the server via a INET UDP socket
 * 
 * @param socket_udp Socket file descriptor
 * @param socket_addr Structure describing the socket
 * @param buffer Buffer to be sent
 * @param buffer_size Size of the buffer to send
 * @return (int) 0 on success, -1 otherwise
 */
int send_buffer_socket_udp_inet(const int socket_udp, const struct sockaddr_in socket_addr, const void *buffer, const int buffer_size);


#endif /*_CLIENT_SOCKET_UDP_H_*/