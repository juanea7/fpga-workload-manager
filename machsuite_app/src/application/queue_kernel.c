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

#include "online_models.h"


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

    }while( (tmp->data.cu > free_slots) || (duplicated_kernels[tmp->data.kernel_label] > 0) );

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
 * @brief Schedule using a Crow Search Algorithm (CSA) to select kernel configuration from the queue
 *
 * @param q Pointer to the queue to dequeue the node from
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
int schedule_CSA_from_n_executable_kernels(queue *q, const int *duplicated_kernels, kernel_data *d, const online_models_t *om, const int num_kernels_to_check, const float user_cpu, const float kernel_cpu, const float idle_cpu, const int reset_prior_decisions) {

    // printf("[SCHED] Scheduling decision using CSA\n");

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Save the first node
    node *prev = NULL;
    node *tmp = q->head;

    // Scheduling variables
    int tmp_kernel_index = 0;                               // Temporal kernel index
    int num_kernels_checked = 0;                            // Number of kernels checked
    int selected_kernel_index = -1;                         // Index of the selected kernel to schedule
    int selected_kernel_cu = 0;                             // CUs of the selected kernel to schedule

    // Scheduling variables that last until the next scheduling decision
    static int kernels_to_schedule[TYPES_OF_KERNELS] = {0};        // Array to indicate which kernels are schedulable
    static int kernels_to_schedule_index[TYPES_OF_KERNELS] = {-1}; // Array to indicate the index of the kernels to schedule
    static int num_kernels_to_schedule = 0;                        // Number of kernels to schedule

    // TODO: remove, debug
    int num_decision_changes = 0;
    static int num_first_kernel_scheduled = 0;
    static int num_other_kernel_scheduled = 0;

    int i;

    // Online models variables
	online_models_features_t scheduling_request;
    online_models_schedule_decision_t scheduling_decision;

    // Reset prior schedule decisions if needed
    if (reset_prior_decisions == 1) {
        // printf("[SCHED] Resetting prior schedule decisions\n");
        for (i = 0; i < TYPES_OF_KERNELS; i++) {
            kernels_to_schedule[i] = 0;
            kernels_to_schedule_index[i] = -1;
        }
        num_kernels_to_schedule = 0;
    }

    // Go to schedule peding kernels if any
    // printf("[SCHED] Number of kernels to schedule0: %d\n", num_kernels_to_schedule);
    if (num_kernels_to_schedule > 0) goto schedule_peding_kernels;

    // Include the cpu_usage and running kernels in the scheduling request
    scheduling_request.user = user_cpu;
    scheduling_request.kernel = kernel_cpu;
    scheduling_request.idle = idle_cpu;
    scheduling_request.main = 0xFF;                         // Tell the python code is an scheduling request
    scheduling_request.aes = duplicated_kernels[0];
    scheduling_request.bulk = duplicated_kernels[1];
    scheduling_request.crs = duplicated_kernels[2];
    scheduling_request.kmp = duplicated_kernels[3];
    scheduling_request.knn = duplicated_kernels[4];
    scheduling_request.merge = duplicated_kernels[5];
    scheduling_request.nw = duplicated_kernels[6];
    scheduling_request.queue = duplicated_kernels[7];
    scheduling_request.stencil2d = duplicated_kernels[8];
    scheduling_request.stencil3d = duplicated_kernels[9];
    scheduling_request.strided = duplicated_kernels[10];

    // Special case for HEAD
    if(duplicated_kernels[tmp->data.kernel_label] == 0) {

        // Include the kernel label in the scheduling request
        add_kernel_label_to_scheduling_request(&scheduling_request, tmp->data.kernel_label);

        // Include the kernel as schedulable
        kernels_to_schedule[tmp->data.kernel_label] = 1;
        kernels_to_schedule_index[tmp->data.kernel_label] = tmp_kernel_index;
        num_kernels_to_schedule++;

        // TODO: remove, debug. Check if the scheduler has taken a decition different that fifo
        num_decision_changes++;

        // Increment the number of kernels checked
        num_kernels_checked++;
    }
    // printf("[SCHED] Number of kernels to schedule1: %d\n", num_kernels_to_schedule);

    // Loop until we run out of kernels in the queue or we have checked the desired number of kernels
    while(num_kernels_checked < num_kernels_to_check) {

        // Rest of the cases
        do {

            // Save previous node
            prev = tmp;
            // Check if exist a node at that position
            // TODO: Remove the goto by spliting this function in multiple and return when this condition happen
            // TODO: This is a temporal solution to go out of the nested loop
            if(prev->next == NULL){
                // printf("[SCHED] No more kernels executable in queue\n");
                goto no_more_kernels;
            }
            // Get next node
            tmp = prev->next;
            // tmp_index
            tmp_kernel_index++;

        // }while( (tmp->data.cu > free_slots) || (duplicated_kernels[tmp->data.kernel_label] == 1) );
        }while( (duplicated_kernels[tmp->data.kernel_label] > 0) || (kernels_to_schedule[tmp->data.kernel_label] == 1) );

        // Include the kernel label in the scheduling request
        add_kernel_label_to_scheduling_request(&scheduling_request, tmp->data.kernel_label);

        // Include the kernel label in the kernels to check
        kernels_to_schedule[tmp->data.kernel_label] = 1;
        kernels_to_schedule_index[tmp->data.kernel_label] = tmp_kernel_index;
        num_kernels_to_schedule++;

        // Increment the number of kernels checked
        num_kernels_checked++;
    }
    // printf("[SCHED] Number of kernels to schedule2: %d\n", num_kernels_to_schedule);

    no_more_kernels:  // Label to go out of the nested loop when no more kernels are available
    if (num_kernels_to_schedule == 0) return -1;

    // Request scheduling decision
    scheduling_decision = online_models_schedule(om, &scheduling_request);

    // Get the kernels to be scheduled
    for (i = 0; i < TYPES_OF_KERNELS; i++) {
        if (kernels_to_schedule[i] == 1) {
            // Get the CUs for the kernel
            kernels_to_schedule[i] = get_kernel_from_scheduling_decision(&scheduling_decision, i);
            // Clean kernel index if the scheduler decides not to schedule it
            // and update the number of kernels to schedule
            if (kernels_to_schedule[i] == 0) {
                kernels_to_schedule_index[i] = -1;
                num_kernels_to_schedule--;
            }
        }
    }

    schedule_peding_kernels: // Label to go to schedule pending kernels

    // printf("[SCHED] Number of kernels to schedule: %d\n", num_kernels_to_schedule);
    // printf("[SCHED] Kernels to schedule: ");
    // for (i = 0; i < TYPES_OF_KERNELS; i++) {
    //     printf("%d ", kernels_to_schedule[i]);
    // }
    // printf("\n");
    // printf("[SCHED] Kernels to schedule index: ");
    // for (i = 0; i < TYPES_OF_KERNELS; i++) {
    //     printf("%d ", kernels_to_schedule_index[i]);
    // }
    // printf("\n");

    // Get the first selected kernel to schedule
    for (i = 0; i < TYPES_OF_KERNELS; i++) {
        if (kernels_to_schedule[i] > 0) {
            selected_kernel_index = kernels_to_schedule_index[i];
            selected_kernel_cu = kernels_to_schedule[i];
            // Clean the kernel info from the scheduling variables
            kernels_to_schedule[i] = 0;
            kernels_to_schedule_index[i] = -1;
            // printf("[SCHED] Kernel %d selected to schedule (CUs: %d)\n", i, selected_kernel_cu);
            break;
        }
    }

    // Check if there are kernels to schedule
    if (selected_kernel_index == -1) {
        if (num_kernels_to_schedule > 0) {
            print_error("%d kernels remain to be scheduled, but 'selected_kernel_index == -1'\n", num_kernels_to_schedule);
            exit(1);
        }
        // No kernel to schedule
        return -1;
    }

    // Dequeue the kernel decided by the CSA
    if(dequeue_from(q, selected_kernel_index, d) < 0){
        print_error("Error dequeuing kernel from queue when scheduling\n");
        exit(1);
    }

    // Set the kernel CUs
    d->cu = selected_kernel_cu;
    num_kernels_to_schedule--;

    // Adjust the kernel index of the rest of the kernels
    if (num_kernels_to_schedule > 0) {
        // printf("[SCHED] Adjusting kernel index of the rest of the kernels\n");
        for (i = 0; i < TYPES_OF_KERNELS; i++) {
            if (kernels_to_schedule_index[i] > selected_kernel_index) {
                kernels_to_schedule_index[i]--;
            }
        }
    }

    // TODO: remove, debug. Indicate the number of times the scheduler takes a decision different than fifo
    if (num_decision_changes == 1)
        num_first_kernel_scheduled++;
    else
        num_other_kernel_scheduled++;

    return 0;
}

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
int schedule_lif_from_n_executable_kernels(queue *q, const int free_slots, const int *duplicated_kernels, kernel_data *d, const online_models_t *om, const int num_kernels_to_check, const float user_cpu, const float kernel_cpu, const float idle_cpu) {

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Save the first node
    node *prev = NULL;
    node *tmp = q->head;

    // Scheduling variables
    float min_kernel_interaction = __FLT_MAX__;  // Stores the mininum predicted kernel interaction
    int min_kernel_interaction_index = -1;       // Stores the index of the kernel with the minimum predicted kernel interaction
    float tmp_kernel_interaction = 0;            // Temporal kernel interaction variabl
    int tmp_kernel_index = 0;                    // Temporal kernel index
    int num_kernels_checked = 0;                 // Number of kernels checked

    int num_decision_changes = 0;
    static int num_first_kernel_scheduled = 0;
    static int num_other_kernel_scheduled = 0;

    // Online models variables
	online_models_features_t features_alone, features_interaction;
	online_models_prediction_t prediction_alone, prediction_interaction;

    int tmp_features_alone_array[TYPES_OF_KERNELS] = {0};
    int tmp_features_interaction_array[TYPES_OF_KERNELS];

    // Special case for HEAD
    if((tmp->data.cu <= free_slots) && (duplicated_kernels[tmp->data.kernel_label] == 0)) {

        // TODO: encapsulate this in a function
        // Prepare the features (TODO: change data structure to avoid this)

        // Features alone
        tmp_features_alone_array[tmp->data.kernel_label] = 1;

        features_alone.user = user_cpu;
        features_alone.kernel = kernel_cpu;
        features_alone.idle = idle_cpu;
        features_alone.main = tmp->data.kernel_label;
        features_alone.aes = tmp_features_alone_array[0];
        features_alone.bulk = tmp_features_alone_array[1];
        features_alone.crs = tmp_features_alone_array[2];
        features_alone.kmp = tmp_features_alone_array[3];
        features_alone.knn = tmp_features_alone_array[4];
        features_alone.merge = tmp_features_alone_array[5];
        features_alone.nw = tmp_features_alone_array[6];
        features_alone.queue = tmp_features_alone_array[7];
        features_alone.stencil2d = tmp_features_alone_array[8];
        features_alone.stencil3d = tmp_features_alone_array[9];
        features_alone.strided = tmp_features_alone_array[10];

        // Prediction alone
	    prediction_alone = online_models_predict(om, &features_alone);

        // Clean the features alone array
        tmp_features_alone_array[tmp->data.kernel_label] = 0;

        // Features interaction
        memcpy(tmp_features_interaction_array, duplicated_kernels, sizeof(tmp_features_interaction_array));
        tmp_features_interaction_array[tmp->data.kernel_label] = tmp->data.cu;

        features_interaction.user = user_cpu;
        features_interaction.kernel = kernel_cpu;
        features_interaction.idle = idle_cpu;
        features_interaction.main = tmp->data.kernel_label;
        features_interaction.aes = tmp_features_interaction_array[0];
        features_interaction.bulk = tmp_features_interaction_array[1];
        features_interaction.crs = tmp_features_interaction_array[2];
        features_interaction.kmp = tmp_features_interaction_array[3];
        features_interaction.knn = tmp_features_interaction_array[4];
        features_interaction.merge = tmp_features_interaction_array[5];
        features_interaction.nw = tmp_features_interaction_array[6];
        features_interaction.queue = tmp_features_interaction_array[7];
        features_interaction.stencil2d = tmp_features_interaction_array[8];
        features_interaction.stencil3d = tmp_features_interaction_array[9];
        features_interaction.strided = tmp_features_interaction_array[10];

        // Predict interaction
	    prediction_interaction = online_models_predict(om, &features_interaction);

        // TODO: remove, debug. Check if the scheduler has taken a decition different that fifo
        num_decision_changes++;


        // Compute the predicted kernel interaction
        min_kernel_interaction = (prediction_interaction.time - prediction_alone.time) / prediction_alone.time;

        // Set the kernel index
        min_kernel_interaction_index = 0;

        // Increment the number of kernels checked
        num_kernels_checked++;
    }

    // Loop until we run out of kernels in the queue or we have checked the desired number of kernels
    while(num_kernels_checked < num_kernels_to_check) {

        // Rest of the cases
        do {

            // Save previous node
            prev = tmp;
            // Check if exist a node at that position
            // TODO: Remove the goto by spliting this function in multiple and return when this condition happen
            // TODO: This is a temporal solution to go out of the nested loop
            if(prev->next == NULL){
                // printf("[SCHED] No more kernels executable in queue\n");
                goto no_more_kernels;
            }
            // Get next node
            tmp = prev->next;
            // tmp_index
            tmp_kernel_index++;

        // }while( (tmp->data.cu > free_slots) || (duplicated_kernels[tmp->data.kernel_label] == 1) );
        }while( (tmp->data.cu > free_slots) || (duplicated_kernels[tmp->data.kernel_label] > 0) );

        // TODO: encapsulate this in a function
        // Prepare the features (TODO: change data structure to avoid this)

        // Features alone
        tmp_features_alone_array[tmp->data.kernel_label] = 1;

        features_alone.user = user_cpu;
        features_alone.kernel = kernel_cpu;
        features_alone.idle = idle_cpu;
        features_alone.main = tmp->data.kernel_label;
        features_alone.aes = tmp_features_alone_array[0];
        features_alone.bulk = tmp_features_alone_array[1];
        features_alone.crs = tmp_features_alone_array[2];
        features_alone.kmp = tmp_features_alone_array[3];
        features_alone.knn = tmp_features_alone_array[4];
        features_alone.merge = tmp_features_alone_array[5];
        features_alone.nw = tmp_features_alone_array[6];
        features_alone.queue = tmp_features_alone_array[7];
        features_alone.stencil2d = tmp_features_alone_array[8];
        features_alone.stencil3d = tmp_features_alone_array[9];
        features_alone.strided = tmp_features_alone_array[10];

        // Prediction alone
	    prediction_alone = online_models_predict(om, &features_alone);

        // Clean the features alone array
        tmp_features_alone_array[tmp->data.kernel_label] = 0;

        // Features interaction
        memcpy(tmp_features_interaction_array, duplicated_kernels, sizeof(tmp_features_interaction_array));
        tmp_features_interaction_array[tmp->data.kernel_label] = tmp->data.cu;

        features_interaction.user = user_cpu;
        features_interaction.kernel = kernel_cpu;
        features_interaction.idle = idle_cpu;
        features_interaction.main = tmp->data.kernel_label;
        features_interaction.aes = tmp_features_interaction_array[0];
        features_interaction.bulk = tmp_features_interaction_array[1];
        features_interaction.crs = tmp_features_interaction_array[2];
        features_interaction.kmp = tmp_features_interaction_array[3];
        features_interaction.knn = tmp_features_interaction_array[4];
        features_interaction.merge = tmp_features_interaction_array[5];
        features_interaction.nw = tmp_features_interaction_array[6];
        features_interaction.queue = tmp_features_interaction_array[7];
        features_interaction.stencil2d = tmp_features_interaction_array[8];
        features_interaction.stencil3d = tmp_features_interaction_array[9];
        features_interaction.strided = tmp_features_interaction_array[10];

        // Predict interaction
	    prediction_interaction = online_models_predict(om, &features_interaction);

        // Compute the predicted kernel interaction
        tmp_kernel_interaction = (prediction_interaction.time - prediction_alone.time) / prediction_alone.time;

        // TODO: remove, debug. Check if the scheduler has taken a decition different that fifo
        if (tmp_kernel_interaction < min_kernel_interaction) num_decision_changes++;

        // Update the index of the kernel with the minimum kernel interaction
        min_kernel_interaction_index = (tmp_kernel_interaction < min_kernel_interaction) ? tmp_kernel_index : min_kernel_interaction_index;

        // Update the minimum kernel interaction
        min_kernel_interaction = (tmp_kernel_interaction < min_kernel_interaction) ? tmp_kernel_interaction : min_kernel_interaction;

        // Increment the number of kernels checked
        num_kernels_checked++;
    }

    no_more_kernels:  // Label to go out of the nested loop when no more kernels are available

    // Return -1 if no kernel was found
    if (min_kernel_interaction_index < 0) return -1;

    // Dequeue the kernel with the least interference
    if(dequeue_from(q, min_kernel_interaction_index, d) < 0){
        print_error("Error dequeuing kernel from queue when scheduling\n");
        exit(1);
    }

    // TODO: remove, debug. Indicate the number of times the scheduler takes a decision different than fifo
    if (num_decision_changes == 1)
        num_first_kernel_scheduled++;
    else
        num_other_kernel_scheduled++;

    return 0;
}


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
int schedule_sjf_from_n_executable_kernels(queue *q, const int free_slots, const int *duplicated_kernels, kernel_data *d, const online_models_t *om, const int num_kernels_to_check, const float user_cpu, const float kernel_cpu, const float idle_cpu) {

    // Check if the queue is empty
    if(q->head == NULL) return -1;

    // Save the first node
    node *prev = NULL;
    node *tmp = q->head;

    // Scheduling variables
    float min_kernel_time = __FLT_MAX__;  // Stores the mininum predicted kernel interaction
    int min_kernel_time_index = -1;       // Stores the index of the kernel with the minimum predicted kernel interaction
    float tmp_kernel_time = 0;            // Temporal kernel interaction variabl
    int tmp_kernel_index = 0;                    // Temporal kernel index
    int num_kernels_checked = 0;                 // Number of kernels checked

    int num_decision_changes = 0;
    static int num_first_kernel_scheduled = 0;
    static int num_other_kernel_scheduled = 0;

    // Online models variables
	online_models_features_t features_interaction;
	online_models_prediction_t prediction_interaction;

    int tmp_features_interaction_array[TYPES_OF_KERNELS];

    // Special case for HEAD
    if((tmp->data.cu <= free_slots) && (duplicated_kernels[tmp->data.kernel_label] == 0)) {

        // TODO: encapsulate this in a function
        // Prepare the features (TODO: change data structure to avoid this)

        // Features interaction
        memcpy(tmp_features_interaction_array, duplicated_kernels, sizeof(tmp_features_interaction_array));
        tmp_features_interaction_array[tmp->data.kernel_label] = tmp->data.cu;

        features_interaction.user = user_cpu;
        features_interaction.kernel = kernel_cpu;
        features_interaction.idle = idle_cpu;
        features_interaction.main = tmp->data.kernel_label;
        features_interaction.aes = tmp_features_interaction_array[0];
        features_interaction.bulk = tmp_features_interaction_array[1];
        features_interaction.crs = tmp_features_interaction_array[2];
        features_interaction.kmp = tmp_features_interaction_array[3];
        features_interaction.knn = tmp_features_interaction_array[4];
        features_interaction.merge = tmp_features_interaction_array[5];
        features_interaction.nw = tmp_features_interaction_array[6];
        features_interaction.queue = tmp_features_interaction_array[7];
        features_interaction.stencil2d = tmp_features_interaction_array[8];
        features_interaction.stencil3d = tmp_features_interaction_array[9];
        features_interaction.strided = tmp_features_interaction_array[10];

        // Predict interaction
	    prediction_interaction = online_models_predict(om, &features_interaction);

        // TODO: remove, debug. Check if the scheduler has taken a decition different that fifo
        num_decision_changes++;


        // Compute the predicted kernel interaction
        min_kernel_time = prediction_interaction.time * tmp->data.num_executions;

        // Set the kernel index
        min_kernel_time_index = 0;

        // Increment the number of kernels checked
        num_kernels_checked++;
    }

    // Loop until we run out of kernels in the queue or we have checked the desired number of kernels
    while(num_kernels_checked < num_kernels_to_check) {

        // Rest of the cases
        do {

            // Save previous node
            prev = tmp;
            // Check if exist a node at that position
            // TODO: Remove the goto by spliting this function in multiple and return when this condition happen
            // TODO: This is a temporal solution to go out of the nested loop
            if(prev->next == NULL){
                goto no_more_kernels;
            }
            // Get next node
            tmp = prev->next;
            // tmp_index
            tmp_kernel_index++;
            // printf("[SCHED] Next kernel will be # kernel: %d\n", tmp_kernel_index);

        // }while( (tmp->data.cu > free_slots) || (duplicated_kernels[tmp->data.kernel_label] == 1) );
        }while( (tmp->data.cu > free_slots) || (duplicated_kernels[tmp->data.kernel_label] > 0) );

        // TODO: encapsulate this in a function
        // Prepare the features (TODO: change data structure to avoid this)

        // Features interaction
        memcpy(tmp_features_interaction_array, duplicated_kernels, sizeof(tmp_features_interaction_array));
        tmp_features_interaction_array[tmp->data.kernel_label] = tmp->data.cu;

        features_interaction.user = user_cpu;
        features_interaction.kernel = kernel_cpu;
        features_interaction.idle = idle_cpu;
        features_interaction.main = tmp->data.kernel_label;
        features_interaction.aes = tmp_features_interaction_array[0];
        features_interaction.bulk = tmp_features_interaction_array[1];
        features_interaction.crs = tmp_features_interaction_array[2];
        features_interaction.kmp = tmp_features_interaction_array[3];
        features_interaction.knn = tmp_features_interaction_array[4];
        features_interaction.merge = tmp_features_interaction_array[5];
        features_interaction.nw = tmp_features_interaction_array[6];
        features_interaction.queue = tmp_features_interaction_array[7];
        features_interaction.stencil2d = tmp_features_interaction_array[8];
        features_interaction.stencil3d = tmp_features_interaction_array[9];
        features_interaction.strided = tmp_features_interaction_array[10];

        // Predict interaction
	    prediction_interaction = online_models_predict(om, &features_interaction);

        // Compute the predicted kernel interaction
        tmp_kernel_time = prediction_interaction.time * tmp->data.num_executions;


        // TODO: remove, debug. Check if the scheduler has taken a decition different that fifo
        if (tmp_kernel_time < min_kernel_time) num_decision_changes++;

        // Update the index of the kernel with the minimum kernel interaction
        min_kernel_time_index = (tmp_kernel_time < min_kernel_time) ? tmp_kernel_index : min_kernel_time_index;

        // Update the minimum kernel interaction
        min_kernel_time = (tmp_kernel_time < min_kernel_time) ? tmp_kernel_time : min_kernel_time;

        // Increment the number of kernels checked
        num_kernels_checked++;
    }

    no_more_kernels:  // Label to go out of the nested loop when no more kernels are available

    // Return -1 if no kernel was found
    if (min_kernel_time_index < 0) return -1;

    // Dequeue the kernel with the least interference
    if(dequeue_from(q, min_kernel_time_index, d) < 0){
        print_error("Error dequeuing kernel from queue when scheduling\n");
        exit(1);
    }

    // TODO: remove, debug. Indicate the number of times the scheduler takes a decision different than fifo
    if (num_decision_changes == 1)
        num_first_kernel_scheduled++;
    else
        num_other_kernel_scheduled++;

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