/**
 * @file client_socket_tcp.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions handle a client-side TCP Unix-domain
 *        socket used for predicting with the online models.
 * @date March 2023
 * 
 */
#ifndef _CLIENT_SOCKET_TCP_H_
#define _CLIENT_SOCKET_TCP_H_

#include <sys/un.h>
#include <netinet/in.h>


/**
 * @brief Create a UNIX-domain TCP socket
 * 
 * @param name Pointer to the structure describing the socket
 * @param path Path to the socket
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
int create_socket_tcp_unix(struct sockaddr_un *name, const char *path);

/**
 * @brief Create a INET-domain TCP socket
 * 
 * @param name Pointer to the structure describing the socket
 * @param ip String indicating the ip of the server
 * @param port Port the server is listening to
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
int create_socket_tcp_inet(struct sockaddr_in *name, const char *ip, const int port);

/**
 * @brief Send data to the server via the socket
 * 
 * @param socket_tcp Socket file descriptor
 * @param data Data to send
 * @param size Size of the data to send
 * @return (int) Number of bytes sent on success, -1 otherwise
 */
int send_data_to_socket_tcp(const int socket_tcp, const void *data, const size_t size);

/**
 * @brief Receive data from the server via the socket
 * 
 * @param socket_tcp Socket file descriptor
 * @param data Buffer for the received data
 * @param size Size of the received data
 * @return (int) Number of bytes received on success, -1 otherwise
 */
int recv_data_from_socket_tcp(const int socket_tcp, void *data, const size_t size);

/**
 * @brief Close the socket
 * 
 * @param socket_tcp Socket file descriptor
 */
void close_socket_tcp(const int socket_tcp);

/**
 * @brief Send buffer to the server via a tcp socket
 * 
 * @param socket_tcp Socket file descriptor
 * @param buffer Buffer to be sent
 * @param buffer_size Size of the buffer to send
 * @return (int) 0 on success, -1 otherwise
 */
int send_buffer_socket_tcp_inet(const int socket_tcp, const void *buffer, const int buffer_size);


#endif /*_CLIENT_SOCKET_TCP_H_*/