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
#define SENSE_CODE_LENGTH  64

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
int ata_pass_through_data(int fd, int isread, char *cmd, int cmdsize, void *databuffer, int buffersize);
int check_status(unsigned char status, unsigned char *sense_b);
void parse_inquiry_data(unsigned char *buffer, unsigned int len);

///////////////
// LOCALS
///////////////
static int lasterror;

///////////////
// FUNCTIONS
///////////////

int ata_pass_through_data(int fd, int isread, char *cmd, int cmdsize, void *databuffer, int buffersize)
{
  struct sg_io_hdr io_hdr;
  unsigned char sense_b[SENSE_CODE_LENGTH];     // buffer size QQQQ:????
  
  memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
  memset(sense_b, 0, SENSE_CODE_LENGTH);

  io_hdr.cmdp = cmd;
  io_hdr.cmd_len = cmdsize;
  io_hdr.dxferp = databuffer;
  io_hdr.dxfer_len = buffersize;
  io_hdr.dxfer_direction = isread ? SG_DXFER_FROM_DEV : SG_DXFER_TO_DEV;

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

int sectors_readwrite(int fd, unsigned int isread, unsigned int startlba, unsigned int sectors, char *databuffer)
{
  int ret;
  unsigned char cmd[16];

  int protocol = isread ? 4 : 5;
  int extend = 0;
  int ck_cond  = 0;   // SATL shall terminate the command with CHECK CONDITION only if an error occurs
  int t_dir = isread ? 1 : 0;      // 1: from device, 0: from controller
  int byt_blok = 1;   // 0: transfer data is measured by byte, 1: measured by block
  int t_length = 2;   // 0: no daa is transfer, 1: length is specified in FEATURE, 2: specified in SECTOR_COUNT, 3: specified in STPSIU

  memset(cmd, 0, sizeof(cmd));

  // build ata pass through command
  cmd[0] = 0x85;
  cmd[1] = (protocol << 1) | extend;
  cmd[2] = (ck_cond << 5) | (t_dir << 3) | (byt_blok << 2) | t_length;
  cmd[6] = sectors;
  cmd[8] = startlba & 0xFF;
  cmd[10] = (startlba >> 8) & 0xFF;
  cmd[12] = (startlba >> 16) & 0xFF;
  cmd[14] = isread ? 0x20 : 0x30;

  ret = ata_pass_through_data(fd, isread, cmd, sizeof(cmd), databuffer, sectors * 512);
  if (ret != 0)
  {
    printf("ERROR, %s: line %d\n", __func__, __LINE__);
    return -1;
  }

  return 0;
  
}

int fpdma_readwrite(int fd, unsigned int isread, unsigned int ncqtag, unsigned int startlba, unsigned int sectors, char *databuffer)
{
  int ret;
  unsigned char cmd[16];

  int protocol = 7;   // DMA Queued
  int extend = 0;
  int ck_cond  = 0;   // SATL shall terminate the command with CHECK CONDITION only if an error occurs
  int t_dir = isread ? 1 : 0;      // 1: from device, 0: from controller
  int byt_blok = 1;   // 0: transfer data is measured by byte, 1: measured by block
  int t_length = 1;   // 0: no daa is transfer, 1: length is specified in FEATURE, 2: specified in SECTOR_COUNT, 3: specified in STPSIU

  memset(cmd, 0, sizeof(cmd));
  
  // build ata pass through command
  cmd[0] = 0x85;
  cmd[1] = (protocol << 1) | extend;
  cmd[2] = (ck_cond << 5) | (t_dir << 3) | (byt_blok << 2) | t_length;
  cmd[3] = (sectors >> 8) & 0xFF;        // transferred secotors size, FEATURES high byte
  cmd[4] = sectors & 0xFF;               // transferred sectors size, FEATURES low byte
  cmd[6] = ncqtag << 3;
  cmd[8] = startlba & 0xFF;
  cmd[10] = (startlba >> 8) & 0xFF;
  cmd[12] = (startlba >> 16) & 0xFF;
  cmd[13] = 0x40;                         // bit 7 FUA set to zero
  cmd[14] = isread ? 0x60 : 0x61;        // 0x60: read fpdma, 0x61 write fpdma
  
  ret = ata_pass_through_data(fd, isread, cmd, sizeof(cmd), databuffer, sectors * 512);
  if (ret != 0)
  {
    printf("ERROR, %s: line %d\n", __func__, __LINE__);
    return -1;
  }

  return 0;
}

int smart_readwritelog(int fd, unsigned int isread, unsigned int logaddr, void *databuffer, unsigned int pagenum)
{
  int ret;
  unsigned char cmd[16];

  int protocol = isread ? 4 : 5;   //4:PIO data-in, 5:PIO data-out
  int extend = 0;
  int ck_cond  = 0;   // SATL shall terminate the command with CHECK CONDITION only if an error occurs
  int t_dir = isread ? 1 : 0;      // 1: from device, 0: from controller
  int byt_blok = 1;   // 0: transfer data is measured by byte, 1: measured by block
  int t_length = 2;   // 0: no daa is transfer, 1: length is specified in FEATURE, 2: specified in SECTOR_COUNT, 3: specified in STPSIU
  
  memset(cmd, 0, sizeof(cmd));
  
  // build ata pass through command
  cmd[0] = 0x85;
  cmd[1] = (protocol << 1) | extend;
  cmd[2] = (ck_cond << 5) | (t_dir << 3) | (byt_blok << 2) | t_length;
  cmd[4] = isread ? 0xD5 : 0xD6;        // SMART READ LOG 
  cmd[6] = pagenum;     // page num
  cmd[8] = logaddr;     // LBA LOW, log addr
  cmd[10] = 0x4F;       // LBA MID
  cmd[12] = 0xC2;       // LBA HIGH
  cmd[14] = 0xB0;       // SMART
  
  ret = ata_pass_through_data(fd, isread, cmd, sizeof(cmd), databuffer, pagenum * 512);
  if (ret != 0)
  {
    printf("ERROR, %s: line %d\n", __func__, __LINE__);
    return -1;
  }

  return 0;
}

int smart_readdata(int fd, char *databuffer)
{
  int ret;
  unsigned char cmd[16];

  int protocol = 4;   // PIO data-in
  int extend = 0;
  int ck_cond  = 0;   // SATL shall terminate the command with CHECK CONDITION only if an error occurs
  int t_dir = 1;      // from device
  int byt_blok = 1;   // 0: transfer data is measured by byte, 1: measured by block
  int t_length = 2;   // 0: no daa is transfer, 1: length is specified in FEATURE, 2: specified in SECTOR_COUNT, 3: specified in STPSIU
  
  memset(cmd, 0, sizeof(cmd));
  
  // build ata pass through command
  cmd[0] = 0x85;
  cmd[1] = (protocol << 1) | extend;
  cmd[2] = (ck_cond << 5) | (t_dir << 3) | (byt_blok << 2) | t_length;
  cmd[4] = 0xD0;      // SMART READ DATA
  cmd[6] = 1;         // data length
  cmd[10] = 0x4F;     // LBA MID
  cmd[12] = 0xC2;     // LBA HIGH
  cmd[14] = 0xB0;     // SMART
  
  ret = ata_pass_through_data(fd, 1, cmd, sizeof(cmd), databuffer, 512);
  if (ret != 0)
  {
    printf("ERROR, %s: line %d\n", __func__, __LINE__);
    return -1;
  }

  return 0;
}


int identify_func(int fd, char *databuffer)
{
  int ret;
  unsigned char cmd[16];

  int protocol = 4;   // PIO data-in
  int extend = 0;
  int ck_cond  = 0;   // SATL shall terminate the command with CHECK CONDITION only if an error occurs
  int t_dir = 1;      // from device
  int byt_blok = 1;   // 0: transfer data is measured by byte, 1: measured by block
  int t_length = 2;   // 0: no daa is transfer, 1: length is specified in FEATURE, 2: specified in SECTOR_COUNT, 3: specified in STPSIU
  
  memset(cmd, 0, sizeof(cmd));

  // build ata pass throung command, refer to ATA Command Pass_Through
  cmd[0] = 0x85;     // ATA PASS-THROUGH(16) command
  cmd[1] = (protocol << 1) | extend; 
  cmd[2] = (ck_cond << 5) | (t_dir << 3) | (byt_blok << 2) | t_length;
  cmd[6] = 1;        // 1 sector count data
  cmd[14] = 0xec;    // IDENTIFY DEVICE

  ret = ata_pass_through_data(fd, 1, cmd, sizeof(cmd), databuffer, 512);
  if (ret != 0)
  {
    printf("ERROR, %s: line %d\n", __func__, __LINE__);
    return -1;
  }

  return 0;
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
  int i;
  int response_code;

  if (status)
  {
    printf("return status %x\n", status);
    printf("sense code : ");
    response_code = 0x7F & sense_b[0];
    switch (response_code)
    {
      // Fixed format sense data, refer to spc-4 section 4.5.3
      case 0x70:
      case 0x71:
        printf("sense key %x | ", sense_b[2] & 0xF);
        printf("additional sense code %x | ", sense_b[12]);
        printf("additional sense code Q %x | ", sense_b[13]);
        break;

      // Descriptor format sense data, refer to spc-4 section 4.5.2
      case 0x72:
      case 0x73:
        printf("sense key %x | ", sense_b[1] & 0xF);
        printf("additional sense code %x | ", sense_b[2]);
        printf("additional sense code Q %x | ", sense_b[3]);
        break;

    }
//    for (i = 0; i < SENSE_CODE_LENGTH; i++)
//    {
//      printf(" 0x%x ", sense_b[i]);
//    }
//    printf("\n");
  
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
