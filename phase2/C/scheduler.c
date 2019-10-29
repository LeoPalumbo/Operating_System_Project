/**************************** Module SCHEDULER.c *****************************
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
#include "../H/scheduler.h"

//MS = milliseconds per process
#define MS 3000
#define TIME_SLICE (TIME_SCALE * MS)

/*
 * Insert a process for the first time into the ready queue (time management added)
 */
void scheduler_add(pcb_t *process){
  process->p_tkernel_start = TOD_LO;
  insertProcQ(&ready_queue, process);
}

/*
 * Selects the next process to be executed from the queue of ready processes.
 * If ready queue is empty and no other process is executing, idle is performed.
 * Ready queue is order by priority (decreasing order), so the next process
 * to be executed is always the first in the queue (with highest priority value).
 * The selected process is removed from the ready queue and load in to the
 * processor with LDST.
 * All the remaining processes in the queue are aged.
 * Timer is setted (and acknowledged).
 * ready_queue = queue of ready processes
 * return = VOID
 */
void scheduler(){
    if(emptyProcQ(&ready_queue) && (currentProc==NULL)){
      while(1);
    } else {
        /* context switch occurs */
        if(currentProc != NULL){

          /* time management */
            if(currentProc->p_tkernel_start != 0){
              currentProc->p_tkernel_tot += TOD_LO - currentProc->p_tkernel_start;
              currentProc->p_tkernel_start = 0;
            }

          insertProcReady(currentProc);      
        }

    currentProc = removeProcQ(&ready_queue);
    currentProc->p_tuser_start = TOD_LO;

    aging();

    setTIMER(TIME_SLICE);
    LDST(&currentProc->p_s);
 }
}

/*
 * Restore the original priority of newProc.
 * Insert a process newProc in the queue of ready process.
 * ready_queue = queue of ready processes
 * newProc = process to be added to the queue of ready processes
 * return = void
 */
void insertProcReady(struct pcb_t* newProc){
  restorePriority(newProc);
  insertProcQ(&ready_queue, newProc);
}

/*
 * Removes rmProc from ready queue.
 * If rmProc is the current running process, currentProc becomes NULL
 * ready_queue = queue of ready processes
 * rmProc = process to be removed
 * return = removed process
 */
void removeProcReady(struct pcb_t* rmProc){
    outProcQ(&ready_queue, rmProc);
    if(rmProc == currentProc) currentProc = NULL;
}


/*
 * Increases, by one, the priority of all the processes in the ready queue, expect the idle process.
 * ready_queue = queue of ready processes
 * return = VOID
 */
void aging(){
    struct list_head* pos;
    struct pcb_t* p;

    if (emptyProcQ(&ready_queue) == FALSE){
        list_for_each(pos, &ready_queue){
            p = container_of(pos, struct pcb_t, p_next);

            /* not aging the idle process */
            if(p->original_priority != -1)
                increasePriority(p);
        }
    }
}

