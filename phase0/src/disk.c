#include "umps/arch.h"
#include "umps/types.h"

/* disk status */
#define DISK_READY	1
#define DISK_BUSY	3

/* disk commands */
#define DISK_ACK	1
#define DISK_SEEK	2
#define DISK_READBLK   	3
#define DISK_WRITEBLK	4

#define HEAD_OFFSET 	16
#define SECT_OFFSET	8
#define CYL_OFFSET 8
#define DISK_STATUS_MASK   0xFF

/* create a disk device */
volatile dtpreg_t *disk = (dtpreg_t *) DEV_REG_ADDR(IL_DISK,0);

/* read disk status */
static unsigned int disk_status(volatile dtpreg_t *disk);

/* store data contains in data0 at specified head and sect */
int store (char* data0, unsigned int head, unsigned int sect){
    unsigned int stat = disk_status(disk);

    if(stat != DISK_READY)
	     return 0;

    disk->data0 = (unsigned int) data0;
    disk->command = ((DISK_WRITEBLK) | (head << HEAD_OFFSET)) | sect << SECT_OFFSET;

    while((stat = disk_status(disk)) == DISK_BUSY)
      ;

    disk->command = DISK_ACK;

    return 1;
}

/* read the register DATA0 located at specified head and sect */
char* read (unsigned int head, unsigned int sect){
    unsigned int stat = disk_status(disk);

    if(stat == DISK_READY){
       disk->command = ((DISK_READBLK) | (head << HEAD_OFFSET)) | sect << SECT_OFFSET;

        while((stat = disk_status(disk)) == DISK_BUSY)
          ;

        disk->command = DISK_ACK;
    }

    return (char *)disk->data0;
}

/* seek the cylinder specified by cyl */
int seek (unsigned int cyl){
  unsigned int stat = disk_status(disk);

  if (stat != DISK_READY)
    return -1;

  disk->command = (DISK_SEEK | (cyl << CYL_OFFSET));
  while((stat = disk_status(disk)) != DISK_READY)
    ;

  return 1;
}

static unsigned int disk_status(volatile dtpreg_t *disk){
   return ((disk->status) & DISK_STATUS_MASK);
}
