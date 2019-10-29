#include "umps/libumps.h"
#include "umps/arch.h"
#include "../H/terminal.h"

#define LINE_BUF_SIZE 100
#define TERMINAL_0  0
#define TERMINAL_1  1

typedef unsigned int u32;

/* read and write aux variable */
static char buf[LINE_BUF_SIZE];

/* disk characteristics*/
static char data0[LINE_BUF_SIZE];
unsigned int MIN_HEAD =  0;
unsigned int MAX_HEAD =  1;
unsigned int MIN_SECT = 0;
unsigned int MAX_SECT = 7;

u32 head = 0;
u32 sect = 0;
int flag = 1;
int currentTerminal;

/* read Terminal i*/
static void readline(char *buf, unsigned int count, int currentTerminal){
    int c;
    while (--count && (c = term_getchar(currentTerminal)) != '\n')
        *buf++ = c;
    *buf = '\0';
}

static void halt(void){
    WAIT();
    *((volatile unsigned int *) MCTL_POWER) = 0x0FF;
    while (1) ;
}


void main(void){

    term_puts("****************************\n",TERMINAL_0);
    term_puts("* Hi, this is ChaTerminal! *\n",TERMINAL_0);
    term_puts("****************************\n",TERMINAL_0);
    term_puts("If you want that Leo save something on the disk insert 0 during the chat\n",TERMINAL_0);
    term_puts("You are --> Matteo \nStart!\n",TERMINAL_0);

    term_puts("****************************\n",TERMINAL_1);
    term_puts("* Hi, this is ChaTerminal! *\n",TERMINAL_1);
    term_puts("****************************\n",TERMINAL_1);
    term_puts("If you want that Leo save something on the disk insert 0 during the chat\n",TERMINAL_1);
    term_puts("You are --> Leo \nWait for your turn (after Matteo)!\n",TERMINAL_1);

    currentTerminal = TERMINAL_0;

    while (1){
        term_puts(">", currentTerminal);
        readline(buf, LINE_BUF_SIZE,currentTerminal);
        if (currentTerminal==TERMINAL_0) currentTerminal = TERMINAL_1;
        else currentTerminal = TERMINAL_0;


        /*if he inserted '0' request the other terminal to insert something to save*/
        /* write to the disk */
        if(atoi(buf) == 0){
          term_puts("Insert a value to be stored (string, number, etc...)\n",currentTerminal);
          term_puts("--> ",currentTerminal);
          readline(data0, LINE_BUF_SIZE,currentTerminal);
          term_puts("\n",currentTerminal);

          term_puts("Stored data: ", (currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);
          term_puts(data0,(currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);
          term_puts("\n", (currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);

          /* set disk head number */
          while(flag) {
             term_puts("Insert head number (0-1)\n",currentTerminal);
             term_puts("--> ",currentTerminal);
             readline(buf, LINE_BUF_SIZE,currentTerminal);
             head = atoi(buf);
             if (head >= MIN_HEAD && head <= MAX_HEAD) flag = 0;
          }
          term_puts("\n",currentTerminal);
          term_puts("Head number is: ", (currentTerminal==TERMINAL_1) ? TERMINAL_0 : 1);
          term_puts(buf,(currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);
          term_puts("\n", (currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);
          flag = 1;

          /* set disk sector number */
          while(flag) {
            term_puts("Insert sector number (0-7)\n",currentTerminal);
            term_puts("--> ",currentTerminal);
            readline(buf, LINE_BUF_SIZE,currentTerminal);
            sect = atoi(buf);
            if (sect >= MIN_SECT && sect <= MAX_SECT) flag = 0;
          }
          term_puts("\n",TERMINAL_0);
          term_puts("Sector number is: ", (currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);
          term_puts(buf,(currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);
          term_puts("\n", (currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);
          flag = 1;


          do {
          term_puts("You want to save? Insert 1 for yes, 0 otherwise.\n", (currentTerminal==TERMINAL_1) ? TERMINAL_0 : 1);
          readline(buf, LINE_BUF_SIZE,(currentTerminal==TERMINAL_1) ? TERMINAL_0 : TERMINAL_1);
          } while (atoi(buf) != 1 && atoi(buf) != 0);

          if(atoi(buf)==1){
            if (store(data0,head,sect))
                term_puts("OK, data saved!\n",currentTerminal);
           else
                term_puts("Error!\n",currentTerminal);

          /* read from the disk */
          term_puts("Stored value\n",currentTerminal);
          term_puts("--> ",currentTerminal);
          term_puts(read(head,sect),currentTerminal);
          term_puts("\nContinue the chat\n",currentTerminal);
          }
      }else{
        /*continue the chat*/
        term_puts("-->",currentTerminal);
        term_puts(buf,currentTerminal);
        term_puts("\n",currentTerminal);
      }
 }

 halt();
}
