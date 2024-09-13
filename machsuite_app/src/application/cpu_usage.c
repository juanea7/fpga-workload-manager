/**
 * @file cpu_usage.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains the functions that calculate the cpu usage.
 * @date June 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cpu_usage.h"


/**
 * @brief Print CPU usage header
 * 
 * @param name Pointer to the structure describing the socket
 * @param path Path to the socket
 * @return (int) File descriptor of the socket on success, -1 otherwise
 */
void print_header() {
    printf("USER    NICE    SYS   IDLE   IOWAIT  IRQ  SOFTIRQ  STEAL\n");
}

/**
 * @brief Parse the /proc/stat "file" to gather CPU usage info
 * 
 * @param cpu_stats Array to return CPU usage data by reference
 */
void cpu_usage_parse(unsigned long long* cpu_stats) {
    
    // Open the virtual /proc/stat file
    FILE* file = fopen(CPU_USAGE_STAT_FILE, "r");
    if (file == NULL) {
        perror("Failed to open CPU stat file\n");
        exit(1);
    }

    // Get info from the file
    if (fscanf(file, "cpu %llu %*llu %llu %llu",
               &cpu_stats[0], &cpu_stats[1], &cpu_stats[2]) != CPU_USAGE_STAT_COLUMNS) {
        printf("Unexpected /proc/stat format\n");
        exit(1);
    }

    // Close the file
    fclose(file);
}

/**
 * @brief Calculate CPU usage and update previous usage
 * 
 * @param curr_stats Array of the latest CPU usage metrics 
 * @param prev_stats Array of the previous CPU usage metrics 
 * @param cpu_usage Array containing the calculate CPU usage ratio, returned by reference
 */
void calculate_and_update_cpu_usage(const unsigned long long* curr_stats, unsigned long long* prev_stats, float* cpu_usage) {

    unsigned long long total_time = 0;
    unsigned long long diff_stats[CPU_USAGE_STAT_COLUMNS];
    
    // Compute the difference of CPU jiffies within the time window
    // As well as update the previous stats
    for (int i = 0; i < CPU_USAGE_STAT_COLUMNS; i++) {
        diff_stats[i] = curr_stats[i] - prev_stats[i];
        prev_stats[i] = curr_stats[i];
        total_time += diff_stats[i];    
    }
    
    // Compute the percentual CPU usage
    for (int i = 0; i < CPU_USAGE_STAT_COLUMNS; i++) {
        cpu_usage[i] = ((float)diff_stats[i] * 100) / total_time;
    }   
}

/**
 * @brief Calculate CPU usage
 * 
 * @param curr_stats Array of the latest CPU usage metrics 
 * @param prev_stats Array of the previous CPU usage metrics 
 * @param cpu_usage Array containing the calculate CPU usage ratio, returned by reference
 */
void calculate_cpu_usage(const unsigned long long* curr_stats, const unsigned long long* prev_stats, float* cpu_usage) {

    unsigned long long total_time = 0;
    unsigned long long diff_stats[CPU_USAGE_STAT_COLUMNS];
    
    // Compute the difference of CPU jiffies within the time window
    for (int i = 0; i < CPU_USAGE_STAT_COLUMNS; i++) {
        diff_stats[i] = curr_stats[i] - prev_stats[i];
        total_time += diff_stats[i];    
    }
    
    // Compute the percentual CPU usage
    for (int i = 0; i < CPU_USAGE_STAT_COLUMNS; i++) {
        cpu_usage[i] = ((float)diff_stats[i] * 100) / total_time;
    }   
}
