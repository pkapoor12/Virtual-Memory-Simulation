// Starting code version v1.0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mmu.h"
#include "memsim.h"
#include "pagetable.h"
#include "input.h"

/* Private Internals: */

// Swapping functionality
#define DISK_SWAP_FILE_PATH ((const char*) "./disk.bin")
FILE* swapFileHandle;

/* Open the file to be used as swap space. */
void MMUOpenSwapFile() {
	swapFileHandle = fopen(DISK_SWAP_FILE_PATH,"wb+");
}

void MMUInit() {
	Memsim_Init(); // Set up simulated physical memory system.
	MMUOpenSwapFile(); // Open swap file for use.
	PT_Init(); // Set up page table register value storage per process.
}

int MMUStart() {
	char* line;
	while (TRUE) { // continue to read input until EOF
		if (Input_GetLine(&line) < 1) { // allocate memory for each line
			printf("End of File.\n");
			return 0;
		} else {
			Input_NextInstruction(line);
		}
		free(line);
	}
}

/*
 * Public Interface:
 */

FILE* MMU_GetSwapFileHandle() {
	return swapFileHandle;
}

/*
 * The most helpful function of an MMU. 
 * Translates the VPN to find the correct physical page, then adds the offset value.
 * Our logic is in software as early ones were, but the MMU is now built as hardware accelration.
 * 
 * Traslates the VPN to find the correct physical page, then adds the offset value
 * to find the exact location of the memory reference. If the page is not mapped, return -1.
*/
int MMU_TranslateAddress(int process_id, int VPN, int offset){
	int page;
	if((page = PT_VPNtoPA(process_id, VPN)) != -1){
		return page + offset;
	} else {
		return -1;
	}
}

/* 

 * Flush the file to disk, and then flush the disk to ensure the data is written.

 * Seeks over the file to the end, and then flushes the file to disk.

 * Returns -1 if there was an error, and 0 if the seek was successful.

 */

int Disk_Flush(int fd, FILE* swapFile) {

	fflush(swapFile);

	off_t end_offset = lseek(fd, 0, SEEK_END);

	if(end_offset == -1){

		printf("Error on disk. Could not write to disk.\n");

		return -1;

	}

	fflush(swapFile);

	return 0;

}

/*

 * Write a PAGE_SIZE worth of bytes from the physical memory starting at the targPFN to the offset in the file.

 * Returns the offset in the file where the data was written.

 */

int Disk_Write(FILE* swapFile, int targPFN, int verbose) {

	assert(PAGE_START(targPFN) % PAGE_SIZE == 0); // only frame-aligned access from this file

	char* physmem = Memsim_GetPhysMem();

	int diskFrameNum = ftell(swapFile);

	fwrite(&physmem[PAGE_START(targPFN)], sizeof(char), PAGE_SIZE, swapFile);

	fflush(swapFile);

	if (verbose) {

		printf("Swapped Frame %d to disk at offset %d.\n", targPFN, diskFrameNum);

	}

	return diskFrameNum;

}

/*

 * Read a PAGE_SIZE worth of bytes from the offset in the file to the physical memory starting at the targPFN.

 * Returns the number of bytes read.

 */

int Disk_ReadFrame(int diskAddr, int targPFN) {

	assert(PAGE_START(targPFN) % PAGE_SIZE == 0); // only frame-aligned access from this file

	FILE* swapFile = MMU_GetSwapFileHandle();

	int fd = fileno(swapFile);

	char* physmem = Memsim_GetPhysMem();

	if (lseek(fd, PAGE_START(diskAddr), SEEK_SET) == -1) {

		printf("Error on disk. Could not read from disk.\n");

		return -1;

	}

	int read = fread(&physmem[PAGE_START(targPFN)], sizeof(char), PAGE_SIZE, swapFile);

	printf("Swapped disk slot %d into frame %d.\n", diskAddr, targPFN);

	return read;

}

/*
 * Main start of simulation of the MMU.
 * Initializes MMU and starts receiving input and executing instructions.
 */
int main () {
	/* Setup free page tracking, page table location register storage (per process), and open swap file. */
	MMUInit(); 
	/* Begin reading instructions and completing requested operations. Loops continuously. Returns when finished. */
	return MMUStart();
}
