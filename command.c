//
// By Penguin, 2015.2
// Send command by ioctl interface
//
#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>

#include "command.h"

#define MAX_LENGTH_OUTPUT  512

// The host associated with the device's fd either has a host dependent information string or failing that its name, output into the given
// structure. Note that the output starts at the begining of given structure(overwriting the input length).
// N.B, a trailing '\0' may need to be put on the output string if it has been truncated by the input length.
// A return value of 1 indicates the host is present, 0 indicates that the host isn't present and a negative value indicated an error
typedef union _PROBE_HOST_ {
  unsigned int length;                   // max length of output ASCII string
  char buffer[MAX_LENGTH_OUTPUT];
} PROBE_HOST;

///////////////
// PROTOTYPE
///////////////
int ata_pass_through_data(int fd, char *cmd, int cmdsize, void *databuffer, int buffersize);
int check_status(unsigned char status, unsigned char *sense_b);
void parse_inquiry_data(unsigned char *buffer, unsigned int len);
void parse_identify_data(unsigned char *buffer, unsigned int len);

///////////////
// LOCALS
///////////////
static int lasterror;

///////////////
// FUNCTIONS
///////////////

int ata_pass_through_data(int fd, char *cmd, int cmdsize, void *databuffer, int buffersize)
{
  struct sg_io_hdr io_hdr;
  unsigned char sense_b[64];     // buffer size QQQQ:????
  
  memset(&io_hdr, 0, sizeof(struct sg_io_hdr));

  io_hdr.cmdp = cmd;
  io_hdr.cmd_len = cmdsize;
  io_hdr.dxferp = databuffer;
  io_hdr.dxfer_len = buffersize;
  io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;

  io_hdr.interface_id = 'S';     // m:wqeans SCSI Generic driver interface 
  io_hdr.mx_sb_len = sizeof(sense_b);
  io_hdr.sbp = sense_b;
  io_hdr.timeout = 20 * 1000;    // 60 seconds  QQQQ:????
  //io_hdr.pack_id = 0           // User can identify the request by using this field

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

  return 0;
}

int dev_identify(int fd)
{
  int ret;
  unsigned char cmd[16];
  unsigned char databuffer[512];

  int protocol = 4;   // PIO data-in
  int extend = 0;
  int ck_cond  = 0;   // SATL shall terminate the command with CHECK CONDITION only if an error occurs
  int t_dir = 1;      // from device
  int byt_blok = 1;   // 0: transfer data is measured by byte, 1: measured by block
  int t_length = 2;   // 0: no daa is transfer, 1: length is specified in FEATURE, 2: specified in SECTOR_COUNT, 3: specified in STPSIU
  
  memset(cmd, 0, sizeof(cmd));
  memset(databuffer, 0, sizeof(databuffer));

  // build ata pass throung command, refer to ATA Command Pass_Through
  cmd[0] = 0x85;     // ATA PASS-THROUGH(16) command
  cmd[1] = (protocol << 1) | extend; 
  cmd[2] = (ck_cond << 5) | (t_dir << 3) | (byt_blok << 2) | t_length;
  cmd[6] = 1;        // 1 sector count data
  cmd[14] = 0xec;    // IDENTIFY DEVICE

  ret = ata_pass_through_data(fd, cmd, sizeof(cmd), databuffer, sizeof(databuffer));
  if (ret != 0)
  {
    printf("ERROR, %s: line %d\n", __func__, __LINE__);
    return -1;
  }

  parse_identify_data(databuffer, sizeof(databuffer));
  
  return 0;
}

void parse_identify_data(unsigned char *buffer, unsigned int len)
{
  int i;

  printf("Identify data: \n");
  printf("Serial number %s\n", buffer + 20);
  printf("Model number %s\n", buffer + 54);
}

int ioctl_test(int fd)
{
  int ret;
  int version;
 
  if (ioctl(fd, SG_GET_VERSION_NUM, &version) != 0)
  {
    lasterror = errno;
    printf("Ioctl SG_GET_VERSION_NUM failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  printf("Version number %o\n", version);
  // SCSI_IOCTL_PROBE_HOST
  PROBE_HOST probe_host;
  probe_host.length = MAX_LENGTH_OUTPUT;
  ret = ioctl(fd, SCSI_IOCTL_PROBE_HOST, &probe_host);
  if (ret < 0) // 1 : host present, 0 : host is not present
  {
    lasterror = errno;
    printf("Ioctl SCSI_IO_PROBE_HOST failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }
  else if (ret == 0)
  {
    printf("Host is not present\n");
    return 0;
  }

  printf("host info %s\n", probe_host.buffer);

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
