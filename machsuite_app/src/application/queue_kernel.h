/**
 * @file queue_kernel.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions to manage a linked list-based queue.
 *        This particular queue contains information about each kernel executed
 *        in the fpga ordered by arrival time
 * @date September 2022
 *
 */

#ifndef _QUEUE_KERNEL_H_
#define _QUEUE_KERNEL_H_

#include "data_structures.h"

// TODO: place online models in data_structures or remove online_models from here.
#include "online_models.h"

/**
 * @brief Node structure forward declaration
 *
 * @note Performing this forward declaration the node can contain a pointer to
 *       itself and be declared with typedef
 */
typedef struct node node;

/**
 * @brief Queue node structure
 *
 */
struct node{
    kernel_data data;   // Data contained in the node
    node *next;         // Pointer to the next linked node in the queue
};

/**
 * @brief Queue structure
 *
 */
typedef struct {
    node *head; // Pointer to the head of the queue
    node *tail; // Pointer to the tail of the queue
    int size;   // Size of the queue, expressed in number of nodes
}queue;

/**
 * @brief Initializes a Kernel Data queue
 *
 * @param q Pointer to the queue to be initialized
 */
void init_queue(queue *q);

/**
 * @brief Remove every element from the queue
 *
 * @param q Pointer to the queue to be cleaned
 */
void clean_queue(queue *q);

/**
 * @brief Add a node to the end of the queue
 *
 * @param q Pointer to the queue to add a node to
 * @param d Pointer to the node to be added to the queue \p q
 * @return (int) 0 on success, error code otherwise
 */
int enqueue(queue *q, const kernel_data *d);

/**
 * @brief Add node to the end of the queue and get its pointer (to be able to modify it)
 *
 * @param q Pointer to the queue to add a node to
 * @param d Pointer to the node to be added to the queue \p q
 * @return (kernel_data*) Pointer to the recently added node
 */
kernel_data* enqueue_and_modify(queue *q, const kernel_data *d);

/**
 * @brief Remove an element from the front of the queue
 *
 * @param q Pointer to the queue to remove a node from
 * @param d Pointer where the function will store the dequeued node
 * @return (int) 0 on success, error code otherwise
 */
int dequeue(queue *q, kernel_data *d);

/**
 * @brief Get the kernel information of the element in the position \p pos as a constant copy (head == 0)
 *
 * @param q Pointer to the queue
 * @param pos Position inside the queue of the node to get information from
 * @param d Pointer where the function will store the node information
 * @return (int) 0 on success, error code otherwise
 */
int get_kernel_info_from(queue *q, const int pos, kernel_data *d);

/**
 * @brief Remove an element from the position \p pos (head == 0)
 *
 * @param q Pointer to the queue to remove a node from
 * @param pos Position inside the queue of the node to get information from
 * @param d Pointer where the function will store the dequeued node
 * @return (int) 0 on success, error code otherwise
 */
int dequeue_from(queue *q, const int pos, kernel_data *d);

/**
 * @brief Remove the first executable kernel (cu < free_slots && kernel_id not duplicated)
 *
 * @param q Pointer to the queue to dequeue the node from
 * @param free_slots Number of free slots
 * @param duplicated_kernels Array indicating wich kernels are duplicated
 * @param d Pointer where the function will store the dequeued node
 * @return (int) 0 on success, error code otherwise
 */
int dequeue_first_executable_kernel(queue *q, const int free_slots, const int *duplicated_kernels, kernel_data *d);

/**
 * @brief Schedule the Least Interaction First (LIF) kernel from the queue
 *
 * @param q Pointer to the queue to dequeue the node from
 * @param free_slots Number of free slots
 * @param duplicated_kernels Array indicating wich kernels are duplicated
 * @param d Pointer where the function will store the dequeued node
 * @param om Pointer to the online models structure
 * @param num_kernels_to_check Number of kernels to check
 * @param user_cpu User CPU usage
 * @param kernel_cpu Kernel CPU usage
 * @param idle_cpu Idle CPU usage
 * @param predicted_time_alone_map Map with the predicted time alone for each kernel
 * @return (int) 0 on success, error code otherwise
 */
int schedule_lif_from_n_executable_kernels(queue *q, const int free_slots, const int *duplicated_kernels, kernel_data *d, const online_models_t *om, const int num_kernels_to_check, const float user_cpu, const float kernel_cpu, const float idle_cpu);

/**
 * @brief Schedule the Shortest Job First (SJF) kernel from the queue
 *
 * @param q Pointer to the queue to dequeue the node from
 * @param free_slots Number of free slots
 * @param duplicated_kernels Array indicating wich kernels are duplicated
 * @param d Pointer where the function will store the dequeued node
 * @param om Pointer to the online models structure
 * @param num_kernels_to_check Number of kernels to check
 * @param user_cpu User CPU usage
 * @param kernel_cpu Kernel CPU usage
 * @param idle_cpu Idle CPU usage
 * @param predicted_time_alone_map Map with the predicted time alone for each kernel
 * @return (int) 0 on success, error code otherwise
 */
int schedule_sjf_from_n_executable_kernels(queue *q, const int free_slots, const int *duplicated_kernels, kernel_data *d, const online_models_t *om, const int num_kernels_to_check, const float user_cpu, const float kernel_cpu, const float idle_cpu);

/**
 * @brief Check if the queue is empty
 *
 * @param q Pointer to the queue to be checked
 * @return (int) 1 if empty, 0 if not empty
 */
int is_queue_empty(const queue *q);

/**
 * @brief Get the size of the queue
 *
 * @param q Pointer to the queue to get the size of
 * @return (int) Size expressed in number of elements
 */
int get_size_queue(const queue *q);

#endif /* _QUEUE_KERNEL_H_ */