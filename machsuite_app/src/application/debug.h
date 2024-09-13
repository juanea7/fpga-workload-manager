/**
 * @file debug.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains defines used to enable/disable some functionalities
 *        such as the use of ARTICo3 or the Monitor as well as fine-tuning the
 *        output verbosity of the debugging print statements.
 * @date September 2022
 *
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>

// If not defined by command line, disable debug
#ifndef DEBUG
    #define DEBUG 0
#endif

#ifndef INFO
    #define INFO 0
#endif

#ifndef ARTICO
    #define ARTICO 1
#endif

#ifndef MONITOR
    #define MONITOR 1
#endif

#ifndef ONLINE_MODELS
    #define ONLINE_MODELS 0
#endif

#ifndef EXECUTION_MODES
    #define EXECUTION_MODES 0
#endif

#ifndef CPU_USAGE
    #define CPU_USAGE 0
#endif

#ifndef TRACES_RAM
    #define TRACES_RAM 0
#endif

#ifndef TRACES_ROM
    #define TRACES_ROM 0
#endif

#ifndef TRACES_SOCKET
    #define TRACES_SOCKET 1
#endif

// Set the board type
// TODO: This could be done using the macros defined by the toolchain in the compilation (much more elegant)
#define PYNQ 1
#define ZCU  2
#ifndef BOARD
    #define BOARD ZCU
#endif

// Debug messages
#if DEBUG
    #define print_debug(msg, args...) printf(msg, ##args)
    #define print_info(msg, args...) printf(msg, ##args)
#else
    #define print_debug(msg, args...)
    #if INFO
        #define print_info(msg, args...) printf(msg, ##args)
    #else
        #define print_info(msg, args...)
    #endif
#endif

// Error messages
#define print_error(msg, args...) printf(msg, ##args)

#endif /* _DEBUG_H_ */
