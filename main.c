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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <scsi/sg.h>

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
int ioctl_test(int fd);
int sg_inquiry(int fd);
int check_status(unsigned char status, unsigned char *sense_b);
void parse_inquiry_data(unsigned char *buffer, unsigned int len);

///////////////
// LOCALS
///////////////
int lasterror;
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

  //check_file_state(scsi_fd);
  //ioctl_test(scsi_fd);
  sg_inquiry(scsi_fd);
  close(scsi_fd);
}

int ioctl_test(int fd)
{
  int version;
 
  if (ioctl(fd, SG_GET_VERSION_NUM, &version) < 0)
  {
    lasterror = errno;
    printf("Ioctl SG_GET_VERSION_NUM failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  printf("Version number %o\n", version);
  return 0;
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

int sg_inquiry(int fd)
{
  struct sg_io_hdr io_hdr;
  unsigned char cmd[6];
  unsigned char databuffer[512];
  unsigned char sense_b[64];

  memset(sense_b, 0, 64);
  memset(databuffer, 0, 512);
  memset(&io_hdr, 0, sizeof(struct sg_io_hdr));

  // build inquiry command, refer to SPC3, section 6.4 INQUIRY command
  cmd[0] = 0x12;     // INQUIRY command
  cmd[1] = 0;        // bit 0 EVPD is ZERO
  cmd[2] = 0;        // page code is ZERO
  cmd[3] = 0;        // high byte of allocation length
  cmd[4] = 36;       // low byte of allocation length
  cmd[5] = 0;        // control byte

  // build sg_io_hdr
  io_hdr.cmdp = cmd;
  io_hdr.cmd_len = sizeof(cmd);
  io_hdr.dxferp = databuffer;
  io_hdr.dxfer_len = 36;
  io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;

  io_hdr.interface_id = 'S';    // ????
  io_hdr.mx_sb_len = 64;        // sizeof(sense_b)
  io_hdr.sbp = sense_b;
  io_hdr.pack_id = 0;           // ????
  io_hdr.timeout = 60 * 1000;   // 60 seconds

  // send command
  if (ioctl(fd, SG_IO, &io_hdr) < 0)
  {
    lasterror = errno;
    printf("Send command failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  // check status
  if (check_status(io_hdr.status, io_hdr.sbp) != 0)
    return -1;

  // parse inquiry data
  parse_inquiry_data(io_hdr.dxferp, io_hdr.dxfer_len);

  return 0;
}

int check_status(unsigned char status, unsigned char *sense_b)
{
  if (status)
  {
    printf("return status %x\n", status);
    return -1;
  }

  return 0;
}

void parse_inquiry_data(unsigned char *buffer, unsigned int len)
{
  int i;

  printf("Peripheral device type %x\n", buffer[0] & 0x1F);
  printf("Peripheral qualifier %x\n", (buffer[0] & 0xE0) >> 5);
  printf("Version %x\n", buffer[2]);
  printf("Additional length %d\n", buffer[4]);
  printf("T10 vendor identification ");
  for (i = 0; i < 8; i++)
  {
    printf("%c", buffer[8+i]);
  }
  printf("\n");

  printf("Product identification ");
  for (i = 0; i < 16; i++)
  {
    printf("%c", buffer[16+i]);
  }
  printf("\n");

  printf("Product revision level ");
  for (i = 0; i < 4; i++)
  {
    printf("%c", buffer[32+i]);
  }
  printf("\n");
}
