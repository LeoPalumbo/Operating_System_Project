/**************************** Module HANDLER.c *****************************
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

#include "../H/syscall.h"
extern int pseudo_clock_sem;

state_t *old_area;

/*
 * Get exception code from cause register
 */
unsigned int getCauseExcCode(unsigned int cause){
    return (((cause) & CAUSE_MASK) >> OFFSET_EXECODE);
}

/* ===================================================================================================================
 *                                                  SYSTEM-CALL HANDLER                                              *
 * ================================================================================================================= */
void syscallHandler(){
    old_area = (state_t *)OLD_AREA_SYSCALL;
    /* Get exception cause from old area */
    int excCode = getCauseExcCode(old_area->cause);
    unsigned int ret;

    if(excCode == SYSTEM_CALL) {
        /* aux variables declaration */
        state_t *statep;
        int priority;
        memaddr pid;
        unsigned int cpid;

        /* time management */
        if (currentProc->p_tuser_start != 0) {
            currentProc->p_tuser_tot += TOD_LO - currentProc->p_tuser_start;
            currentProc->p_tuser_start = 0;
        }
        currentProc->p_tkernel_start = TOD_LO;

        /* get syscall code from register a0 */
        int numberOfSyscall = old_area->reg_a0;
        switch (numberOfSyscall) {
            case GETCPUTIME:
                Get_Cpu_Time((unsigned int *) old_area->reg_a1, (unsigned int *) old_area->reg_a2,
                             (unsigned int *) old_area->reg_a3);
                break;

            case CREATEPROCESS:
                statep = (state_t *) old_area->reg_a1;
                priority = (int) old_area->reg_a2;
                Create_Process(statep, priority, (void **)old_area->reg_a3);
                break;

            case TERMINATEPROCESS:
                Terminate_Process((void **)old_area->reg_a1);
                break;

            case VERHOGEN:
                Verhogen((int *) old_area->reg_a1);
                break;

            case PASSEREN:
                Passeren((int *) old_area->reg_a1);
                break;

            case WAITCLOCK:
                Passeren(&pseudo_clock_sem);
                break;

            case WAITIO:
                ret = Do_Io((unsigned int) old_area->reg_a1, (memaddr) old_area->reg_a2,
                            (unsigned int) old_area->reg_a3);
                break;

            case SETTUTOR:
                Set_Tutor();
                break;

            case SPECPASSUP:
                Specpassup((int) old_area->reg_a1, (state_t *) old_area->reg_a2, (state_t *) old_area->reg_a3);
                break;

            case GETPID:
                Get_Pid_Ppid((void **)old_area->reg_a1, (void **)old_area->reg_a2);
                break;

            default:
                /* If a superior handler is not defined, terminates the process */
                if (!currentProc->has_spec[SYSBP]){
                    Terminate_Process(0);
                    return;
                }

                old_area->pc_epc += WORD_SIZE;
                copyState(old_area, currentProc->spec_Sys_old);
                /* load superior handler */
                LDST(currentProc->spec_Sys_new);

        }
    }
    else if(excCode == BREAKPOINT) {
        /* If a superior handler is not defined, terminates the process */
        if (!currentProc->has_spec[SYSBP]) Terminate_Process(0);
        old_area->pc_epc += WORD_SIZE;
        copyState(old_area, currentProc->spec_Sys_old);
        /* load superior handler */
        LDST(currentProc->spec_Sys_new);
    }
    /* Time management */
    currentProc->p_tkernel_tot += TOD_LO - currentProc->p_tkernel_start;
    currentProc->p_tkernel_start = 0;
    currentProc->p_tuser_start = TOD_LO;

    old_area->reg_v0=ret;
    old_area->pc_epc += WORD_SIZE;

    /* continue execution, after increase PC */
    LDST(old_area);
}
/* ===================================================================================================================*/

/*=============================================== GET_CPU_TIME =======================================================*/
/*
 * Returns Usert time, Kernel time and Total time
 */
