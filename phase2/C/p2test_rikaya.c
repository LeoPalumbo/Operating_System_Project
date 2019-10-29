/*****************************************************************************
 * Copyright 2019, Renzo Davoli, Mattia Maldini                              *
 * Copyright 2004, 2005 Michael Goldweber, Davide Brini.                     *
 *****************************************************************************/

/*********************************P2TEST.C*******************************
 *
 *	Test program for the Rikaya Kernel: phase 2.
 *
 *	Produces progress messages on Terminal0.
 *	
 *	This is pretty convoluted code, so good luck!
 *
 *		Aborts as soon as an error is detected.
 *
 *      Modified by Michael Goldweber on May 15, 2004
 *      Modified by Davide Brini on Nov 26, 2004
 *      Modified by Renzo Davoli 2010
 *      Modified by Miro Mannino on May 8, 2010
 *      Modified by Mattia Maldini, Renzo Davoli 2019
 */

#include "../H/const_rikaya.h"
#include "../H/types_rikaya.h"
#include <umps/libumps.h>
#include <umps/arch.h>
#include "../H/p2test_rikaya.h"
#include "../H/utils_def.h"
#include "../H/utils.h"

typedef unsigned int devregtr;
typedef unsigned int cpu_t;
typedef unsigned int pid_t;

/* if these are not defined */
/* typedef U32 cpu_t; */
/* typedef U32 pid_t; */

/* hardware constants */
#define PRINTCHR	2
#define BYTELEN	8
#define RECVD	5
#define TRANSM 5

#define STATUS_ALL_INT_ENABLE(x)    (x | (0xFF << 8))

#define CLOCKINTERVAL	100000UL	/* interval to V clock semaphore */

#define TERMSTATMASK	0xFF
#define TERMCHARMASK	0xFF00
#define CAUSEMASK		0xFF
#define VMOFF 			0xF8FFFFFF

#define KUPBITON		0x8			//nota bene
#define KUPBITOFF		0xFFFFFFF7

#define MINLOOPTIME		10000
#define LOOPNUM 		10000

#define CLOCKLOOP		10
#define MINCLOCKLOOP	3000	

#define BADADDR			0xFFFFFFFF /* could be 0x00000000 as well */
#define TERM0ADDR       0x10000250


/* Software and other constants */
#define CREATENOGOOD	-1
#define TERMINATENOGOOD	-1

#define ON        1
#define OFF       0

/* just to be clear */
#define SEMAPHORE		S32
#define NOLEAVES		4	/* number of leaves of p8 process tree */
#define MAXSEM			20

#define DEFAULT_PRIORITY    1


SEMAPHORE term_mut=1,	/* for mutual exclusion on terminal */
					s[MAXSEM+1],	/* semaphore array */
					testsem=0,		/* for a simple test */
					startp2=0,		/* used to start p2 */
					endp2=0,		/* used to signal p2's demise */
					endp3=0,		/* used to signal p3's demise */
					blkp4=1,		/* used to block second incaration of p4 */
					synp4=0,		/* used to allow p4 incarnations to synhronize */
					endp4=0,		/* to signal demise of p4 */
					endp5=0,		/* to signal demise of p5 */
					endp8=0,		/* to signal demise of p8 */
					endcreate=0,	/* for a p8 leaf to signal its creation */
					blkp8=1;		/* to block p8 */

state_t p2state, p3state, p4state, p5state,	p6state, p7state;
state_t p8rootstate, interchildstate, child1state, child2state;
state_t gchild1state, gchild2state, gchild3state, gchild4state;

/* trap states for p5 */
state_t pstat_n, mstat_n, sstat_n, pstat_o,	mstat_o, sstat_o;

int		p1p2synch = 0;	/* to check on p1/p2 synchronization */

int 	p8inc;			/* p8's incarnation number */ 
int		p4inc=1;		/* p4 incarnation number */

unsigned int p5Stack;	/* so we can allocate new stack for 2nd p5 */

int creation = 0; 				/* return code for SYSCALL invocation */
memaddr *p5MemLocation = (memaddr*)0x34;		/* To cause a p5 trap */

pid_t p4pid;
pid_t testpid;
pid_t childpid, intermediatepid, p8pid;

