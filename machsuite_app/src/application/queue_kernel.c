/**
 * @file queue_ll.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions to manage a linked list-based queue.
 *        This particular queue contains information about each kernel executed
 *        in the fpga ordered by arrival time
 * @date September 2022
 * 
 */

#include <stdlib.h>

#include "queue_kernel.h"


/**
 * @brief Initializes a Kernel Data queue
 * 
 * @param q Pointer to the queue to be initialized
 */
void init_queue(queue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

/**
 * @brief Remove every element from the queue
 * 
 * @param q Pointer to the queue to be cleaned
 */
void clean_queue(queue *q) {
    
    // Create a temporal node
    node* tmp;

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
int enqueue(queue *q, const kernel_data *d) {

    // Create a new node
    node *new_node = malloc(sizeof(*new_node));
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
 * @return (kernel_data*) Pointer to the recently added node
 */
kernel_data* enqueue_and_modify(queue *q, const kernel_data *d) {

    // Create a new node
    node *new_node = malloc(sizeof(*new_node));
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
int dequeue(queue *q, kernel_data *d) {

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Save the node we are going to dequeue
    node *tmp = q->head;
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
 * @brief Get the kernel information of the element in the position \p pos as a constant copy (head == 0)
 * 
 * @param q Pointer to the queue
 * @param pos Position inside the queue of the node to get information from
 * @param d Pointer where the function will store the node information
 * @return (int) 0 on success, error code otherwise 
 */
int get_kernel_info_from(queue *q, const int pos, kernel_data *d) {

    int i;

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Save the first node
    node *tmp = q->head;

    i = 1; // We loop through queue if the position is not the head
    while(i <= pos) {
        // Check if exist a node at that position
        if(tmp->next == NULL) return -1;
        // Get next node
        tmp = tmp->next;
        // Increment the iterator
        i++;
    }

    // Copy the node to get information from
    *d = tmp->data;

    return 0;
}

/**
 * @brief Remove an element from the position \p pos (head == 0)
 * 
 * @param q Pointer to the queue to remove a node from
 * @param pos Position inside the queue of the node to get information from
 * @param d Pointer where the function will store the dequeued node
 * @return (int) 0 on success, error code otherwise
 */
int dequeue_from(queue *q, const int pos, kernel_data *d) {

    int i;

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Special case for dequeuing HEAD
    if(pos == 0) {
        if(dequeue(q,d) < 0) return -1;
    }
    else{
        // Save the first node
        node *prev = NULL;
        node *tmp = q->head;

        i = 1;
        while(i <= pos) {
            // Save previous node
            prev = tmp;
            // Check if exist a node at that position
            if(prev->next == NULL) return -1;
            // Get next node
            tmp = prev->next;
            // Increment the iterator
            i++;
        }

        // Save the node we are going to dequeue
        *d = tmp->data;

        // Take the node off the queue
        prev->next = tmp->next;
        free(tmp);

        // Ensure the tail makes sense
        if(prev->next == NULL) {
            q->tail = prev;
        }
    }

    // Update size
    (q->size)--;

    return 0;
}

/**
 * @brief Remove the first executable kernel (cu < free_slots && kernel_id not duplicated)
 * 
 * @param q Pointer to the queue to dequeue the node from
 * @param free_slots Number of free slots
 * @param duplicated_kernels Array indicating wich kernels are duplicated
 * @param d Pointer where the function will store the dequeued node
 * @return (int) 0 on success, error code otherwise
 */
int dequeue_first_executable_kernel(queue *q, const int free_slots, const int *duplicated_kernels, kernel_data *d) {

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Save the first node
    node *prev = NULL;
    node *tmp = q->head;
    
    // Special case for HEAD
    if((tmp->data.cu <= free_slots) && (duplicated_kernels[tmp->data.kernel_label] == 0)) {
        
        // Save the node we are going to dequeue
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

    // Rest of the cases
    do {

        // Save previous node
        prev = tmp;
        // Check if exist a node at that position
        if(prev->next == NULL) return -1;
        // Get next node
        tmp = prev->next;

    }while( (tmp->data.cu > free_slots) || (duplicated_kernels[tmp->data.kernel_label] == 1) );

    // Save the node we are going to dequeue
    *d = tmp->data;

    // Take the node off the queue
    prev->next = tmp->next;
    free(tmp);
    
    // Ensure the tail makes sense
    if(prev->next == NULL) {
        q->tail = prev;
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
int is_queue_empty(const queue *q) {

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
int get_size_queue(const queue *q) {
    return q->size;
}