void Get_Cpu_Time(unsigned int *user_time, unsigned int *kernel_time, unsigned int *wallclock_time){

    /* Update kernel time */
    currentProc->p_tkernel_tot += TOD_LO - currentProc->p_tkernel_start;;
    currentProc->p_tkernel_start = TOD_LO;

    /* return value if requested */
    if(user_time)
        *user_time = currentProc->p_tuser_tot;

    if(kernel_time)
        *kernel_time = currentProc->p_tkernel_tot;

    if(wallclock_time)
        *wallclock_time = TOD_LO - currentProc->p_ttime;
}

/*=============================================== CREATE_PROCESS =====================================================*/
/*
 * Create a child process for the current process.
 *
 * statep = will be the state assign to the new process (PC, SP, status ...)
 * priority = will be the priority (and original priority) of the new process
 * cpid = contains the new process's address
 * return = 0 successful
 *         -1 otherwise
 */
int Create_Process(state_t *statep, int priority, void ** cpid){
    pcb_t *new_proc = allocPcb();

    if(new_proc != NULL){
        /* new process allocated successfully */
        /* Set new_proc state */
        copyState(statep, &new_proc->p_s);

        /* Set priority and original priority */
        setPriority(new_proc, priority);

        /* Set new_proc as a child of the current process */
        insertChild(currentProc, new_proc);

        /* Time management and scheduler */
        new_proc->p_ttime = TOD_LO;
        insertProcReady(new_proc);

        if (cpid)
            *((pcb_t **)cpid) = new_proc;

        return 0;
    } else
        return -1;
}

/*============================================ TERMINATE_PROCESS =====================================================*/
/*
 * Terminate process indicated by pid. If pid == NULL, current process is terminated.
 * Terminated process must be a descendant of the current process.
 * Terminated process's children are inherited by a tutor process.
 *
 * pid = address of the process to be removed
 * return = 1, success
 *         -1, error status
 */
int Terminate_Process(void **pid){
    pcb_t *tmp = NULL;
    pcb_t *tutor = NULL;
    pcb_t* pointerTO = NULL;
    bool allowed = FALSE;
    tmp = (pid) ? *((pcb_t **)pid) : currentProc;
    pointerTO = tmp;


    /* rm_proc has no parent, so it is the root process */
    /* return error status */
    if (tmp->p_parent == NULL)
        return -1;

    /* Checks if rm_proc is a descendant of the current process */
    /* If tmp == NULL, rm_proc is not a descendant of the current process */
    while (tmp != NULL && !allowed){
        if (tmp == currentProc) allowed = TRUE;
        else tmp = tmp->p_parent;
    }

    if (!allowed)
        return -1;

    /* Search for a tutor process that will inherit rm_proc's children */
    /* Tutor will contain a pointer to a tutor PCB. */
    /* Root (init) process is always a tutor process */
    tmp = pointerTO;
    tmp = tmp->p_parent;
    while ((tutor == NULL) && (tmp->p_parent != NULL)){
        if (tmp->tutor) tutor = tmp;
        else tmp = tmp->p_parent;
    }

    /* Move all rm_proc's children to tutor */
    tmp = pointerTO;
    pcb_t* child;
    /* se il proc da eliminare ha figli o no */
    bool isEmpty = false;
    while (!isEmpty){
        child = removeChild(tmp);
        if (child != NULL) insertChild(tutor, child);
        else isEmpty = true;
    }

    tmp = pointerTO;

    /* Removes rm_proc from the semaphore on which it is eventually blocked */
    outBlocked(tmp);

    /* Removes rm_proc from the ready queue, if it is present */
    /* if rm_proc == current process, current process = NULL */
    removeProcReady(tmp);
    freePcb(tmp);

    /* Eventually selects next process */
     if (currentProc == NULL)
        scheduler();
    return 0;
}

/*============================================== VERHOGEN ============================================================*/
/*
 * Performs a Verhogen operation on the given semaphore.
 * Increases semaphore value, so a process could be awake and insert into the ready queue.
 */
pcb_t *Verhogen(int *semval){
    *semval += 1;

    pcb_t *tmp;
    if(*semval <= 0){
        /* wakeup a process from the queue of semaphore[semval] */
        tmp = removeBlocked(semval);
        if (tmp) {
            /* If a process was blocked */
            /* insert the wakeup process in ready queue */
            insertProcReady(tmp);
            return tmp;
        }
    }
    return NULL;
}