void	p2(),p3(),p4(),p5(),p5a(),p5b(),p6(),p7(),p7a(),p5prog(),p5mm();
void	p5sys(),p8root(),child1(),child2(),p8leaf(),curiousleaf(), intermediate();
void breakpointV(){}
void breakpointP2(){}
void breakpointP(){}
void afterCreateProcess(){}
void beforeCreateProcess(){}
void breakpointGET(){}


/* a procedure to print on terminal 0 */
breakpointprint(){}
void print(char *msg) {
	unsigned int command;
	char * s = msg;
	devregtr * base = (devregtr *) (TERM0ADDR);
	devregtr status;

	SYSCALL(PASSEREN, (int)&term_mut, 0, 0);				/* get term_mut lock */

	while (*s != '\0') {
		/* Put "transmit char" command+char in term0 register (3rd word). This 
			 actually starts the operation on the device! */
		command = PRINTCHR | (((devregtr) *s) << BYTELEN);

		/* Wait for I/O completion (SYS8) */
		status = SYSCALL(WAITIO, command, (int)base, FALSE);
		breakpointprint();
		/*		PANIC(); */

		if ((status & TERMSTATMASK) != TRANSM)
			HALT();

		if (((status & TERMCHARMASK) >> BYTELEN) != *s)
			PANIC();
			

		s++;	
	}

	SYSCALL(VERHOGEN, (int)&term_mut, 0, 0);				/* release term_mut */
}


/*                                                                   */
/*                 p1 -- the root process                            */
/*
 * */
