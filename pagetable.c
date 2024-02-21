// Starting code version 1.0 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include "mmu.h"
#include "pagetable.h"
#include "memsim.h"

int pageToEvict = 1;

typedef struct{
	int ptStartPA;
	int present;
} ptRegister;

// Page table root pointer register values 
// One stored for each process, swapped in with process)
ptRegister ptRegVals[NUM_PROCESSES]; 

/*
 * Public Interface:
 */
void PT_SetPTE(int pid, int VPN, int PFN, int valid, int protection, int present, int referenced) {
	char* physmem = Memsim_GetPhysMem();
	//todo
	//get physical address of the page table entry to be set
	int ptePA = PT_GetRootPtrRegVal(pid) + (sizeof(u_int8_t) * VPN);

	//initialize a 1-byte page table entry value with its PFN and indicator bits, then place it at the physical address
	u_int8_t pageTableEntry = (u_int8_t) (VPN << 6);
	pageTableEntry += (u_int8_t) (PFN << 4);
	pageTableEntry += (u_int8_t) (valid << 3);
	pageTableEntry += (u_int8_t) (protection << 2);
	pageTableEntry += (u_int8_t) (present << 1);
	pageTableEntry += (u_int8_t) (referenced);
	physmem[ptePA] = pageTableEntry;

	return;
}

/* 
 * Set all PTE valid bits to zero (invalid)
 * Returns a new page for the map instruction
 */
int PT_PageTableInit(int pid, int pa){
	char* physmem = Memsim_GetPhysMem();
	// todo
	int ptPA = PT_GetRootPtrRegVal(pid);
	for(int i = ptPA; i < ptPA + (sizeof(u_int8_t) * NUM_PAGES); i++) {
		if(physmem[i] != 0) {
			physmem[i] &= (u_int8_t) 0xf7;
		}
	}
	return PFN(pa);
 }

 void PT_PageTableCreate(int pid, int pa){
	//todo
	 ptRegVals[pid].ptStartPA = pa;
	 ptRegVals[pid].present = 1;
	 return;
 }

 int PT_PageTableExists(int pid){
	//todo
	 return ptRegVals[pid].ptStartPA != -1;
 }

/* Gets the location of the start of the page table. If it is on disk, brings it into memory. */
int PT_GetRootPtrRegVal(int pid){
	// todo
	return ptRegVals[pid].ptStartPA;
}

/*
 * Evicts the next page. 
 * Updates the corresponding information in the page table, returns the location that was evicted.
 * 
 * The supplied input and output used in autotest.sh *RR tests, uses the round-robin algorithm. 
 * You may also implement the simple and powerful Least Recently Used (LRU) policy, 
 * or another fair algorithm.
 */
int PT_Evict() {
	char* physmem = Memsim_GetPhysMem();
	FILE* swapFile = MMU_GetSwapFileHandle();
	//todo

	//get pid of process which currently resides in the page to evict and its VPN
	int pid;
	int diskPage;
	int targPFN = PTNextEvictionRR();
	for(int i = 0; i < NUM_PROCESSES; i++) {
		if(PT_PageTableExists(i)) {
			int ptPA = PT_GetRootPtrRegVal(i);
			for(int j = ptPA; j < ptPA + (sizeof(u_int8_t) * NUM_PAGES); j++) {
				int physicalPage = (physmem[j] & 0x30) >> 4;
				if(physmem[j] != 0 && physicalPage == targPFN) {
					physmem[j] &= (u_int8_t) 0xFD;
					pid = i;
					diskPage = j - ptPA;
					break;
				}
			}
		}

	}

	//write the page contents into proper place in swap file
	int fd = fileno(swapFile);
	if(Disk_Flush(fd, swapFile) == -1) {
		return -1;
	}

	int offset = (pid * 4 * 16) + PAGE_START(diskPage);
	lseek(fd, offset, SEEK_SET);
	Disk_Write(swapFile, targPFN, 1);

	//clear out phyiscal frame for new virtual page
	for(int i = PAGE_START(targPFN); i < PAGE_START(targPFN) + PAGE_SIZE; i++) {
		physmem[i] = 0;
	}

	return PAGE_START(targPFN);
}

/* Returns next page selected for eviction, based on policy. Currently simple Round Robin. */

int PTNextEvictionRR() {

	int nextPgToEvict = pageToEvict;

	pageToEvict++;

	pageToEvict = pageToEvict % NUM_PAGES; //wraps around to first page if it goes over

	if(pageToEvict == 0) {
		pageToEvict++;
	}

	return nextPgToEvict;

}

/*
 * Searches through the process's page table. If an entry is found containing the specified VPN, 
 * return the address of the start of the corresponding physical page frame in physical memory. 
 *
 * If the physical page is not present, first swaps in the physical page from the physical disk,
 * and returns the physical address.
 * 
 * Otherwise, returns -1.
 */
int PT_VPNtoPA(int pid, int VPN){
	char *physmem = Memsim_GetPhysMem();
	FILE* swapFile = MMU_GetSwapFileHandle();
	//todo
	int fd = fileno(swapFile);
	if(PT_PageTableExists(pid)) {
		int ptePA = PT_GetRootPtrRegVal(pid) + (sizeof(u_int8_t) * VPN);
		if(physmem[ptePA] != 0) {
			int present = (physmem[ptePA] & 0x2) >> 1;
			if(!present) {
				int pa = PT_Evict();
				Disk_ReadFrame((pid * 4) + VPN, PFN(pa));
				physmem[ptePA] &= (u_int8_t) 0xCF;
				physmem[ptePA] |= (u_int8_t) (PFN(pa) << 4);
				physmem[ptePA] |= (u_int8_t) 0x2;
				printf("Virtual page %d for PID %d fetched from disk into frame %d.\n", VPN, pid, PFN(pa));
			}
			int physicalPage = (physmem[ptePA] & 0x30) >> 4;
			return PAGE_START(physicalPage);
		}
	}
	return -1;
}

/*
 * Finds the page table entry corresponding to the VPN, and checks
 * to see if the protection bit is set to 1 (readable and writable).
 * If it is 1, it returns TRUE, and FALSE if it is not found or is 0.
 */
int PT_PIDHasWritePerm(int pid, int VPN){
	char* physmem = Memsim_GetPhysMem();
	//todo
	if(PT_PageTableExists(pid)) {
		int ptePA = ptRegVals[pid].ptStartPA + (sizeof(u_int8_t) * VPN);
		if(physmem[ptePA] != 0) {
			int protection = (physmem[ptePA] & 0x4) >> 2;
			return protection;
		}
	}
	return 0;
}

/* Initialize the register values for each page table location (per process). */
void PT_Init() {
	//todo
	for(int i = 0; i < NUM_PROCESSES; i++) {
		ptRegVals[i].ptStartPA = -1;
		ptRegVals[i].present = 0;
	}
	return;
}
