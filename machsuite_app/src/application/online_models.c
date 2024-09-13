/**
 * @file online_models.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions that handle the sockets used for
 *        online training and infering of the power and performance models
 *        implemented in a external python module.
 * @date March 2023
 *
 */

#include <stdlib.h>

#include "online_models.h"

#include "debug.h"
#include "client_socket_udp.h"
#include "client_socket_tcp.h"

// UDP training socket path
#define TRAINING_SOCKET_NAME "/tmp/my_training_socket"
// Command indicating the python code to perform a training operation on the models
#define TRAIN_SIGNAL "1"
// Command indicating the python code the training stage has finished
#define END_TRAINING_SIGNAL 0

// TCP prediction socket path
#define PREDICTION_SOCKET_NAME "/tmp/my_prediction_socket"
// Command indicating the python code the prediction stage has finished
#define END_PREDICTING_SIGNAL "0"

// Python training and predicting threads command size
// Note: The size has to be just 1 and not sizeof(command) because,
//        for some reason, the tcp socket receives a b'0\x00' instead
//        of b'0' when using sizeof(command).
#define COMMAND_SIZE 1


/**
 * @brief Prints the error metrics of the models
 *
 * @param omm Metrics structure
 */
static void _online_models_print_metrics(const online_models_metrics_t *omm){
	#if BOARD == ZCU
		printf("Metrics {PS Power: %.6f, PL Power: %.6f, Time: %.6f}\n", omm->ps_power_error, omm->pl_power_error, omm->time_error);
	#elif BOARD == PYNQ
		printf("Metrics {Power: %.6f, Time: %.6f}\n", omm->power_error, omm->time_error);
	#endif
}
/**
 * @brief Prints the features of a particular observation
 *
 * @param omf Features structure
 */
void online_models_print_features(const online_models_features_t *omf) {
	printf("Features{\n");
	printf("USER:      %f\n", omf->user);
	printf("KERNEL:    %f\n", omf->kernel);
	printf("IDLE:      %f\n", omf->idle);
	printf("AES:       %d\n", omf->aes);
	printf("BULK:      %d\n", omf->bulk);
	printf("CRS:       %d\n", omf->crs);
	printf("KMP:       %d\n", omf->kmp);
	printf("KNN:       %d\n", omf->knn);
	printf("MERGE:     %d\n", omf->merge);
	printf("NW:        %d\n", omf->nw);
	printf("QUEUE:     %d\n", omf->queue);
	printf("STENCIL2D: %d\n", omf->stencil2d);
	printf("STENCIL3D: %d\n", omf->stencil3d);
	printf("STRIDED:   %d\n", omf->strided);
	printf("}\n\n");
}

/**
 * @brief Prints the prediction for a particular observation
 *
 * @param omp Prediction structure
 */
void online_models_print_prediction(const online_models_prediction_t *omp) {
	printf("Prediction{\n");
	#if BOARD == ZCU
	printf("PS Power: %f\n", omp->ps_power);
	printf("PL Power: %f\n", omp->pl_power);
	#elif BOARD == PYNQ
	printf("Power: %f\n", omp->power);
	#endif
	printf("Time:     %f\n", omp->time);
	printf("}\n\n");
}

/**
 * @brief Creates the UDP and TCP client Unix-domain sockets for training
 *        and predicting with the online models implemented in the external
 *        python code
 *
 * @param om Pointer to the online models structure
 * @param num_measurements Indicate the number of measurements within a training stage
 * @return (int) 0 on success, error code otherwise
 */
int online_models_setup(online_models_t *om, const unsigned int num_measurements) {

	int ret, ack;

    // Create the UDP socket to signal the python code that there are traces ready
    om->training_socket_fd = create_socket_tcp_unix(&(om->training_socket_name), TRAINING_SOCKET_NAME);
    if(om->training_socket_fd < 0) {
        print_error("Error TCP training socket creation\n");
        exit(1);
    }
	print_debug("The TCP training socket has been successfully created\n");

	// When the traces are shared via Shared Memory we have to tell the python program
	// how many iterations are in a training stage for apropriatelly dimensioning the buffer
	#if TRACES_RAM
		// Indicate the python program how many measurements are going to be sent
		ret = send_data_to_socket_tcp(om->training_socket_fd, &num_measurements, sizeof(num_measurements));
		if(ret < 0) {
			print_error("Error TCP training socket send num measurements\n");
			exit(1);
		}
		ret = recv_data_from_socket_tcp(om->training_socket_fd, &ack, sizeof(ack));
		if(ret < 0) {
			print_error("Error TCP training socket recv ack\n");
			exit(1);
		}
		printf("ACK num measurements python: %d\n", ack);
		print_debug("The TCP training socket has successfully notified python\n");
	#endif

    // Create the TCP socket to signal the python code to predict
    om->prediction_socket_fd = create_socket_tcp_unix(&(om->prediction_socket_name), PREDICTION_SOCKET_NAME);
    if(om->prediction_socket_fd < 0) {
        print_error("Error TCP prediction socket creation\n");
        exit(1);
    }
	print_debug("The TCP prediction socket has been successfully created\n");

    return 0;
}


/**
 * @brief Destroys the client Unix-domain sockets for training
 *        and predicting with the online models implemented in the external
 *        python code
 *
 * @param om Pointer to the online models structure
 * @return (int) 0 on success, error code otherwise
 */
