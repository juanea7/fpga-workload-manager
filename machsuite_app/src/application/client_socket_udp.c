/**
 * @file client_socket_udp.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions handle a client-side UDP Unix- and
 *        inet-domain socket.
 * @date October 2022
 * 
 */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "client_socket_udp.h"

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
 * @brief Create a UNIX-domain UDP socket
 * 
 * @param name Pointer to the structure describing the socket
 * @param path Path to the socket
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
int create_socket_udp_unix(struct sockaddr_un *name, const char *path) {

    int socket_udp;

    // Create the socket
    socket_udp = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(socket_udp < 0) {
        print_error("Opening UNIX UDP socket\n");
        return -1;
    }

    // Construct name of socket to send to
    name->sun_family = AF_UNIX;
    strcpy(name->sun_path, path);

    return socket_udp;
}

/**
 * @brief Create a INET-domain UDP socket
 * 
 * @param name Pointer to the structure describing the socket
 * @param ip String indicating the ip of the server
 * @param port Port the server is listening to
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
int create_socket_udp_inet(struct sockaddr_in *name, const char *ip, const int port) {

    int socket_udp;

    // Create the socket
    socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_udp < 0) {
        print_error("Opening INET UDP socket\n");
        return -1;
    }

    // Construct name of socket to send to
    name->sin_family = AF_INET; //IPv4
    name->sin_port = htons(port);
    inet_pton(AF_INET, ip, &(name->sin_addr));

    return socket_udp;
}

/**
 * @brief Send data to the server via the socket
 * 
 * @param socket_udp Socket file descriptor
 * @param data Data to send
 * @param data Size of the data to send
 * @param name Pointer to the structure describing the socket
 * @return (int) Number of bytes sent on success, -1 otherwise
 */
int send_data_to_socket_udp(const int socket_udp, const void *data, const size_t size, const struct sockaddr *name) {

    int ret;

    // Send data to the server
    ret = sendto(socket_udp, data, size, 0, name, sizeof(name));
    if (ret < 0) {
        print_error("UDP socket send error: %d: %s\n", ret, strerror(errno));
        return -1;
    }
    return ret;
}

/**
 * @brief Close the socket
 * 
 * @param socket_udp Socket file descriptor
 */
void close_socket_udp(const int socket_udp) {
    close(socket_udp);
}

/**
 * @brief Send buffer to the server via a INET UDP socket
 * 
 * @param socket_udp Socket file descriptor
 * @param socket_addr Structure describing the socket
 * @param buffer Buffer to be sent
 * @param buffer_size Size of the buffer to send
 * @return (int) 0 on success, -1 otherwise
 */
int send_buffer_socket_udp_inet(const int socket_udp, const struct sockaddr_in socket_addr, const void *buffer, const int buffer_size){

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
    ret = send_data_to_socket_udp(socket_udp, &buffer_info, sizeof(buffer_info), (const struct sockaddr *) &socket_addr);
    if (ret < 0) {
        print_error("Error UDP socket send data\n");
        exit(1);
    }

    // Send each of the packets
    for (i = 0; i < num_messages; i++){

        // Send max_message_size until the bytes_left are fewer that max_message_size
        bytes_to_send = bytes_left > MAX_PACKET_SIZE? MAX_PACKET_SIZE: bytes_left;

        // Send packet
        ret = send_data_to_socket_udp(socket_udp, current_addr, bytes_to_send,  (const struct sockaddr *) &socket_addr);
        if (ret < 0) {
            print_error("Error UDP socket send data\n");
            exit(1);
        }

        // Update current addres and bytes left
        current_addr += ret;
        bytes_left -= ret;
    }

    return 0;
}
