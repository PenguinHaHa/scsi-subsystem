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

typedef enum _OPS {
  OP_READ = 0,
  OP_WRITE,
  OP_IDENTIFY
} OPS;

typedef struct _PARAMETERS {
  char dev_path[256];
  OPS  operation;
  unsigned int startlba;
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
const char* const short_options = "hd:o:s:D";
const struct option long_options[] = {
  {"help", 0, NULL, 'h'},
  {"devpath", 1, NULL, 'd'},
  {"operate", 1, NULL, 'o'},
  {"startlba", 1, NULL, 's'},
  {"debug", 0, NULL, 'D'}
};
PARAMETER scsi_param;

extern unsigned int isDebug;
///////////////
// FUNCTIONS
///////////////
int main(int argc, char* argv[])
{

  printf("This is Penguin's scsi sub-system test\n");

  parse_options(&scsi_param, argc, argv);

  scsi_dev(scsi_param.dev_path);

  return 0;
}

void print_usage(void)
{
  printf("  -h  --help          Display usage information\n");
  printf("  -d  --devpath       Specify test scsi device path\n");
  printf("  -o  --operate=r/w/i Specify read/write operetion\n");
  printf("  -s  --startlba      Specify startlba to read/write\n");
  printf("  -D  --debug         print debug info\n");
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

  param->operation = OP_IDENTIFY;
  param->startlba = 0;

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

      case 'o':
        opt_arg = optarg;
        if (*opt_arg == 'r')
          param->operation = OP_READ;
        else if (*opt_arg == 'w')
          param->operation = OP_WRITE;
        else if (*opt_arg == 'i')
          param->operation = OP_IDENTIFY;
        else
        {
          printf("unsupported operation of option -o\n");
          print_usage();
          exit(0);
        }
        break;

      case 's':
        opt_arg = optarg;
        param->startlba = strtol(opt_arg, NULL, 0);
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

  if (isDebug)
    printf("OPTIONS : dev_path %s, operation %x, startlba %x\n", param->dev_path, param->operation, param->startlba); 
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
//  get_smartdata(scsi_fd);
//  get_smartlogdir(scsi_fd);
  if (scsi_param.operation == OP_IDENTIFY)
    get_identifydata(scsi_fd);
  
  if (scsi_param.operation == OP_READ || scsi_param.operation == OP_WRITE)
    read_data(scsi_fd);

  close(scsi_fd);
}

void read_data(int fd)
{
  int ret;
  unsigned int startlba = scsi_param.startlba;
  unsigned int sectors = 1;                // QQQQ For USB storage device, this field is a little weird, different kind of device has differrent max value of this field (0xF0, 1, ...)
  unsigned int ncqtag = 3;
  unsigned int isread = (scsi_param.operation == OP_READ) ? 1 : 0;
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
      printf("%x ", databuffer[i]);
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
  unsigned short *iden;

  iden = (unsigned short*)buffer;

  if (isDebug)
    printf("\nDEBUG, word[0] %x\n", iden[0]);
  printf("%s Identify data:\n", iden[0] >> 15 ? "ATAPI" : "ATA");
  printf("Serial number %s\n", (unsigned char *)(iden + 10));
  printf("Model number %s\n", (unsigned char *)(iden + 27));

  // PACKET feature set, bit 4 of WORD 82 & 85, 1 : support while 0 : unsupport
  if (isDebug)
    printf("\nDEBUG, bit 4 of word[82] %x, bit 4 of word[85] %x\n", iden[82], iden[85]);
  if ((iden[82] & (1 << 4)) && ((iden[85] & (1 << 4))))
    printf("PACKET feature set is supported\n");
  else
    printf("PACKET feature set is NOT supported\n");

  // bit 12:8 indicate command set used by PACKET command, valid for ATAPI device
  if (iden[0] >> 15)
  {
    if (isDebug)
      printf("\nDEBUG, bit 12:8 of word[0] %x\n", iden[0]);
    printf("COMMAND set %x\n", (iden[0] >> 8) & 0x1F);
  }

  // 48-bit Address feature set, bit 10 of WORD 83 & 86, 1 : support while. Only in IDENTIFY DATA
  if (isDebug)
    printf("\nDEBUG, bit 10 0f word[83] %x, bit 10 of word[86] %x\n", iden[83], iden[86]);
  if ((iden[83] & (1 << 10)) && ((iden[86] & (1 << 10))))
    printf("48-bit Address feature set is supported\n");
  else
    printf("48-bit Address feature set is NOT supported\n");
  
  if (isDebug)
    printf("\nDEBUG, word[60] %x, word[61] %x\n", iden[60], iden[61]);
  printf("Total number of user addressable logical sectors for 28-bit commands %x%x\n", iden[61], iden[60]);
  if (isDebug)
    printf("\nDEBUG, word[100] %x, word[101] %x, word[102] %x, word[103] %x\n", iden[100], iden[101], iden[102], iden[103]);
  printf("Total number of user addressable logical sectors for 48-bit commands %x%x%x%x\n", iden[103], iden[102], iden[101], iden[100]);

  // NCQ(Native Command Queuing) feature set, bit 8 of WORD 76, 1 : support while 0 : unsupport
  if (isDebug)
    printf("\nDEBUG, bit 8 of word[76] %x\n", iden[76]);
  if (iden[76] & (1 << 8))
    printf("NCQ feature set is support\n");
  else
    printf("NCQ feature set is NOT support\n");

  // TCQ(Tagged Command Queuing) feature set, bit 1 of WORD 83 & 86, 1 : support while 0 : unsupport. Only in IDENTIFY DATA
  if (isDebug)
    printf("\nDEBUG, bit 1 of word[83] %x, bit 1 of word[86] %x\n", iden[83], iden[86]);
  if ((iden[83] & (1 << 1)) && (iden[86] & (1 << 1)))
    printf("TCQ feature set is support\n");
  else
    printf("TCQ feature set is NOT support\n");

  // Queue depth for TCQ/NCQ, bit 4:0 of WORD 75
  if (isDebug)
    printf("\nDEBUG, bit 4:0 of word[75] %x\n", iden[75]);
  printf("queue depth %x\n", (iden[75] & 0x1F) + 1);

  // Streaming feature set, bit 4 of WORD 84, 1 : support while 0 : unsupport. 48-bit address only
  if (isDebug)
    printf("\nDEBUG, bit 4 of word[84] %x\n", iden[84]);
  if (iden[84] & (1 << 4))
    printf("Streaming feature set is support\n");
  else
    printf("Streaming feature set is NOT support\n");
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
