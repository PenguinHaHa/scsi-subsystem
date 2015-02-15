//
// By Penguin, 2015.2
//

#ifndef _COMMAND_H_
#define _COMMAND_H_

int ioctl_test(int fd);
int sg_inquiry(int fd);
int check_status(unsigned char status, unsigned char *sense_b);
void parse_inquiry_data(unsigned char *buffer, unsigned int len);
void parse_identify_data(unsigned char *buffer, unsigned int len);
int dev_identify(int fd);

#endif