void breakpointEnteredInTest(){}
void breakpointp3wasstarted(){}
void test() {	
    
	SYSCALL(VERHOGEN, (int)&testsem, 0, 0);					/* V(testsem)   */
	if (testsem != 1) { print("error: p1 v(testsem) with no effects\n"); PANIC(); }

	print("p1 v(testsem)\n");
breakpointEnteredInTest();
	/* set up states of the other processes */

	/* set up p2's state */


	STST(&p2state);			/* create a state area using p1's state    */
    setStatus(&p2state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	/* stack of p2 should sit above ??????  */
	p2state.reg_sp = p2state.reg_sp - FRAME_SIZE;

	/* p2 starts executing function p2 */
	p2state.pc_epc = (memaddr)p2;

	/* p2 has interrupts on and unmasked */
	p2state.status = STATUS_ALL_INT_ENABLE(p2state.status);


	/* Set up p3's state */
	STST(&p3state);
    setStatus(&p3state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);

	/* p3's stack is another 1K below p2's one */
	p3state.reg_sp = p2state.reg_sp - FRAME_SIZE;
	p3state.pc_epc = (memaddr)p3;
	p3state.status = STATUS_ALL_INT_ENABLE(p3state.status);


	/* Set up p4's state */
	STST(&p4state);
    setStatus(&p4state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	/* p4's stack is another 1k below p3's one */
	p4state.reg_sp = p3state.reg_sp - FRAME_SIZE;
	p4state.pc_epc = (memaddr)p4;
	p4state.status = STATUS_ALL_INT_ENABLE(p4state.status);

	/* Set up p5's state */
	STST(&p5state);
    setStatus(&p5state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	/* because there will be two p4s running*/
	/* Record the value in p5stack */
	p5Stack = p5state.reg_sp = p4state.reg_sp - (2 * FRAME_SIZE);
	p5state.pc_epc = (memaddr)p5;
	p5state.status = STATUS_ALL_INT_ENABLE(p5state.status);

	/* Set up p6's state */
	STST(&p6state);
    setStatus(&p6state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	/* Ther will be two p5s ???? */
	p6state.reg_sp = p5state.reg_sp - (2 * FRAME_SIZE);
	p6state.pc_epc = (memaddr)p6;
	p6state.status = STATUS_ALL_INT_ENABLE(p6state.status);


	/* Set up p7's state */
	STST(&p7state);
    setStatus(&p7state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	/* Only one p6 */
	p7state.reg_sp = p6state.reg_sp - FRAME_SIZE;
	p7state.pc_epc = (memaddr)p7;
	p7state.status = STATUS_ALL_INT_ENABLE(p7state.status);

	STST(&p8rootstate);
    setStatus(&p8rootstate, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	p8rootstate.reg_sp = p7state.reg_sp - FRAME_SIZE;
	p8rootstate.pc_epc = (memaddr)p8root;
	p8rootstate.status = STATUS_ALL_INT_ENABLE(p8rootstate.status);

	STST(&child1state);
    setStatus(&child1state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	child1state.reg_sp = p8rootstate.reg_sp - FRAME_SIZE;
	child1state.pc_epc = (memaddr)child1;
	child1state.status = STATUS_ALL_INT_ENABLE(child1state.status);

	STST(&interchildstate);
    setStatus(&interchildstate, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	interchildstate.reg_sp = p8rootstate.reg_sp - FRAME_SIZE;
	interchildstate.pc_epc = (memaddr)intermediate;
	interchildstate.status = STATUS_ALL_INT_ENABLE(interchildstate.status);

	STST(&child2state);
    setStatus(&child2state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	child2state.reg_sp = child1state.reg_sp - FRAME_SIZE;
	child2state.pc_epc = (memaddr)child2;
	child2state.status = STATUS_ALL_INT_ENABLE(child2state.status);

	STST(&gchild1state);
    setStatus(&gchild1state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	gchild1state.reg_sp = child2state.reg_sp - FRAME_SIZE;
	gchild1state.pc_epc = (memaddr)curiousleaf;
	gchild1state.status = STATUS_ALL_INT_ENABLE(gchild1state.status);

	STST(&gchild2state);
    setStatus(&gchild2state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	gchild2state.reg_sp = gchild1state.reg_sp - FRAME_SIZE;
	gchild2state.pc_epc = (memaddr)p8leaf;
	gchild2state.status = STATUS_ALL_INT_ENABLE(gchild2state.status);

	STST(&gchild3state);
    setStatus(&gchild3state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	gchild3state.reg_sp = gchild2state.reg_sp - FRAME_SIZE;
	gchild3state.pc_epc = (memaddr)p8leaf;
	gchild3state.status = STATUS_ALL_INT_ENABLE(gchild3state.status);

	STST(&gchild4state);
    setStatus(&gchild4state, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	gchild4state.reg_sp = gchild3state.reg_sp - FRAME_SIZE;
	gchild4state.pc_epc = (memaddr)p8leaf;
	gchild4state.status = STATUS_ALL_INT_ENABLE(gchild4state.status);


	/* create process p2 */
	SYSCALL(CREATEPROCESS, (int)&p2state, DEFAULT_PRIORITY, 0);				/* start p2     */


	print("p2 was started\n");
	
	breakpointV();
	SYSCALL(VERHOGEN, (int)&startp2, 0, 0);					/* V(startp2)   */
	breakpointP();
	
	/* P1 blocks until p2 finishes and Vs endp2 */
	SYSCALL(PASSEREN, (int)&endp2, 0, 0);					/* P(endp2)     */
	breakpointP();
	
	/* make sure we really blocked */
	if (p1p2synch == 0)
		print("error: p1/p2 synchronization bad\n");

    beforeCreateProcess();

	SYSCALL(CREATEPROCESS, (int)&p3state, DEFAULT_PRIORITY, 0);				/* start p3  */
    afterCreateProcess();
	breakpointp3wasstarted();
    print("p3 is started\n");
	/* P1 blocks until p3 ends */
	SYSCALL(PASSEREN, (int)&endp3, 0, 0);					/* P(endp3)     */


	SYSCALL(CREATEPROCESS, (int)&p4state, DEFAULT_PRIORITY, (int)&p4pid);		/* start p4     */

	SYSCALL(CREATEPROCESS, (int)&p5state, DEFAULT_PRIORITY, 0); 		/* start p5     */

	SYSCALL(CREATEPROCESS, (int)&p6state, DEFAULT_PRIORITY, 0);		/* start p6		*/

	SYSCALL(CREATEPROCESS, (int)&p7state, DEFAULT_PRIORITY, 0);		/* start p7		*/

	SYSCALL(PASSEREN, (int)&endp5, 0, 0);

	print("p1 knows p5 ended\n");

	SYSCALL(PASSEREN, (int)&blkp4, 0, 0);					/* P(blkp4)		*/

	/* now for a more rigorous check of process termination */
	for (p8inc = 0; p8inc < 4; p8inc++) {
		creation = SYSCALL(CREATEPROCESS, (int)&p8rootstate, DEFAULT_PRIORITY, 0);

		if (creation == CREATENOGOOD) {
			print("error in process termination\n");
			PANIC();
		}

		SYSCALL(PASSEREN, (int)&endp8, 0, 0);
	}

	print("\n");

	print("p1 finishes OK -- TTFN\n");
	* ((memaddr *) BADADDR) = 0;				/* terminate p1 */

	/* should not reach this point, since p1 just got a program trap */
	print("error: p1 still alive after progtrap & no trap vector\n");
	PANIC();					/* PANIC !!!     */
}


/* p2 -- semaphore and cputime-SYS test process */
void p2() {
	int		i;				       /* just to waste time  */
	cpu_t	now1,now2;		   /* times of day        */
	cpu_t	user_t1, user_t2;	 /* user time used       */
	cpu_t	kernel_t1, kernel_t2;	 /* kernel time used       */
	cpu_t	wallclock_t1, wallclock_t2;	 /* wallclock time used       */

	/* startp2 is initialized to 0. p1 Vs it then waits for p2 termination */
	SYSCALL(PASSEREN, (int)&startp2, 0, 0);				/* P(startp2)   */
    breakpointP2();
	print("p2 starts\n");

	/* initialize all semaphores in the s[] array */
	for (i = 0; i <= MAXSEM; i++) s[i] = 0;

	/* V, then P, all of the semaphores in the s[] array */
	for (i = 0; i <= MAXSEM; i++)  {
		SYSCALL(VERHOGEN, (int)&s[i], 0, 0);			/* V(S[I]) */
		SYSCALL(PASSEREN, (int)&s[i], 0, 0);			/* P(S[I]) */
		if (s[i] != 0) print("error: p2 bad v/p pairs\n");
	}

	print("p2 v/p pairs successfully\n");

	/* test of SYS6 */
	breakpointGET();

	now1 = getTODLO();                  				/* time of day   */
	SYSCALL(GETCPUTIME, (int)&user_t1, (int)&kernel_t1, (int)&wallclock_t1);			/* CPU time used */

	/* delay for several milliseconds */
	for (i = 1; i < LOOPNUM; i++)
		;

	SYSCALL(GETCPUTIME, (int)&user_t2, (int)&kernel_t2, (int)&wallclock_t2);			/* CPU time used */
	now2 = getTODLO();				/* time of day  */

	if (((user_t2 - user_t1) >= (kernel_t2 - kernel_t1)) &&
			((wallclock_t2 - wallclock_t1) >= (user_t2 - user_t1)) &&
			((now2 - now1) >= (wallclock_t2 - wallclock_t1)) &&
			((user_t2 - user_t1) >= MINLOOPTIME)) {
		print("p2 is OK\n");
	} else {
		if ((user_t2 - user_t1) < (kernel_t2 - kernel_t1))
			print ("warning: here kernel time should be less than user time\n");
		if ((wallclock_t2 - wallclock_t1) < (user_t2 - user_t1))
			print ("error: more cpu time than wallclock time\n");
		if ((now2 - now1) < (wallclock_t2 - wallclock_t1))
			print ("error: more wallclock time than real time\n");
		if ((user_t2 - user_t1) < MINLOOPTIME)
			print ("error: not enough cpu time went by\n");
		print("p2 blew it!\n");
	}

	p1p2synch = 1;				/* p1 will check this */

	SYSCALL(VERHOGEN, (int)&endp2, 0, 0);				/* V(endp2)     */

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);			/* terminate p2 */

	/* just did a SYS2, so should not get to this point */
	print("error: p2 didn't terminate\n");
	PANIC();					/* PANIC! */
}


/* p3 -- clock semaphore test process */
void breakpointp3(){}
void p3() {
    breakpointp3();
	cpu_t	time1, time2;
	cpu_t	kernel_t1,kernel_t2;	/* cpu time used */
	int	i;

	time1 = 0;
	time2 = 0;

	/* loop until we are delayed at least half of clock V interval */
	while ((time2 - time1) < (CLOCKINTERVAL >> 1) )  {
		time1 = getTODLO();		/* time of day     */
        SYSCALL(WAITCLOCK, 0, 0, 0);
		time2 = getTODLO();			/* new time of day */
	}

	print("p3 - WAITCLOCK OK\n");

	/* now let's check to see if we're really charged for CPU
		 time correctly */
	SYSCALL(GETCPUTIME, 0, (int)&kernel_t1, 0);

	for (i = 0; i < CLOCKLOOP; i++)
		SYSCALL(WAITCLOCK, 0, 0, 0);

	SYSCALL(GETCPUTIME, 0, (int)&kernel_t2, 0);

	if ((kernel_t2 - kernel_t1) < MINCLOCKLOOP)
		print("error: p3 - CPU time incorrectly maintained\n");
	else
		print("p3 - CPU time correctly maintained\n");

	SYSCALL(VERHOGEN, (int)&endp3, 0, 0);				/* V(endp3)        */

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);			/* terminate p3    */

	/* just did a SYS2, so should not get to this point */
	print("error: p3 didn't terminate\n");
	PANIC();					/* PANIC  */
}


void breakpointp4(){};
/* p4 -- termination test process and getpid test */
void p4() {
	pid_t pid;
    pid_t p42id;

	switch (p4inc) {
		case 1:
			print("first incarnation of p4 starts\n");
			SYSCALL(GETPID, (int)&pid, 0, 0);
			if (p4pid != pid){
				print("error: createprocess or getpid are wrong\n");
				PANIC();
			}
			p4inc++;
			break;
		case 2:
			print("second incarnation of p4 starts\n");
			SYSCALL(GETPID, 0, (int)&pid, 0);
			if (p4pid != pid){
				print("error: createprocess or getppid are wrong\n");
				PANIC();
			}

			break;
	}
	SYSCALL(VERHOGEN, (int)&synp4, 0, 0);				/* V(synp4)     */

	/* first incarnation made blkp4=0, the second is blocked (blkp4 become -1) */
	SYSCALL(PASSEREN, (int)&blkp4, 0, 0);				/* P(blkp4)     */

	SYSCALL(PASSEREN, (int)&synp4, 0, 0);				/* P(synp4)     */

	/* start another incarnation of p4 running, and wait for  */
	/* a V(synp4). the new process will block at the P(blkp4),*/
	/* and eventually, the parent p4 will terminate, killing  */
	/* off both p4's.                                         */

	p4state.reg_sp -= FRAME_SIZE;		/* give another page  */

	print("p4 create a new p4\n");
	SYSCALL(CREATEPROCESS, (int)&p4state, DEFAULT_PRIORITY, (int) &p42id);			/* start a new p4    */

	SYSCALL(PASSEREN, (int)&synp4, 0, 0);				/* wait for it       */
	print("p4 termination of the child\n");
	breakpointp4();
	if (SYSCALL(TERMINATEPROCESS, (int) &p42id, 0, 0) < 0) {			/* terminate p4      */
		print("error: terminate process is wrong\n");
		PANIC();
	}

	print("p4 is OK\n");

	SYSCALL(VERHOGEN, (int)&endp4, 0, 0);				/* V(endp4)          */

	print("p4 termination after the child\n");

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);			/* terminate p4      */

	/* just did a SYS2, so should not get to this point */
	print("error: p4 didn't terminate\n");
	PANIC();					/* PANIC            */
}



/* p5's program trap handler */
void breakingp5prog(){}
void p5prog() {
	unsigned int exeCode = pstat_o.cause;
	exeCode = (exeCode & CAUSEMASK)>>2;
    breakingp5prog();
	switch (exeCode) {
		case EXC_BUSINVFETCH:
			print("pgmTrapHandler - Access non-existent memory\n");
            setStatus(&pstat_o, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
			pstat_o.pc_epc = (memaddr)p5a;   /* Continue with p5a() */
			break;

		case EXC_RESERVEDINSTR:
			print("pgmTrapHandler - privileged instruction - set kernel mode on\n");
			print("p5 - try call sys13 in kernel mode for verify pass up\n");
			/* return in kernel mode */
			pstat_o.status = pstat_o.status | 0xF;
            setStatus(&pstat_o, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
			pstat_o.pc_epc = (memaddr)p5b;
			break;

		case EXC_ADDRINVLOAD:
			print("pgmTrapHandler - Address Error: KSegOS w/KU=1\n");
			/* return in kernel mode */
			pstat_o.status = pstat_o.status | 0xF;
            setStatus(&pstat_o, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
			pstat_o.pc_epc = (memaddr)p5b;
			break;

		default:
			print("pgmTrapHandler - other program trap\n");
	}
    setStatus(&pstat_o, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	LDST(&pstat_o);  /* "return" to old area (that changed meanwhile) */
}

/* p5's memory management (tlb) trap handler */
/* void p5mm(unsigned int cause) { */
void p5mm() {
	print("memory management (tlb) trap - set user mode on\n");
	mstat_o.status = mstat_o.status & 0xFFFFFFF0;  /* user mode on */
	mstat_o.status &= VMOFF; /* disable VM */
	mstat_o.pc_epc = (memaddr)p5b;  /* return to p5b */
	mstat_o.reg_sp = p5Stack-FRAME_SIZE;				/* Start with a fresh stack */

	/* this is made to p5b(). printed in this point because in p5b the kernel mode is off */
	print("p5 - try call sys13 in user mode for verify pass up\n");

	LDST(&mstat_o);
}

/* p5's SYS/BK trap handler */
/* void p5sys(unsigned int cause) { */
void breakpointp5sys() {};
void p5sys() {
	unsigned int p5status = sstat_o.status;
	p5status = p5status & 0xF; 
	if(p5status){
		print("High level SYS call from kernel mode process\n");
	} else {
		print("High level SYS call from user mode process\n");
		print("p5 - try to call P in user mode\n");
	}
	breakpointp5sys();
	LDST(&sstat_o);
}
breakpointSpec1(){};
breakpointSpec2(){};
breakpointSpec3(){}
/* p5 -- SYS5 test process */
void p5() {
	print("p5 starts\n");

	/* set up higher level TRAP handlers (new areas) */
	STST(&pstat_n);  /* pgmtrap new area */
    setStatus(&pstat_n, IEC_OFF, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	pstat_n.pc_epc = (memaddr)p5prog; /* pgmtrap exceptions */

	STST(&mstat_n);  /* tlb new area */
    setStatus(&mstat_n, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	mstat_n.pc_epc = (memaddr)p5mm;   /* tlb exceptions */

	STST(&sstat_n);  /* sys/bk new area */
    setStatus(&sstat_n, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
	sstat_n.pc_epc = (memaddr)p5sys;  /* sys/bk exceptions */

	/* trap handlers should operate in complete mutex: no interrupts on */
	/* this because they must restart using some BIOS area */
	/* thus, IEP bit is not set for them (see test() for an example of it) */


	/* specify trap vectors */
	SYSCALL(SPECPASSUP, 2, (int)&pstat_o, (int)&pstat_n);
    breakpointSpec1();
	SYSCALL(SPECPASSUP, 1, (int)&mstat_o, (int)&mstat_n);
    breakpointSpec2();
	SYSCALL(SPECPASSUP, 0, (int)&sstat_o, (int)&sstat_n);
    breakpointSpec3();

	print("p5 - try to cause a pgm trap access some non-existent memory\n");
	/* to cause a pgm trap access some non-existent memory */	
	*p5MemLocation = *p5MemLocation + 1;		 /* Should cause a program trap */
}

void p5a() {
	unsigned int p5Status;

	print("p5 - try to generate a TLB exception\n");

	/* generate a TLB exception by turning on VM without setting up the 
		 seg tables */
	p5Status = getSTATUS();
	p5Status = p5Status | 0x03000000;
	setSTATUS(p5Status);
}

/* second part of p5 - should be entered in user mode */
void breakpointp5b(){};
void p5b() {
	cpu_t		time1, time2;

	SYSCALL(13, 0, 0, 0);

	/* the first time through, we are in user mode */
	/* and the P should generate a program trap */
	SYSCALL(PASSEREN, (int)&endp4, 0, 0);			/* P(endp4)*/

	/* do some delay to be reasonably sure p4 and its offspring are dead */
	time1 = 0;
	time2 = 0;
	while (time2 - time1 < (CLOCKINTERVAL >> 1))  {
		time1 = getTODLO();
		SYSCALL(WAITCLOCK, 0, 0, 0);
		time2 = getTODLO();
	}


	/* if p4 and offspring are really dead, this will increment blkp4 */

	SYSCALL(VERHOGEN, (int)&blkp4, 0, 0);			/* V(blkp4) */

	SYSCALL(VERHOGEN, (int)&endp5, 0, 0);			/* V(endp5) */

    breakpointp5b();
	print("p5 - try to redefine PGMVECT, it will cause p5 termination\n");
	/* should cause a termination       */
	/* since this has already been      */
	/* done for PROGTRAPs               */
	if (SYSCALL(SPECPASSUP, 2, (int)&pstat_o, (int)&pstat_n) == 0) {
		print("error: double SPECPASSUP should not succeed\n");
		PANIC();
	}

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
	/* should have terminated, so should not get to this point */
	print("error: p5 didn't terminate\n");
	PANIC();				/* PANIC            */
}


/*p6 -- high level syscall without initializing trap vector*/
void p6() {
	print("p6 starts\n");

	SYSCALL(13, 0, 0, 0);		/* should cause termination because p6 has no 
														 trap vector */

	print("error: p6 alive after SYS13() with no trap vector\n");

	PANIC();
}

/*p7 -- program trap without initializing passup vector*/
void p7() {
	print("p7 starts\n");

	* ((memaddr *) BADADDR) = 0;

	print("error: p7 alive after program trap with no trap vector\n");
	PANIC();
}


/* p8root -- test of termination of subtree of processes              */
/* create a subtree of processes, wait for the leaves to block, signal*/
/* the root process, and then terminate                               */
void p8root() {
	int		grandchild;

	print("p8root starts\n");

	SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
	SYSCALL(CREATEPROCESS, (int)&child1state, DEFAULT_PRIORITY, (pid_t)&childpid);
	SYSCALL(CREATEPROCESS, (int)&child2state, DEFAULT_PRIORITY, 0);

	for (grandchild=0; grandchild < NOLEAVES; grandchild++) {
		SYSCALL(PASSEREN, (int)&endcreate, 0, 0);
	}

	SYSCALL(VERHOGEN, (int)&blkp8, 0, 0);

	SYSCALL(VERHOGEN, (int)&endp8, 0, 0);

	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

/*child1 & child2 -- create two sub-processes each*/

void child1() {
	print("child1 starts\n");

	SYSCALL(SETTUTOR, 0, 0, 0);
	SYSCALL(CREATEPROCESS, (int)&interchildstate, DEFAULT_PRIORITY, (int)&intermediatepid);

	SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
	SYSCALL(VERHOGEN, (int)&blkp8, 0, 0);
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

/* Intermediary creates two processes and then dies, leaving them to the supervising tutor (child1) */

void intermediate() {
	print("intermediate starts (and dies)\n");

	SYSCALL(CREATEPROCESS, (int)&gchild1state, DEFAULT_PRIORITY, 0);
	SYSCALL(CREATEPROCESS, (int)&gchild2state, DEFAULT_PRIORITY, 0);

	SYSCALL(WAITCLOCK, 0, 0, 0);
	SYSCALL(WAITCLOCK, 0, 0, 0);
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

void child2() {
	print("child2 starts\n");

	SYSCALL(CREATEPROCESS, (int)&gchild3state, DEFAULT_PRIORITY, 0);
	SYSCALL(CREATEPROCESS, (int)&gchild4state, DEFAULT_PRIORITY, 0);

	SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
	SYSCALL(VERHOGEN, (int)&blkp8, 0, 0);
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

/*p8leaf -- code for leaf processes*/

void p8leaf() {
	print("leaf process starts\n");

	SYSCALL(VERHOGEN, (int)&endcreate, 0, 0);

	SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
	SYSCALL(VERHOGEN, (int)&blkp8, 0, 0);
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}

void curiousleaf() {
	pid_t parentid;
	print("leaf process starts\n");

	SYSCALL(GETPID, 0, (unsigned int)&parentid, 0);
	if (SYSCALL(TERMINATEPROCESS, (int)&parentid, 0, 0) == 0) {
		print("error: curiousleaf killed its parent\n");
		PANIC();
	}

	while (1) {
		SYSCALL(GETPID, 0, (int)&parentid, 0);
		if (parentid == childpid) {
			print("curiousleaf okay, intermediate died\n");
			break;
		}
		if (parentid != intermediatepid) {
			print("error: wrong parent in curiousleaf, set tutor error\n");
			PANIC();
		}
		print("curiousleaf waiting...\n");
		SYSCALL(WAITCLOCK, 0, 0, 0);
	}

	SYSCALL(VERHOGEN, (int)&endcreate, 0, 0);

	SYSCALL(PASSEREN, (int)&blkp8, 0, 0);
	SYSCALL(VERHOGEN, (int)&blkp8, 0, 0);
	SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}
