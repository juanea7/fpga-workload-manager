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
#include "data_structures.h"
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
	printf("MAIN:      %d\n", omf->main);
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
 * @brief Prints the scheduling decision
 *
 * @param omsd Scheduling decision structure
 */
void online_models_print_decision(const online_models_schedule_decision_t *omsd) {
	printf("Scheduling Decision{\n");
	printf("AES:       %d\n", omsd->aes);
	printf("BULK:      %d\n", omsd->bulk);
	printf("CRS:       %d\n", omsd->crs);
	printf("KMP:       %d\n", omsd->kmp);
	printf("KNN:       %d\n", omsd->knn);
	printf("MERGE:     %d\n", omsd->merge);
	printf("NW:        %d\n", omsd->nw);
	printf("QUEUE:     %d\n", omsd->queue);
	printf("STENCIL2D: %d\n", omsd->stencil2d);
	printf("STENCIL3D: %d\n", omsd->stencil3d);
	printf("STRIDED:   %d\n", omsd->strided);
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
 * @brief Adds a kernel label to the scheduling request
 *
 * @param schedule_request Pointer to the scheduling request
 * @param kernel_label Kernel label to be added
 * @return (int) 0 on success, error code otherwise
 */
int add_kernel_label_to_scheduling_request(online_models_features_t *schedule_request, const int kernel_label) {

        switch (kernel_label)
        {
        case AES:
            schedule_request->aes = 0xFF;
            break;
        case BULK:
            schedule_request->bulk = 0xFF;
            break;
        case CRS:
            schedule_request->crs = 0xFF;
            break;
        case KMP:
            schedule_request->kmp = 0xFF;
            break;
        case KNN:
            schedule_request->knn = 0xFF;
            break;
        case MERGE:
            schedule_request->merge = 0xFF;
            break;
        case NW:
            schedule_request->nw = 0xFF;
            break;
        case QUEUE:
            schedule_request->queue = 0xFF;
            break;
        case STENCIL2D:
            schedule_request->stencil2d = 0xFF;
            break;
        case STENCIL3D:
            schedule_request->stencil3d = 0xFF;
            break;
        case STRIDED:
            schedule_request->strided = 0xFF;
            break;
        default:
            break;
        }

		return 0;
}


/**
 * @brief Gets the CUs of a kernel from the scheduling decision
 *
 * @param schedule_decision Pointer to the scheduling decision
 * @param kernel_label Kernel label to get the CUs from
 * @return (int) CUs of the kernel
 */
int get_kernel_from_scheduling_decision(online_models_schedule_decision_t *schedule_decision, const int kernel_label) {

		int kernel_cu = 0;

        switch (kernel_label)
        {
        case AES:
            kernel_cu = schedule_decision->aes;
            break;
        case BULK:
            kernel_cu = schedule_decision->bulk;
            break;
        case CRS:
            kernel_cu = schedule_decision->crs;
            break;
        case KMP:
            kernel_cu = schedule_decision->kmp;
            break;
        case KNN:
            kernel_cu = schedule_decision->knn;
            break;
        case MERGE:
            kernel_cu = schedule_decision->merge;
            break;
        case NW:
            kernel_cu = schedule_decision->nw;
            break;
        case QUEUE:
            kernel_cu = schedule_decision->queue;
            break;
        case STENCIL2D:
            kernel_cu = schedule_decision->stencil2d;
            break;
        case STENCIL3D:
            kernel_cu = schedule_decision->stencil3d;
            break;
        case STRIDED:
            kernel_cu = schedule_decision->strided;
            break;
        default:
            break;
        }

		return kernel_cu;
}


/**
 * @brief Asks for a CSA training process to the online models on the external python code via the
 *        TCP training socket
 *
 * @param om Pointer to the online models structure
 * @param schedule_request Pointer to the schedule request to be sent to the python code
 * @return (online_models_schedule_decision_t) Scheduling decision made by the online models
 */
online_models_schedule_decision_t online_models_schedule(const online_models_t *om, const online_models_features_t *schedule_request) {

    int ret;

	online_models_schedule_decision_t decision;

	// printf("[Models Operation] scheduling decision\n");
	// online_models_print_features(schedule_request);

    // The python code can tell if it is a scheduling request by checking whether Main == 0xFF
    ret = send_data_to_socket_tcp(om->prediction_socket_fd, schedule_request, sizeof(*schedule_request));
	if(ret < 0) {
		print_error("Error TCP prediction socket request schedule\n");
		exit(1);
	}

    ret = recv_data_from_socket_tcp(om->prediction_socket_fd, &decision, sizeof(decision));
	if(ret < 0) {
		print_error("Error TCP prediction socket receive decision\n");
		exit(1);
	}

	// DEBUG
	// printf("[Models Operation] scheduling decision\n");
	// online_models_print_decision(&decision);
	print_debug("The TCP training socket has successfully notified python to schedule\n");

    return decision;
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