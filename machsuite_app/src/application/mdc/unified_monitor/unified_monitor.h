/*
 * UPM + UNICA Monitor
 *
 * Author      : Juan Encinas Anchústegui <juan.encinas@upm.es>
 * Date        : November 2024
 * Description : This file contains the unified UPM + UNICA Monitor runtime API, which can
 *               be used by any application to monitor the power consumption and performances
 *               of hardware accelerators.
 *
 */

#ifndef _UNIFIED_MONITOR_H_
#define _UNIFIED_MONITOR_H_

#include "upm_monitor/monitor.h"

/*
 * Unified Monitor init function
 *
 * This function initialized the UPM and UNICA monitoring infrastructure.
 *
 */
int unified_monitor_init();

/*
 * Unified Monitor start function
 *
 * This function starts the UPM and UNICA monitoring infrastructure.
 *
 */
int unified_monitor_start();

/*
 * Unified Monitor stop function
 *
 * This function stops the UPM and UNICA monitoring infrastructure.
 *
 */
int unified_monitor_stop();

/*
 * Unified Monitor read function
 *
 * This function reads the UPM and UNICA monitoring data.
 *
 */
int unified_monitor_read(int **power_reference, monitortdata_t **traces_reference, int *number_power_samples_reference, int *number_traces_samples_reference, int *elapsed_time_reference);

/*
 * Unified Monitor clean function
 *
 * This function cleans the UPM and UNICA monitoring infrastructure.
 *
 */
int unified_monitor_clean();

#endif /* _UNIFIED_MONITOR_H_ */