//
// By Penguin, 2015.2
//

#ifndef _COMMAND_H_
#define _COMMAND_H_

int ioctl_test(int fd);
int sg_inquiry(int fd);
int smart_readdata(int fd, char *databuffer);
int identify_func(int fd, char *databuffer);
int smart_readwritelog(int fd, unsigned int isread, unsigned int logaddr, void *databuffer, unsigned int pagenum);
int fpdma_readwrite(int fd, unsigned int isread, unsigned int ncqtag, unsigned long startlba, unsigned int sectors, char *databuffer);
int sectors_readwrite(int fd, unsigned int isread, unsigned int isext, unsigned long startlba, unsigned int sectors, char *databuffer);
int dmaqueued_readwrite(int fd, unsigned int isread, unsigned int isext, unsigned int tag, unsigned long startlba, unsigned int sectors, char *databuffer);
int dma_readwrite(int fd, unsigned int isread, unsigned int isext, unsigned long startlba, unsigned int sectors, char *databuffer);
int multi_readwrite(int fd, unsigned int isread, unsigned int isext, unsigned long startlba, unsigned int sectors, char *databuffer);
int sg_mode(int fd);

#endif
