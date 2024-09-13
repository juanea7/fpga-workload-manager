/**
 * @file queue_online.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions to manage a linked list-based queue.
 *        This particular queue contains information about which kernel is 
 *        executed and when in a particular ARTICo3 slot, therefore, there will
 *        be as many instances as ARTICo3 slots.
 * @note This queue does not contain the kernel data itself in the node, but a 
 *       pointer to its on the Kernel Data Queue. 
 *       This is performed this way to avoid duplicated data in memoryand for 
 *       coding convenience.
 * @date September 2022
 * 
 */

#ifndef _QUEUE_ONLINE_H_
#define _QUEUE_ONLINE_H_

#include "data_structures.h"

/**
 * @brief Node structure forward declaration
 * 
 * @note Performing this forward declaration the node can contain a pointer to
 *       itself and be declared with typedef
 */
typedef struct node_online node_online;

/**
 * @brief Queue node structure
 * 
 */
struct node_online{
    kernel_data *data;      // Pointer to a kernal_data structure
    node_online *next; // Pointer to the next linked node in the queue
};

/**
 * @brief Queue structure
 * 
 */
typedef struct {
    node_online *head; // Pointer to the head of the queue
    node_online *tail; // Pointer to the tail of the queue
    int size;          // Size of the queue, expressed in number of nodes
}queue_online;


/**
 * @brief Initializes a Monitor queue
 * 
 * @param q Pointer to the queue to be initialized
 */
void init_queue_online(queue_online *q);

/**
 * @brief Remove every element from the queue
 * 
 * @param q Pointer to the queue to be cleaned
 */
void clean_queue_online(queue_online *q);

/**
 * @brief Add a node to the end of the queue
 * 
 * @param q Pointer to the queue to add a node to
 * @param d Pointer to the node to be added to the queue \p q 
 * @return (int) 0 on success, error code otherwise
 */
int enqueue_online(queue_online *q, kernel_data *d);

/**
 * @brief Remove an element from the front of the queue
 * 
 * @param q Pointer to the queue to remove a node from
 * @param d Pointer where the function will store the dequeued node pointer
 * @return (int) 0 on success, error code otherwise
 */
int dequeue_online(queue_online *q, kernel_data **d);

/**
 * @brief Get the size of the queue
 * 
 * @param q Pointer to the queue to get the size of
 * @return (int) Size expressed in number of elements 
 */
int get_size_queue_online(const queue_online *q);

#endif /* _QUEUE_ONLINE_H_ */