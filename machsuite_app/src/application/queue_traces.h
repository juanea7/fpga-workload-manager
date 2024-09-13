/**
 * @file queue_monitor.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions to manage a linked list-based queue.
 *        This particular queue contains information about each monitoring window
 * @date September 2022
 * 
 */

#ifndef _QUEUE_MONITOR_H_
#define _QUEUE_MONITOR_H_

#include "data_structures.h"


/**
 * @brief Node structure forward declaration
 * 
 * @note Performing this forward declaration the node can contain a pointer to
 *       itself and be declared with typedef
 */
typedef struct node_monitor node_monitor;

/**
 * @brief Queue node structure
 * 
 */
struct node_monitor{
    monitor_data data;  // Data contained in the node
    node_monitor *next; // Pointer to the next linked node in the queue
};

/**
 * @brief Queue structure
 * 
 */
typedef struct {
    node_monitor *head; // Pointer to the head of the queue
    node_monitor *tail; // Pointer to the tail of the queue
    int size;           // Size of the queue, expressed in number of nodes
}queue_monitor;


/**
 * @brief Initializes a Monitor queue
 * 
 * @param q Pointer to the queue to be initialized
 */
void init_queue_monitor(queue_monitor *q);

/**
 * @brief Remove every element from the queue
 * 
 * @param q Pointer to the queue to be cleaned
 */
void clean_queue_monitor(queue_monitor *q);

/**
 * @brief Add a node to the end of the queue
 * 
 * @param q Pointer to the queue to add a node to
 * @param d Pointer to the node to be added to the queue \p q 
 * @return (int) 0 on success, error code otherwise
 */
int enqueue_monitor(queue_monitor *q, const monitor_data *d);

/**
 * @brief Add node to the end of the queue and get its pointer (to be able to modify it)
 * 
 * @param q Pointer to the queue to add a node to
 * @param d Pointer to the node to be added to the queue \p q 
 * @return (monitor_data*) Pointer to the recently added node
 */
monitor_data* enqueue_and_modify_monitor(queue_monitor *q, const monitor_data *d);

/**
 * @brief Remove an element from the front of the queue
 * 
 * @param q Pointer to the queue to remove a node from
 * @param d Pointer where the function will store the dequeued node
 * @return (int) 0 on success, error code otherwise
 */
int dequeue_monitor(queue_monitor *q, monitor_data *d);

/**
 * @brief Check if the queue is empty
 * 
 * @param q Pointer to the queue to be checked
 * @return (int) 1 if empty, 0 if not empty
 */
int is_queue_empty_monitor(const queue_monitor *q);

/**
 * @brief Get the size of the queue
 * 
 * @param q Pointer to the queue to get the size of
 * @return (int) Size expressed in number of elements 
 */
int get_size_queue_monitor(const queue_monitor *q);

#endif /* _QUEUE_MONITOR_H_ */