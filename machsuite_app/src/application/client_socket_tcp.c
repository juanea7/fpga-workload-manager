/**
 * @file client_socket_tcp.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions handle a client-side TCP Unix- and 
 *        INET-domain socket.
 * @date March 2023
 * 
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "client_socket_tcp.h"

#include "debug.h"


// The limit is around ~64kB (64KB - 20 bits of header and something else)
#define MAX_PACKET_SIZE (1024 * 32)

/**
 * @brief Socket buffer transmission related info
 * 
 */
typedef struct _socket_buffer_info{
    int num_packets;            // Number of packet within the whole transmission
    int regular_packet_size;    // Size of each packet, except the last one (in bytes)
    int last_packet_size;       // Size of the last packet (in bytes)
}_socket_buffer_info_t;

/**
 * @brief Create a UNIX-domain TCP socket
 * 
 * @param name Pointer to the structure describing the socket
 * @param path Path to the socket
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
int create_socket_tcp_unix(struct sockaddr_un *name, const char *path) {

    int socket_tcp, len, ret;

    // Create the socket
    socket_tcp = socket(AF_UNIX, SOCK_STREAM, 0);
    if(socket_tcp < 0) {
        print_error("Opening stream socket\n");
        return -1;
    }

    // Construct name of socket to send to
    name->sun_family = AF_UNIX;
    strcpy(name->sun_path, path);

    // Connect to the socket
    len = strlen(name->sun_path) + sizeof(name->sun_family);
    ret = connect(socket_tcp, (struct sockaddr *)name, len);
    if (ret == -1) {
        print_error("Connecting to the TCP prediction socket. | len:%d # ret: %d\n", len, ret);
        return -1;
    }

    return socket_tcp;
}

/**
 * @brief Create a INET-domain TCP socket
 * 
 * @param name Pointer to the structure describing the socket
 * @param ip String indicating the ip of the server
 * @param port Port the server is listening to
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
int create_socket_tcp_inet(struct sockaddr_in *name, const char *ip, const int port) {

    int socket_tcp, ret;

    // Create the socket
    socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_tcp < 0) {
        printf("Opening TCP socket\n");
        return -1;
    }

    // Construct name of socket to send to
    name->sin_family = AF_INET; //IPv4
    name->sin_port = htons(port);
    inet_pton(AF_INET, ip, &(name->sin_addr));

    // Connect to the socket
    ret = connect(socket_tcp, (struct sockaddr *)name, sizeof(struct sockaddr));
    if (ret == -1) {
        print_error("Connecting to the UNET-domain TCP socket. %d: %s\n", ret, strerror(errno));
        return -1;
    }

    return socket_tcp;
}

/**
 * @brief Send data to the server via the socket
 * 
 * @param socket_tcp Socket file descriptor
 * @param data Data to send
 * @param size Size of the data to send
 * @return (int) Number of bytes sent on success, -1 otherwise
 */
int send_data_to_socket_tcp(const int socket_tcp, const void *data, const size_t size) {

    int ret;

    // Send data to the server
    ret = send(socket_tcp, data, size, 0);
    if (ret < 0) {
        print_error("TCP socket send error: %d: %s\n", ret, strerror(errno));
        return -1;
    }
    return ret;
}

/**
 * @brief Receive data from the server via the socket
 * 
 * @param socket_tcp Socket file descriptor
 * @param data Buffer for the received data
 * @param size Size of the received data
 * @return (int) Number of bytes received on success, -1 otherwise
 */
int recv_data_from_socket_tcp(const int socket_tcp, void *data, const size_t size) {

    int ret;

    // Send data to the server
    ret = recv(socket_tcp, data, size, 0);
    if (ret < 0) {
        print_error("TCP socket recv error: %d: %s\n", ret, strerror(errno));
        return -1;
    }
    return ret;
}

/**
 * @brief Close the socket
 * 
 * @param socket_tcp Socket file descriptor
 */
void close_socket_tcp(const int socket_tcp) {
    close(socket_tcp);
}

/**
 * @brief Send buffer to the server via a tcp socket
 * 
 * @param socket_tcp Socket file descriptor
 * @param buffer Buffer to be sent
 * @param buffer_size Size of the buffer to send
 * @return (int) 0 on success, -1 otherwise
 */
int send_buffer_socket_tcp_inet(const int socket_tcp, const void *buffer, const int buffer_size){

    int i, ret;
    int num_messages, bytes_left, bytes_to_send;
    void *current_addr;
    _socket_buffer_info_t buffer_info;

    // Initialize the variables that manage the buffer slicing
    bytes_left = buffer_size;
    current_addr = buffer;
    num_messages = (buffer_size + (MAX_PACKET_SIZE - 1)) / MAX_PACKET_SIZE;  // integer round-up

    // Send whole buffer transmission info
    buffer_info.num_packets = num_messages;
    buffer_info.regular_packet_size = MAX_PACKET_SIZE;
    buffer_info.last_packet_size = buffer_size % MAX_PACKET_SIZE;

    // Send the structure containing the whole transmission information    
    ret = send_data_to_socket_tcp(socket_tcp, &buffer_info, sizeof(buffer_info));
    if (ret < 0) {
        print_error("Error TCP socket send data\n");
        exit(1);
    }

    // Send each of the packets
    for (i = 0; i < num_messages; i++){

        // Send max_message_size until the bytes_left are fewer that max_message_size
        bytes_to_send = bytes_left > MAX_PACKET_SIZE? MAX_PACKET_SIZE: bytes_left;

        // Send packet
        ret = send_data_to_socket_tcp(socket_tcp, current_addr, bytes_to_send);
        if (ret < 0) {
            print_error("Error TCP socket send data\n");
            exit(1);
        }

        // Update current addres and bytes left
        current_addr += ret;
        bytes_left -= ret;
    }

    return 0;
}
