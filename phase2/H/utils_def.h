#ifndef UTILS_DEF
#define UTILS_DEF

#include <umps/libumps.h>
#include <umps/arch.h>
#include <umps/types.h>


/* ============================ OLD/NEW AREAS ========================================= */
#define FRAMESIZE 4096
#define NEW_AREA_SYSCALL 0x200003d4
#define OLD_AREA_SYSCALL 0x20000348
#define NEW_AREA_TRAP 0x200002bc
#define OLD_AREA_TRAP 0x20000230
#define NEW_AREA_TLB 0x200001a4
#define OLD_AREA_TLB 0x20000118
#define NEW_AREA_INTERRUPT 0x2000008c
#define OLD_AREA_INTERRUPT 0x20000000

/* ============================ STATUS REGISTER ======================================== */
#define OFFSET_IEC 0
#define OFFSET_KUC 1
#define OFFSET_IEP 2
#define OFFSET_KUP 3
#define OFFSET_IEO 4
#define OFFSET_KUO 5
#define OFFSET_INT 8
#define OFFSET_VMC 22
#define OFFSET_TIMER 27
#define OFFSET_CPU 28

//interrupt current
#define IEC_ON 1
#define IEC_OFF 0

//kernel-user mode
#define KUC_ON 1
#define KUC_OFF 0

//virtual memory
#define VMC_ON 1
#define VMC_OFF 0

//timer
#define TIMER_ON 1
#define TIMER_OFF 0

//Mask
//0111 1111 = all interrupt unmasked, 11111111
#define INT_MASK 0xFF
//0001 = mask process previus and older bit. IEp unmasked,IEo, KUp and KUo masked
#define MASK_PROC_PO 0x1
//0000 = mask new area previus and older bit. IEp, IEo, KUp and KUo masked
#define MASK_NEW_PO 0x0
//all interrupt are disable
#define MASK_INT_OFF 0x00
//0001 = first processor is used
#define CPU_1 0x1

/* ============================ CAUSE REGISTER ========================================= */
#define OFFSET_EXECODE 2
#define CAUSE_MASK 0x7C //0111 1100

//interrupt lines
#define INTER_PROCESSOR 0
#define PROCESSOR_LOCAL_TIMER 1
#define BUS_INTERVAL_TIMER 2
#define DISK_DEVICE 3
#define TAPE_DEVICE 4
#define NETWORK_DEVICE 5
#define PRINTER_DEVICE 6
#define TERMINAL_DEVICE 7

/* ============================ DEVICE COMMANDS ========================================= */
#define TRANSMITTED 5
#define TRANSTATUS 2
#define ACK 1
#define PRINTCHR 2
#define CHAROFFSET 8
#define STATUSMASK 0xFF
#define TERM0ADDR 0x10000250
#define DEVREGSIZE 16
#define READY 1
#define DEVREGLEN 4
#define TRANCOMMAND 3
#define BUSY 3

/* ============================== SYSCALL ============================================== */
#define SYSTEM_CALL 8
#define BREAKPOINT 9

#define GET_CPU_TIME 1
#define CREATE_PROCESS 2
#define TERMINATE_PROCESS 3
#define VERHOGEN 4
#define PASSEREN 5
#define WAIT_CLOCK 6
#define DO_IO 7
#define SET_TUTOR 8
#define SPEC_PASSUP 9


#define TOD_LO *((unsigned int *)0x1000001C)
#define TIME_SCALE *((unsigned int *)0x10000024)
#define RAMBASE *((unsigned int *)0x10000000)
#define RAMSIZE *((unsigned int *)0x10000004)
//#define RAMTOP (RAMBASE + RAMSIZE)

#endif