int online_models_clean(const online_models_t *om) {

    int ret;
	unsigned int end_training = 0;

	// Signal the python code that the training has finished
	//ret = send_data_to_socket_udp(om->training_socket_fd, END_TRAINING_SIGNAL, COMMAND_SIZE, om->training_socket_name);
	ret = send_data_to_socket_tcp(om->training_socket_fd, &end_training, sizeof(end_training));
	if(ret < 0) {
		print_error("Error TCP training socket send init\n");
		exit(1);
	}

	// Close the UDP training socket
    close_socket_tcp(om->training_socket_fd);
	print_debug("The TCP training socket has been successfully close\n");

	// test
	{
	online_models_features_t features_test = {58.08,33.33,8.59,2,0,0,1,0,1,0,1,4,0,0};
	online_models_prediction_t prediction_test;
	prediction_test = online_models_predict(om, &features_test);
	online_models_print_features(&features_test);
	online_models_print_prediction(&prediction_test);
	}

    // Clean prediction TCP socket
    ret = send_data_to_socket_tcp(om->prediction_socket_fd, END_PREDICTING_SIGNAL, COMMAND_SIZE);
	if(ret < 0) {
		print_error("Error TCP prediction socket send init\n");
		exit(1);
	}

	// Close the TCP prediction socket
    close_socket_tcp(om->prediction_socket_fd);
	print_debug("The TCP prediction socket has been successfully close\n");

    return 0;
}


/**
 * @brief Commands a training/test process (is the python's decision) to the online models on the
 * 		  external python code via the TCP training socket
 *
 * @param om Pointer to the online models structure
 * @param num_measurements Number of measurements to be trained
 * @param obs_to_wait Pointer to store the received observations to wait in idle from the models
 * @return (int) 0 on success, error code otherwise
 */
int online_models_operation(const online_models_t *om, const unsigned int num_measurements, int *obs_to_wait) {

    int ret;

    // Signal the python code that there is new traces to be processed
	ret = send_data_to_socket_tcp(om->training_socket_fd, &num_measurements, sizeof(num_measurements));
	if(ret < 0) {
		print_error("Error TCP training socket send operation\n");
		exit(1);
	}
    ret = recv_data_from_socket_tcp(om->training_socket_fd, obs_to_wait, sizeof(*obs_to_wait));
	if(ret < 0) {
		print_error("Error TCP training socket recv ack\n");
		exit(1);
	}
	// DEBUG
	printf("[Models Operation] obs_to_wait: %d\n", *obs_to_wait);
	print_debug("The TCP training socket has successfully notified python to train/test\n");

    return 0;
}


/**
 * @brief Commands a training process to the online models on the external
 *        python code via the TCP training socket
 *
 * @param om Pointer to the online models structure
 * @param num_measurements Number of measurements to be trained
 * @param metrics Pointer to store the received metrics from the models
 * @return (int) 0 on success, error code otherwise
 */
int online_models_train(const online_models_t *om, const unsigned int num_measurements, online_models_metrics_t *metrics) {

    int ret;
	unsigned int command;

	// Indicate we want to train by setting the MSB
	command = num_measurements | ((unsigned int)1 << 31);

    // Signal the python code that there is new traces to be processed
	ret = send_data_to_socket_tcp(om->training_socket_fd, &command, sizeof(command));
	if(ret < 0) {
		print_error("Error TCP training socket send train\n");
		exit(1);
	}
    ret = recv_data_from_socket_tcp(om->training_socket_fd, metrics, sizeof(*metrics));
	if(ret < 0) {
		print_error("Error TCP training socket recv ack\n");
		exit(1);
	}
	// DEBUG
	printf("Training ");
	_online_models_print_metrics(metrics);
	print_debug("The TCP training socket has successfully notified python to train\n");

    return 0;
}


/**
 * @brief Commands a testing process to the online models on the external
 *        python code via the TCP training socket
 *
 * @param om Pointer to the online models structure
 * @param num_measurements Number of measurements to be trained
 * @param metrics Pointer to store the received metrics from the models
 * @return (int) 0 on success, error code otherwise
 */
int online_models_test(const online_models_t *om, const unsigned int num_measurements, online_models_metrics_t *metrics) {

    int ret;
	unsigned int command;

	// Indicate we want to test by clearing the MSB
	command = num_measurements & ~((unsigned int)1 << 31);

    // Signal the python code that there is new traces to be processed
	ret = send_data_to_socket_tcp(om->training_socket_fd, &command, sizeof(command));
	if(ret < 0) {
		print_error("Error TCP training socket send test\n");
		exit(1);
	}
    ret = recv_data_from_socket_tcp(om->training_socket_fd, metrics, sizeof(*metrics));
	if(ret < 0) {
		print_error("Error TCP training socket recv ack\n");
		exit(1);
	}
	// DEBUG
	printf("Testing ");
	_online_models_print_metrics(metrics);
	print_debug("The TCP training socket has successfully notified python to test\n");

    return 0;
}

/**
 * @brief Commands an inference process to the online models on the external
 *        python code via the UDP training socket
 *
 * @param om Pointer to the online models structure
 * @param features Pointer to the features of the observation to be predicted
 * @return (online_models_prediction_t) Prediction made by the online models
 */
online_models_prediction_t online_models_predict(const online_models_t *om, const online_models_features_t *features) {

    int ret;

    online_models_prediction_t result;

    ret = send_data_to_socket_tcp(om->prediction_socket_fd, features, sizeof(*features));
	if(ret < 0) {
		print_error("Error TCP prediction socket send predict\n");
		exit(1);
	}

    ret = recv_data_from_socket_tcp(om->prediction_socket_fd, &result, sizeof(result));
	if(ret < 0) {
		print_error("Error TCP prediction socket recv prediction\n");
		exit(1);
	}

    return result;
}

// TODO: certain communications, such as getting the actual models error metric
//       could handle by sending a particular command (e.g. b'5') via the tcp
//		 socket and writing the particular response in the python code.