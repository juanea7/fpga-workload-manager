/**
 * @file queue_monitor.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions to manage a linked list-based queue.
 *        This particular queue contains information about each monitoring window
 * @date September 2022
 * 
 */

#include <stdlib.h>

#include "queue_traces.h"


/**
 * @brief Initializes a Monitor queue
 * 
 * @param q Pointer to the queue to be initialized
 */
void init_queue_monitor(queue_monitor *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

/**
 * @brief Remove every element from the queue
 * 
 * @param q Pointer to the queue to be cleaned
 */
void clean_queue_monitor(queue_monitor *q) {
    
    // Create a temporal node
    node_monitor* tmp;

    // Repite until the queue is empty
    while(q->head != NULL) {
        // Take the first node off the queue
        tmp = q->head;
        q->head = tmp->next;
        free(tmp);
    }
    // Ensure the tail makes sense
    q->tail = NULL;

    // Update size
    q->size = 0;
}

/**
 * @brief Add a node to the end of the queue
 * 
 * @param q Pointer to the queue to add a node to
 * @param d Pointer to the node to be added to the queue \p q 
 * @return (int) 0 on success, error code otherwise
 */
int enqueue_monitor(queue_monitor *q, const monitor_data *d) {

    // Create a new node
    node_monitor *new_node = malloc(sizeof(*new_node));
    if(new_node == NULL) return -1;
    new_node->data = *d;
    new_node->next = NULL;

    // If there is a tail, connect the tail to the new node
    if(q->tail != NULL) {
        q->tail->next = new_node;
    }
    q->tail = new_node;

    // If the queue was empty, the new node is the first node
    if(q->head == NULL) {
        q->head = new_node;
    }

    // Update size
    (q->size)++;

    return 0;
}

/**
 * @brief Add node to the end of the queue and get its pointer (to be able to modify it)
 * 
 * @param q Pointer to the queue to add a node to
 * @param d Pointer to the node to be added to the queue \p q 
 * @return (monitor_data*) Pointer to the recently added node
 */
monitor_data* enqueue_and_modify_monitor(queue_monitor *q, const monitor_data *d) {

    // Create a new node
    node_monitor *new_node = malloc(sizeof(*new_node));
    if(new_node == NULL) return NULL;
    new_node->data = *d;
    new_node->next = NULL;

    // If there is a tail, connect the tail to the new node
    if(q->tail != NULL) {
        q->tail->next = new_node;
    }
    q->tail = new_node;

    // If the queue was empty, the new node is the first node
    if(q->head == NULL) {
        q->head = new_node;
    }

    // Update size
    (q->size)++;

    return &(new_node->data);
}

/**
 * @brief Remove an element from the front of the queue
 * 
 * @param q Pointer to the queue to remove a node from
 * @param d Pointer where the function will store the dequeued node
 * @return (int) 0 on success, error code otherwise
 */
int dequeue_monitor(queue_monitor *q, monitor_data *d) {

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Save the node we are going to dequeue
    node_monitor *tmp = q->head;
    *d = tmp->data;

    // Take the node off the queue
    q->head = q->head->next;
    free(tmp);

    // Ensure the tail makes sense
    if(q->head == NULL) {
        q->tail = NULL;
    }

    // Update size
    (q->size)--;

    return 0;
}

/**
 * @brief Check if the queue is empty
 * 
 * @param q Pointer to the queue to be checked
 * @return (int) 1 if empty, 0 if not empty
 */
int is_queue_empty_monitor(const queue_monitor *q) {

    // Check if the queue is empty
    if(q->head == NULL) return 1;
    
    // Return 0 if not
    return 0;
}

/**
 * @brief Get the size of the queue
 * 
 * @param q Pointer to the queue to get the size of
 * @return (int) Size expressed in number of elements 
 */
int get_size_queue_monitor(const queue_monitor *q) {
    return q->size;
}