/*============================================== PASSEREN ============================================================*/

/*============================= vPASSEREN =======================================*/
/*
 * This function is invoked every time a process needs an eventually blocked operation on a semaphore.
 * First, it decreases semaphore value.
 * If semaphore value < 0, process is blocked; otherwise process is not blocked on the semaphore.
 *
 * proc = process that request a blocked operation
 * semval = semaphore
 * return = TRUE, if process is NOT blocked
 *          FALSE, if process is blocked
 */
bool vPasseren(pcb_t* proc, int *semval) {
    *semval -= 1;

    if (*semval < 0) {
        outProcQ(&ready_queue, proc);
        if (proc != currentProc){
            /* If proc is NOT current Process */
            /* insert process in blocked process queue[semval] */
            insertBlocked(semval, proc);
            /*ATTENTION: MAYBE TIMER SHOULD BE RE-SET IN A DIFFERENT WAY*/
        } else {
            /* If proc is current Process */
            /* Insert process in blocked process queue[semval] */
            insertBlocked(semval, currentProc);

            /* save curret state of execution, increases PC  */
            copyState(old_area, &currentProc->p_s);
            currentProc->p_s.pc_epc += WORD_SIZE;

            /* time management */
            currentProc->p_tkernel_tot += TOD_LO - currentProc->p_tkernel_start;
            currentProc->p_tkernel_start = 0;

            /* Selects the next process */
            currentProc = NULL;
            scheduler();
        }
        return FALSE;
    } return TRUE;
}

/*============================= PASSEREN =======================================*/
/*
 * Executed a Passern operation on the given semaphore.
 *
 * Return = TRUE, if current process is not blocked;
 *          FALSE, otherwise.
 */
bool Passeren(int *semval){
    return(vPasseren(currentProc, semval));
}

/*============================================== WAIT_CLOCK ==========================================================*/
void Wait_Clock(){
    Passeren(&pseudo_clock_sem);
}

/*============================================== DO_IO ===============================================================*/
/*
 * Every time a process needs a I/O operation raises this sys-call.
 * Stores the command and request operation.
 *
 * command = command which will be executed
 * reg = memory address of the selected device
 * sud_dev = only for terminal, indicates if it is a transmitting (0) or receiving (1) operation.
 */
unsigned int Do_Io(unsigned int command, memaddr reg, unsigned int sub_dev){
    /* Store the requested command in the pcb variable */
    /* subdevice_command is used only if device is a terminal */
    currentProc->device_command = command;
    currentProc->subdevice_command = sub_dev;

    /* Request the I/O operation*/
    return request_IO(currentProc, reg, sub_dev);
}

/*============================================ SET_TUTOR =============================================================*/
void Set_Tutor(){
    currentProc->tutor = true;
}

/*=========================================== SPEC_PASSUP ============================================================*/
/*
 * This function id used to specify a superior handler for a process.
 * It must be called AT MOST ONCE for each type of exception (SYS/BREAKPOINT, TLB, PROGRAM TRAP)
 *
 * type = type of the specified exception
 * old = old area to store the current state of execution
 * new = new area to load that will handle the exception
 * return = 0, Success to set the handler
 *         -1, otherwise.
 */
int Specpassup(int type, state_t *old, state_t *new){
    /* Handler was already defined for that type. Failure */
    if(currentProc->has_spec[type])
        return -1;

    currentProc->has_spec[type] = TRUE;
    /* Set old and new ares */
    switch (type){
        case SYSBP:
            currentProc->spec_Sys_old = old;
            currentProc->spec_Sys_new = new;
            break;
        case TLB:
            currentProc->spec_TLB_old = old;
            currentProc->spec_TLB_new = new;
            break;
        case PROGRAM_TRAP:
            currentProc->spec_PgmTrap_old = old;
            currentProc->spec_PgmTrap_new = new;
            break;
        default:
            break;
    }
    return 0;
}

/*============================================ GET_PID_PPID ==========================================================*/
void Get_Pid_Ppid(void ** pid, void ** ppid){
    if(pid != 0)
        *pid = currentProc;
    if(ppid != 0)
        *ppid = currentProc->p_parent;
}
