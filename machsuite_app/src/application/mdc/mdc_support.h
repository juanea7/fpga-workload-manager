/**
 * @file mdc_support.h
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains a collection of functions for running MDC
 * @date November 2024
 *
 */

#ifndef _MDC_SUPPORT_H_
#define _MDC_SUPPORT_H_

/* AES accelerator */
#define OFFS_AES_IP 	0xA0010000
#define SLV_REG0	0x00000000
#define OUT_SIZE_SHIFT	9
#define OUT_SIZE 4	//128 bits (output size) divided by 32 bit (stream word size)  (multiple of 4 -> change execution time)

#define TEXT_SIZE_BYTE 16
#define KEY_SIZE_BYTE 16
#define ENCRYPTED_SIZE_BYTE 16

/* Customizable buffer for input and output data */
#define TEXT_BUFFER	0x0E000000
#define KEY_BUFFER	0x0E010000
#define ENCRYPTED_BUFFER	0x0F000000

/*  Open DDR memory */
int ddr_memory;

/*  Map PL registers */
unsigned int *aes_ip_virtual_addr;
unsigned int *dma_TEXT_virtual_addr;
unsigned int *dma_KEY_virtual_addr;
unsigned int *dma_ENCRYPTED_virtual_addr;

/* Map data buffers */
unsigned int *virtual_src_TEXT_addr;
unsigned int *virtual_src_KEY_addr;
unsigned int *virtual_dst_ENCRYPTED_addr;

/**
 * @brief Initialize MDC
 *
 */
void mdc_setup(void);

/**
 * @brief Clean MDC
 *
 */
void mdc_cleanup(void);

/**
 * @brief MDC AES Execution
 *
 */
void mdc_aes(void);

#endif /* _MDC_SUPPORT_H_*/