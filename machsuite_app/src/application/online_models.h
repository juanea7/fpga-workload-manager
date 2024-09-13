/**
 * @file online_models.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-03-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef _ONLINE_MODELING_H_
#define _ONLINE_MODELING_H_

#include <sys/un.h>
#include <stdint.h>

#include "debug.h"


typedef struct online_models_features_t{
    float user;
    float kernel;
    float idle;
    uint8_t aes;
    uint8_t bulk;
    uint8_t crs;
    uint8_t kmp;
    uint8_t knn;
    uint8_t merge;
    uint8_t nw;
    uint8_t queue;
    uint8_t stencil2d;
    uint8_t stencil3d;
    uint8_t strided;
}online_models_features_t;

typedef struct online_models_prediction_t{
    #if BOARD == ZCU
    float ps_power;
    float pl_power;
    #elif BOARD == PYNQ
    float power;
    #endif
    float time;
}online_models_prediction_t;

typedef struct online_models_metrics_t{
    #if BOARD == ZCU
    float ps_power_error;
    float pl_power_error;
    #elif BOARD == PYNQ
    float power_error;
    #endif
    float time_error;
}online_models_metrics_t;

/**
 * @brief
 *
 */
typedef struct online_models_t{
    int training_socket_fd;                       // Training socket file descriptor
    struct sockaddr_un training_socket_name;      // Training socket structure
    int prediction_socket_fd;                     // Prediction socket file descriptor
    struct sockaddr_un prediction_socket_name;    // Prediction socket structure
}online_models_t;

/**
 * @brief Prints the features of a particular observation
 *
 * @param omf Features structure
 */
void online_models_print_features(const online_models_features_t *omf);

/**
 * @brief Prints the prediction for a particular observation
 *
 * @param omp Prediction structure
 */
void online_models_print_prediction(const online_models_prediction_t *omp);

/**
 * @brief Creates the UDP and TCP client Unix-domain sockets for training
 *        and predicting with the online models implemented in the external
 *        python code
 *
 * @param om Pointer to the online models structure
 * @param num_measurements Indicate the number of measurements within a training stage
 * @return (int) 0 on success, error code otherwise
 */
int online_models_setup(online_models_t *om, const unsigned int num_measurements) ;

/**
 * @brief Destroys the TCP client Unix-domain sockets for training
 *        and predicting with the online models implemented in the external
 *        python code
 *
 * @param om Pointer to the online models structure
 * @return (int) 0 on success, error code otherwise
 */
int online_models_clean(const online_models_t *om);


/**
 * @brief Commands a training/test process (is the python's decision) to the online models on the
 * 		  external python code via the TCP training socket
 *
 * @param om Pointer to the online models structure
 * @param num_measurements Number of measurements to be trained
 * @param obs_to_wait Pointer to store the received observations to wait in idle from the models
 * @return (int) 0 on success, error code otherwise
 */
int online_models_operation(const online_models_t *om, const unsigned int num_measurements, int *obs_to_wait);

/**
 * @brief Commands a training process to the online models on the external
 *        python code via the TCP training socket
 *
 * @param om Pointer to the online models structure
 * @param num_measurements Number of measurements to be trained
 * @param metrics Pointer to store the received metrics from the models
 * @return (int) 0 on success, error code otherwise
 */
int online_models_train(const online_models_t *om, const unsigned int num_measurements, online_models_metrics_t *metrics);

/**
 * @brief Commands a testing process to the online models on the external
 *        python code via the TCP training socket
 *
 * @param om Pointer to the online models structure
 * @param num_measurements Number of measurements to be trained
 * @param metrics Pointer to store the received metrics from the models
 * @return (int) 0 on success, error code otherwise
 */
int online_models_test(const online_models_t *om, const unsigned int num_measurements, online_models_metrics_t *metrics);

/**
 * @brief Commands an inference process to the online models on the external
 *        python code via the UDP training socket
 *
 * @param om Pointer to the online models structure
 * @param features Pointer to the features of the observation to be predicted
 * @return (online_models_prediction_t) Prediction made by the online models
 */
online_models_prediction_t online_models_predict(const online_models_t *om, const online_models_features_t *features);

#endif /*_ONLINE_MODELING_H_*/