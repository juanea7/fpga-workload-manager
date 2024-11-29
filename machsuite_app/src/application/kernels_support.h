/**
 * @file kernels_support.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions used to perform the configuration
 * 		  and execution of each kernel in the FPGA using ARTICo3
 * @date September 2022
 *
 */

#ifndef _KERNELS_SUPPORT_H_
#define _KERNELS_SUPPORT_H_

#include <pthread.h>

#include "queue_online.h"
#include "data_structures.h"


/* Kernel data handling fucntions */

/**
 * @brief Kernel input and reference data initialization
 *
 */
void kernel_init_data();

/**
 * @brief Kernel input and reference data clean-up
 *
 */
void kernel_clean_data();

/**
 * @brief Kernel input data copying
 *
 * @param io_data Pointer to store the copied input data in
 * @param kernel Kernel label enum
 */
void kernel_copy_input(char **io_data, kernel_label_t kernel);

/**
 * @brief Kernel result validation
 *
 * @param output_data Pointer to the output data
 * @param kernel Kernel label enum
 */
void kernel_result_validation(char **output_data, kernel_label_t kernel);

/* Kernel execution functions */

/**
 * @brief AES kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param aes_vargs Pointer to the kernel inputs and outputs buffers
 */
void aes_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *aes_vargs);

/**
 * @brief BULK kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param bulk_vargs Pointer to the kernel inputs and outputs buffers
 */
void bulk_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *bulk_vargs);

/**
 * @brief CRS kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param crs_vargs Pointer to the kernel inputs and outputs buffers
 */
void crs_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *crs_vargs);

/**
 * @brief KMP kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param kmp_vargs Pointer to the kernel inputs and outputs buffers
 */
void kmp_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *kmp_vargs);

/**
 * @brief KNN kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param knn_vargs Pointer to the kernel inputs and outputs buffers
 */
void knn_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *knn_vargs);

/**
 * @brief MERGE kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param merge_vargs Pointer to the kernel inputs and outputs buffers
 */
void merge_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *merge_vargs);

/**
 * @brief NW kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param nw_vargs Pointer to the kernel inputs and outputs buffers
 */
void nw_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *nw_vargs);

/**
 * @brief QUEUE kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param queue_vargs Pointer to the kernel inputs and outputs buffers
 */
void queue_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *queue_vargs);

/**
 * @brief STENCIL2D kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param stencil2d_vargs Pointer to the kernel inputs and outputs buffers
 */
void stencil2d_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *stencil2d_vargs);

/**
 * @brief STENCIL3D kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param stencil3d_vargs Pointer to the kernel inputs and outputs buffers
 */
void stencil3d_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *stencil3d_vargs);

/**
 * @brief STRIDED kernel configuration and execution in the FPGA via ARTICo3
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param strided_vargs Pointer to the kernel inputs and outputs buffers
 */
void strided_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *strided_vargs);


/**
 * @brief MDC AES kernel configuration and execution in the FPGA via MDC
 *
 * @param kernel kernel information
 * @param online_queue Array of online queues
 * @param online_queue_lock Array of online queues mutexes
 * @param aes_vargs Pointer to the kernel inputs and outputs buffers (retrocompatibility)
 */
void mdc_aes_execution(kernel_data *kernel, queue_online *online_queue, pthread_mutex_t *online_queue_lock, void *aes_vargs);

#endif /* _KERNELS_SUPPORT_H_*/