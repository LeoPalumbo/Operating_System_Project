#include "umps/arch.h"
#include "umps/types.h"

#define READY         1
#define BUSY          3
#define TRANSMITTED   5
#define RECEIVED      5

#define CMD_ACK       1
#define CMD_TRANSMIT  2
#define CMD_RECV      2

#define CHAR_OFFSET   8
#define STATUS_MASK   0xFF

#define BUF_SIZE 64

/* Define a new terminal devices on line 0. */
volatile termreg_t *terminal = (termreg_t*) DEV_REG_ADDR(IL_TERMINAL, 0);

/* Return the status of transmitter. */
static unsigned int tx_status(volatile termreg_t *tp);
/* Return the status of receiver. */
static unsigned int rx_status(volatile termreg_t *tp);

/**
 * * Print a char on the terminal.
 * * Return 1 if device not ready or char not transmitted, 0 otherwise.
*/
int putchar(char c) {
  unsigned int stat = tx_status(terminal);

  if(stat != READY && stat != TRANSMITTED)
    return -1;
  
  terminal->transm_command = ((c << CHAR_OFFSET) | CMD_TRANSMIT);

  while((stat = tx_status(terminal)) == BUSY) ;

  if(stat != TRANSMITTED) 
    return -1;
  
  terminal->transm_command = CMD_ACK;

  return 0;
}

/**
 * * Print a string on the terminal by calling putchar for every char.
 * * Return -1 if error in putchar, 0 otherwise. 
 */
int putstring(char *str) {
  for( ; *str; ++str) 
    if(putchar(*str))
      return -1;

  return 0;
}

/**
 * * Get a char from the terminal.
 * * Return -1 if device not ready or no char, otherwise return the read char.
 */
int getchar(void) {
  unsigned int stat = rx_status(terminal);

  if(stat != READY && stat != RECEIVED) 
    return -1;
  
  terminal->recv_command = CMD_RECV;

  while((stat = rx_status(terminal)) == BUSY) ;

  if(stat != RECEIVED)
    return -1;
  
  stat = terminal->recv_status;
  terminal->recv_command = CMD_ACK;

  return stat >> CHAR_OFFSET;
}

void getstring(char *buf, unsigned int count) {
  int c;

  while(--count && (c = getchar()) != '\n')
    *buf++ = c;
  
  *buf = '\0';
}

static unsigned int tx_status(volatile termreg_t *t) {
  return ((t->transm_status) & STATUS_MASK);
}

static unsigned int rx_status(volatile termreg_t *t) {
  return ((t->recv_status) & STATUS_MASK);
}