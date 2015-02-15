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

///////////////
// LOCALS
///////////////
static int lasterror;
const char* const short_options = "hd:";
const struct option long_options[] = {
  {"help", 0, NULL, 'h'},
  {"devpath", 1, NULL, 'd'}
};

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
  
 // printf("\nioctl test:\n");
//  ioctl_test(scsi_fd);
//  printf("\nInquiry:\n");
//  sg_inquiry(scsi_fd);

  printf("This is ATA COMMAND PASS THROUGH\n");
//  ata_pass_through_identify(scsi_fd);
  dev_identify(scsi_fd);

  close(scsi_fd);
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
