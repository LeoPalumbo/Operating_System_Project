#include "umps/arch.h"
#include "umps/types.h"

/* terminal status */
#define ST_READY           1
#define ST_BUSY            3
#define ST_TRANSMITTED     5
#define ST_RECEIVED        5

/* terminal commands */
#define CMD_ACK            1
#define CMD_TRANSMIT       2
#define CMD_RECV           2

#define CHAR_OFFSET        8
#define TERM_STATUS_MASK   0xFF

/* create terminal device */
volatile termreg_t *term0_reg = (termreg_t *) DEV_REG_ADDR(IL_TERMINAL, 0);
volatile termreg_t *term1_reg = (termreg_t *) DEV_REG_ADDR(IL_TERMINAL, 1);
/* utility function to read the status device */
static unsigned int tx_status(volatile termreg_t *tp);
static unsigned int rx_status(volatile termreg_t *tp);


/* convert a C-string to int value */
int atoi(const char* s){
  int num=0;
  while(*s) {
   num=((*s)-'0')+num*10;
   s++;
  }
  return num;
}


int term_putchar(char c, int currentTerminal){
    unsigned int stat;
    if(currentTerminal==0){
        stat = tx_status(term0_reg);

        if (stat != ST_READY && stat != ST_TRANSMITTED) return -1;

        term0_reg->transm_command = ((c << CHAR_OFFSET) | CMD_TRANSMIT);

        while ((stat = tx_status(term0_reg)) == ST_BUSY);

        term0_reg->transm_command = CMD_ACK;

        if (stat != ST_TRANSMITTED) return -1;
        else  return 0;

    }else{
        stat = tx_status(term1_reg);

        if (stat != ST_READY && stat != ST_TRANSMITTED) return -1;

        term1_reg->transm_command = ((c << CHAR_OFFSET) | CMD_TRANSMIT);

        while ((stat = tx_status(term1_reg)) == ST_BUSY);

        term1_reg->transm_command = CMD_ACK;

        if (stat != ST_TRANSMITTED) return -1;
        else return 0;
     }
}


/* write str on the terminal one char at once */
int term_puts(const char *str,unsigned int currentTerminal){
    for (; *str; ++str)
        if (term_putchar(*str,currentTerminal))
            return -1;
    return 0;
}

int term_getchar(int currentTerminal){
    unsigned int stat;
    if (currentTerminal==0){
         stat = rx_status(term0_reg);
         if (stat != ST_READY && stat != ST_RECEIVED) return -1;

         term0_reg->recv_command = CMD_RECV;

         while ((stat = rx_status(term0_reg)) == ST_BUSY);

         if (stat != ST_RECEIVED) return -1;

         stat = term0_reg->recv_status;

         term0_reg->recv_command = CMD_ACK;

         return stat >> CHAR_OFFSET;
     }else{
         stat = rx_status(term1_reg);
         if (stat != ST_READY && stat != ST_RECEIVED) return -1;

         term1_reg->recv_command = CMD_RECV;

         while ((stat = rx_status(term1_reg)) == ST_BUSY);

         if (stat != ST_RECEIVED) return -1;

         stat = term1_reg->recv_status;

         term1_reg->recv_command = CMD_ACK;

         return stat >> CHAR_OFFSET;
     }

}

static unsigned int tx_status(volatile termreg_t *tp){
    return ((tp->transm_status) & TERM_STATUS_MASK);
}

static unsigned int rx_status(volatile termreg_t *tp){
    return ((tp->recv_status) & TERM_STATUS_MASK);
}
