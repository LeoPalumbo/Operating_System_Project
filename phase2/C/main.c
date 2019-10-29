/**************************** Module MAIN.c *****************************
*                                                                      *
* Copyright (C) 2019 Matteo Mele, Leonardo Pio Palumbo                 *
*                                                                      *
* This program is free software: you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation, either version 3 of the License, or    *
* (at your option) any later version.                                  *
*                                                                      *
* This program is distributed in the hope that it will be useful,      *
* but WITHOUT ANY WARRANTY; without even the implied warranty of       *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
* GNU General Public License for more details.                         *
*                                                                      *
* You should have received a copy of the GNU General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>. *
*                                                                      *
***********************************************************************/
#include <umps/libumps.h>
#include <umps/arch.h>
#include <umps/types.h>

#include "../H/p2test_rikaya.h"
#include "../H/const.h"
#include "../H/listx.h"
#include "../H/asl.h"
#include "../H/interrupts.h"
#include "../H/syscall.h"
#include "../H/tlb.h"
#include "../H/trap.h"
#include "../H/scheduler.h"
#include "../H/utils_def.h"

#define SP (RAMTOP-FRAME_SIZE)
#define SP2 (RAMTOP-FRAME_SIZE *2)

struct pcb_t *currentProc=NULL;
struct list_head ready_queue;
int idleProcess(){
	while(1)
		WAIT();
}
int main(){
    /* Initialize new Areas */
    initArea((memaddr) syscallHandler, (memaddr) NEW_AREA_SYSCALL);
    initArea((memaddr) trapHandler, (memaddr) NEW_AREA_TRAP);
    initArea((memaddr) tlbHandler, (memaddr) NEW_AREA_TLB);
    initArea((memaddr) interruptHandler, (memaddr) NEW_AREA_INTERRUPT);

    /* Initialize data structures */
    initPcbs();
    initASL();
    INIT_LIST_HEAD(&ready_queue);
    interruptInit();

    /* Initialize test and idle processes */
    pcb_t *test1, *idle;

    test1 = allocPcb();
    setStatus(&test1->p_s, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
    setPC_SP(&test1->p_s,(memaddr)test, SP);
    setPriority(test1, 1);
    test1->tutor = TRUE;

    idle = allocPcb();
    setStatus(&idle->p_s, IEC_ON, KUC_OFF, MASK_PROC_PO, INT_MASK, VMC_OFF, TIMER_ON);
    setPC_SP(&idle->p_s, (memaddr)idleProcess, SP2);
    setPriority(idle, -1);

    /* Running test */
    scheduler_add(test1);
    scheduler_add(idle);
    scheduler();
    
    return 0;
}
