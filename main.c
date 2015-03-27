//
// This is Penguin's linux scsi
// This app is used to investigate linux scsi sub-system which covers ATA, usb, IDE, iscsi....
//


#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "command.h"

typedef struct _PARAMETERS {
  char dev_path[256];
} PARAMETER;

///////////////
// PROTOTYPES
///////////////
void parse_options(PARAMETER *param, int argc, char **argv);
void print_usage(void);
void scsi_dev(char* const dev_path);
int check_file_state(int fd);
void parse_identify_data(unsigned char *buffer, unsigned int len);
void get_identifydata(int fd);
void parse_smart_data(unsigned char *buffer, unsigned int len);
void get_smartdata(int fd);
void get_smartlogdir(int fd);
void parse_smart_log(unsigned char *buffer, unsigned int len);
void read_data(int fd);

///////////////
// LOCALS
///////////////
static int lasterror;
const char* const short_options = "hd:D";
const struct option long_options[] = {
  {"help", 0, NULL, 'h'},
  {"devpath", 1, NULL, 'd'},
  {"debug", 0, NULL, 'D'}
};

extern unsigned int isDebug;
///////////////
// FUNCTIONS
///////////////
int main(int argc, char* argv[])
{
  PARAMETER scsi_param;

  printf("This is Penguin's scsi sub-system test\n");

  parse_options(&scsi_param, argc, argv);

  scsi_dev(scsi_param.dev_path);

  return 0;
}

void print_usage(void)
{
  printf("  -h  --help     Display usage information\n");
  printf("  -d  --devpath  Specify test scsi device path\n");
  printf("  -D  --debug    print debug info\n");
}

void parse_options(PARAMETER *param, int argc, char **argv)
{
  int option;
  char *opt_arg = NULL;

  if (argc == 1)
  {
    print_usage();
    exit(0);
  }

  do
  {
    option = getopt_long(argc, argv, short_options, long_options, NULL);
    switch(option)
    {
      case 'h':
        print_usage();
        exit(0);

      case 'd':
        opt_arg = optarg;
        strcpy(param->dev_path, opt_arg);
        break;

      case 'D':
        isDebug = 1;
        break;

      case -1:
        break;

      default:
        printf("Undefined parameter!\n");
        print_usage();
        exit(0);
    }
  } while (option != -1);

  if (param->dev_path == NULL)
  {
    printf("Please input device path\n");
    exit(0);
  }
}

void scsi_dev(char* const dev_path)
{
  int scsi_fd;
  int ret = 0;

  printf("SCSI dev : %s\n", dev_path);

  scsi_fd = open(dev_path, O_RDONLY | O_NONBLOCK);
  if (scsi_fd < 0)
  {
    lasterror = errno;
    printf("Open %s failed (%d) - %s\n", dev_path, lasterror, strerror(lasterror));
    exit(-1);
  }

//  printf("Check file state:\n");
//  check_file_state(scsi_fd);
  
//  printf("\nioctl test:\n");
//  ioctl_test(scsi_fd);
//  printf("\nInquiry:\n");
//  sg_inquiry(scsi_fd);

//  printf("This is ATA COMMAND PASS THROUGH\n");
//  get_identifydata(scsi_fd);
//  get_smartdata(scsi_fd);
//  get_smartlogdir(scsi_fd);
  read_data(scsi_fd);

  close(scsi_fd);
}

void read_data(int fd)
{
  int ret;
  unsigned int startlba = 1;
  unsigned int sectors = 1;
  unsigned int ncqtag = 3;
  unsigned int isread = 1;
//  unsigned int isread = 0;
  unsigned char *databuffer;

  databuffer = (char *)malloc(512 * sectors);
  memset(databuffer, 0, 512 * sectors);

  if (isread == 0)
  {
    unsigned int input;
    printf("Wrtie op, it will destroy the current data, press y to continue, or stop with any other key?\n");
    input = getchar();
    if (input != 'y')
      return;
    
    databuffer[0]  = 0x11;
    databuffer[2]  = 0x22;
    databuffer[4]  = 0x44;
    databuffer[6]  = 0x66;
    databuffer[8]  = 0x88;
    databuffer[10] = 0xaa;
    
    databuffer[511] = 0xFF;
    databuffer[509] = 0xEE;
    databuffer[507] = 0xDD;
    databuffer[505] = 0xCC;
    databuffer[503] = 0xBB;
  }

//  ret = fpdma_readwrite(fd, isread, ncqtag, startlba, sectors, databuffer);
 ret = sectors_readwrite(fd, isread, startlba, sectors, databuffer);

  if (ret == 0 && isread)
  {
    int i;
    printf("sector %d: \n", startlba);
    for (i= 0; i < 512; i++)
    {
      printf("%d:%x | ", i, databuffer[i]);
    }
  printf("\n");
  }

  free(databuffer);
}

