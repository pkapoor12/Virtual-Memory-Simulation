// Starting code version 1.0

#ifndef PROJECT3_H
#define PROJECT3_H

/*
 * Public Interface:
 */

#define TRUE 1
#define FALSE 0

FILE* MMU_GetSwapFileHandle();
int MMU_TranslateAddress(int process_id, int VPN, int offset);
int Disk_Flush(int fd, FILE* swapFile);
int Disk_Write(FILE* swapFile, int targPFN, int verbose);
int Disk_ReadFrame(int diskAddr, int targPFN);

#endif // PROJECT3_H
