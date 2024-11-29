/**
 * @file mdc_support.c
 * @author Juan Encinas (juan.encinas@upm.es)
 * @brief This file contains a collection of functions for running MDC
 * @date November 2024
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/mman.h>
#include <stdlib.h>

#include "../debug.h"
#include "dma_simplemode.h"

#include "mdc_support.h"


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
void mdc_setup(void) {

	print_debug("\nMDC Setup...\n");

	#if MDC

	/*  Configure the PL */
	system("fpgautil -b design_1_wrapper.bin");

	/*  Open DDR memory */
	ddr_memory = open("/dev/mem", O_RDWR | O_SYNC);
	print_debug(" DDR memory opened\n");

	/*  Map PL registers */
	aes_ip_virtual_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, OFFS_AES_IP);
	dma_TEXT_virtual_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, OFFS_DMA_TEXT_DATA);
	dma_KEY_virtual_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, OFFS_DMA_KEY_DATA);
	dma_ENCRYPTED_virtual_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, OFFS_DMA_ENCRYPT_DATA);
	print_debug(" PL registers mapped\n");

	/* Map data buffers */
	virtual_src_TEXT_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, TEXT_BUFFER);
	virtual_src_KEY_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, KEY_BUFFER);
	virtual_dst_ENCRYPTED_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, ddr_memory, ENCRYPTED_BUFFER);
	print_debug(" Data buffers mapped\n");

	#endif

}

/**
 * @brief Clean MDC
 *
 */
void mdc_cleanup(void) {

	print_debug("\nCleaning MDC...\n");

	#if MDC

	// unmapping
	munmap(dma_TEXT_virtual_addr, 65535);
	munmap(dma_KEY_virtual_addr, 65535);
	munmap(dma_ENCRYPTED_virtual_addr, 65535);
	munmap(aes_ip_virtual_addr, 65535);
	munmap(virtual_src_TEXT_addr, 65535);
	munmap(virtual_src_KEY_addr, 65535);
	munmap(virtual_dst_ENCRYPTED_addr, 65535);

	// close /dev/mem
	close(ddr_memory);

	#endif
}

/**
 * @brief MDC AES Execution
 *
 */
void mdc_aes(void) {

	print_debug("\nAES Execution...\n");

	#if MDC

	/* Init data buffers */
	/* text data FFEEDDCCBBAA99887766554433221100 */
	unsigned int j = 0x00000000;
	for (int i = 0; i < 16; ++i)
	{
		virtual_src_TEXT_addr[i] = j;
		j = j + 0x11;
	}

	j = 0x00000000;
	/* key data F0E0D0C0B0A09080706050403020100 */
	for (int i = 0; i < 32; ++i)
	{
		virtual_src_KEY_addr[i] = j;
		j = j + 0x1;
	}
	print_debug(" Data buffers initialized\n");

	memset(virtual_dst_ENCRYPTED_addr, 0, 16 * 4);

	/* Configure the accelerator */
	write_dma(aes_ip_virtual_addr, SLV_REG0, (OUT_SIZE << OUT_SIZE_SHIFT));

	/*  Reset the DMAs */
	write_dma(dma_TEXT_virtual_addr, MM2S_CONTROL_REGISTER, RESET_DMA);
	write_dma(dma_KEY_virtual_addr, MM2S_CONTROL_REGISTER, RESET_DMA);
	write_dma(dma_ENCRYPTED_virtual_addr, S2MM_CONTROL_REGISTER, RESET_DMA);

	/* Halt DMAs */
	write_dma(dma_TEXT_virtual_addr, MM2S_CONTROL_REGISTER, HALT_DMA);
	write_dma(dma_KEY_virtual_addr, MM2S_CONTROL_REGISTER, HALT_DMA);
	write_dma(dma_ENCRYPTED_virtual_addr, S2MM_CONTROL_REGISTER, HALT_DMA);

	/* Enable interrupts */
	write_dma(dma_TEXT_virtual_addr, MM2S_CONTROL_REGISTER, ENABLE_ALL_IRQ);
	write_dma(dma_KEY_virtual_addr, MM2S_CONTROL_REGISTER, ENABLE_ALL_IRQ);
	write_dma(dma_ENCRYPTED_virtual_addr, S2MM_CONTROL_REGISTER, ENABLE_ALL_IRQ);

	/*  Write the SOURCE/DESTINATION ADDRESS registers */
	write_dma(dma_TEXT_virtual_addr, MM2S_SRC_ADDRESS_REGISTER, TEXT_BUFFER);
	write_dma(dma_KEY_virtual_addr, MM2S_SRC_ADDRESS_REGISTER, KEY_BUFFER);
	write_dma(dma_ENCRYPTED_virtual_addr, S2MM_DST_ADDRESS_REGISTER, ENCRYPTED_BUFFER);

	/*  Run DMAs */
	write_dma(dma_KEY_virtual_addr, MM2S_CONTROL_REGISTER, RUN_DMA);
	write_dma(dma_TEXT_virtual_addr, MM2S_CONTROL_REGISTER, RUN_DMA);
	write_dma(dma_ENCRYPTED_virtual_addr, S2MM_CONTROL_REGISTER, RUN_DMA);

	/*  Write the LENGTH of the transfer
	 *  Transfer will start right after this register is written
	 */
	print_debug(" DMA transfer started\n");

	write_dma(dma_TEXT_virtual_addr, MM2S_TRNSFR_LENGTH_REGISTER, TEXT_SIZE_BYTE);
	write_dma(dma_KEY_virtual_addr, MM2S_TRNSFR_LENGTH_REGISTER, KEY_SIZE_BYTE);
	write_dma(dma_ENCRYPTED_virtual_addr, S2MM_BUFF_LENGTH_REGISTER, ENCRYPTED_SIZE_BYTE);

	/* Wait for transfers completion */
	dma_mm2s_sync(dma_TEXT_virtual_addr);
	dma_mm2s_sync(dma_KEY_virtual_addr);
	dma_s2mm_sync(dma_ENCRYPTED_virtual_addr);

	#endif
}
