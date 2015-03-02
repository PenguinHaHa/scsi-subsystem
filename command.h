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
int fpdma_readwrite(int fd, unsigned int isread, unsigned int ncqtag, unsigned int startlba, unsigned int sectors, char *databuffer);
int sectors_readwrite(int fd, unsigned int isread, unsigned int startlba, unsigned int sectors, char *databuffer);

#endif