void get_smartlogdir(int fd)
{
  char *smartlog;
  unsigned int isread = 1;
  unsigned int logaddr = 0;

  smartlog = (char *)malloc(512);
  memset(smartlog, 0, 512);

  smart_readwritelog(fd, isread, logaddr, smartlog, 1);
  
  parse_smart_log(smartlog, 512);

}

void parse_smart_log(unsigned char *buffer, unsigned int len)
{
  int i;
  printf("smart log dir : ");
  for (i = 0; i < len; i++)
  {
    printf("%d:%x | ", i, buffer[i]);
  }
  printf("\n");
}

void get_smartdata(int fd)
{
  char *smartdata;

  smartdata = (char *)malloc(512);
  memset(smartdata, 0, 512);

  smart_readdata(fd, smartdata);

  parse_smart_data(smartdata, 512);
}

void parse_smart_data(unsigned char *buffer, unsigned int len)
{

  printf("SMART data : \n");
  printf("Off-line data collection status: 0x%02x\n", buffer[362]);
  printf("Self-test execution status byte: 0x%02x\n", buffer[363]);
  printf("Total time in seconds to complete off-line data collection activity: 0x%02x%02x s\n", buffer[365], buffer[364]);
  printf("Short self-test routine recommended polling time in minutes : 0x%02x m\n", buffer[372]);
  printf("Extended self-test routine recommended polling time in minutes: 0x%02x m\n", buffer[373]);
  printf("Conveyance self-test routine recommended polling time in minutes: 0x%02x m \n", buffer[374]);
}

void get_identifydata(int fd)
{
  char *identifydata;

  identifydata = (char *)malloc(512);
  memset(identifydata, 0, 512);

  identify_func(fd, identifydata);
  
  parse_identify_data(identifydata, 512);
}

void parse_identify_data(unsigned char *buffer, unsigned int len)
{

  printf("Identify data: \n");
  printf("Serial number %s\n", buffer + 20);
  printf("Model number %s\n", buffer + 54);
}

int check_file_state(int fd)
{
  struct stat f_stat;

  if (fstat(fd, &f_stat) < 0)
  {
    lasterror = errno;
    printf("Get file state failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  switch (f_stat.st_mode & S_IFMT) {
    case S_IFBLK:   printf("block device\n");     break;
    case S_IFCHR:   printf("character device\n"); break;
    case S_IFDIR:   printf("directory\n");        break;
    case S_IFIFO:   printf("FIFO/pipe\n");        break;
    case S_IFLNK:   printf("symlink\n");          break;
    case S_IFREG:   printf("regular file\n");     break;
    case S_IFSOCK:  printf("socket\n");           break;
    default:        printf("unknown?\n");         break;
  }

  printf("I-node number:            %ld\n", (long)f_stat.st_ino);
  printf("Mode:                     %lo (octal)\n", (unsigned long)f_stat.st_mode);
  printf("Link count:               %ld\n", (long)f_stat.st_nlink);
  printf("Ownership:                UID=%ld GID=%ld\n", (long)f_stat.st_uid, (long)f_stat.st_gid);
  printf("Preferred I/O block size: %ld bytes\n", (long)f_stat.st_blksize);
  printf("File size:                %lld bytes\n", (long long)f_stat.st_size);
  printf("Blocks allocated:         %lld bytes\n", (long long)f_stat.st_blocks);
  printf("Last status change:       %s", ctime(&f_stat.st_ctime));
  printf("Last file access:         %s", ctime(&f_stat.st_atime));
  printf("Last file modification:   %s", ctime(&f_stat.st_mtime));
 
  return 0;
}
