/**
 * @file queue_online.c
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

#include <stdlib.h>

#include "queue_online.h"


/**
 * @brief Initializes a Monitor queue
 * 
 * @param q Pointer to the queue to be initialized
 */
void init_queue_online(queue_online *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

/**
 * @brief Remove every element from the queue
 * 
 * @param q Pointer to the queue to be cleaned
 */
void clean_queue_online(queue_online *q) {
    
    // Create a temporal node
    node_online* tmp;

    // Repite until the queue is empty
    while(q->head != NULL){
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
int enqueue_online(queue_online *q, kernel_data *d) {

    // Create a new node
    node_online *new_node = malloc(sizeof(*new_node));
    if(new_node == NULL) return -1;
    new_node->data = d;
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
 * @brief Remove an element from the front of the queue
 * 
 * @param q Pointer to the queue to remove a node from
 * @param d Pointer where the function will store the dequeued node pointer
 * @return (int) 0 on success, error code otherwise
 */
int dequeue_online(queue_online *q, kernel_data **d) {

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Save the node we are going to dequeue
    node_online *tmp = q->head;
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
 * @brief Get the size of the queue
 * 
 * @param q Pointer to the queue to get the size of
 * @return (int) Size expressed in number of elements 
 */
int get_size_queue_online(const queue_online *q) {
    return q->size;
}