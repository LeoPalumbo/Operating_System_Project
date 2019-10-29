/**************************** Module INTERRUPTS.c *****************************
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
#include "../H/interrupts.h"

#define CLOCK_TIME_MS (TIME_SCALE * 100)
#define MS 3000
#define TIME_SLICE (TIME_SCALE * MS)
#define TRM_CMD_ACK 1

#define INTER_DEVICES_BASE 0x1000003C
#define INTER_DEVICES(line) (INTER_DEVICES_BASE + (line - 3) * WS)

/* if true indicates that a process local timer interrupt is pending */
HIDDEN bool PLT = 0;

/* DEVICE MANAGEMENT STRUCTURES */
/* Array pf pcb which have sent a command to a device and are waiting for termination */
HIDDEN pcb_t *waitToComplete_proc[N_DEV_PER_IL][N_EXT_IL - 1];
/* Array of semaphore for I/O queue of device (terminals excluded) */
HIDDEN int waitDevice_semd[N_DEV_PER_IL][N_EXT_IL - 1];

/* TERMINALS MANAGEMENT STRUCTURES */
/* Array pf pcb which have sent a command to a terminal and are waiting for termination */
HIDDEN pcb_t *waitToCompleteTerminal_proc[N_DEV_PER_IL][2];
/*
 * Array of semaphore for I/O terminal, each terminal has 2 sub-devices (for receiving and for transmitting)
 * -> waitTerminal[i][1] is the i-th terminal's semaphore for receiving sub-device
 * -> waitTerminal[i][0] is the i-th terminal's semaphore for transmitting sub-device
 */
HIDDEN int waitTerminal_semd[N_DEV_PER_IL][2];

int pseudo_clock_sem = 0;

/* ===================================================================================================================
 *                                             AUXILIARY FUNCTIONS                                                   *
 * ================================================================================================================= */
void interruptInit(){
    initSemaphore(waitDevice_semd, 1, N_DEV_PER_IL, N_EXT_IL - 1);
    initSemaphore_Term(waitTerminal_semd, 1, N_DEV_PER_IL, 2);
    setIntervalTimer(CLOCK_TIME_MS*1000);
}

/*
 * Given a line, determines if an interrupt is pending.
 * casue = register cause
 * line = 0-7 interval for devices
 * return = 1 if interrupt is pending of the line supplied; 0 otherwise
 */
HIDDEN unsigned int getIPline(unsigned int cause, int line){
    //perchè gli interrupt iniziano dall'8 bit in poi. quindi se vuoi vedere sulla linea 2 devi vedere il 2+8=10imo bit
    line += OFFSET_INT;
    return cause & (1 << line);
}

/*
 * Given a line and a device,
 * Return: 1 if that device raises an interrupt on the supplied line, 0 otherwise.
 */
HIDDEN bool get_device_pending(int line, int device) {
    return *((unsigned int *)INTER_DEVICES(line)) << (1 << device);
}

/*
 * Given a memory address of device register, computes the corresponding line and device
 * that generated the interrupt. Results are stored in the supplied variables.
 * return = VOID
 *
 * FIX THIS VERSION
 */
HIDDEN void find_dev(unsigned int add, int *lineInt, int *devInt){
    int val;
    int line, sub;

    for(line = 3; line <= 7; ++line) {
        for (sub = 0; sub <= 7; ++sub) {
            // Given an interrupt line (IntLineNo) and a device number (DevNo) one can
            // compute the starting address of the device’s device register:
            // devAddrBase = 0x1000.0050 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10) 
            // pagina 32
            val = 0x10000050 + ((line - 3) * 0x80) + (sub * 0x10);

            if (val == add) {
                *lineInt = line;
                *devInt = sub;
                return;
            }
        }
    }
    /* ---> FIX BETTER VERSION
     * switch(add)
     *  case add <= xx && add >= yy: *lineINT = zz
     *  case add ...
     * break;
     *
     * for sub0 to sub7 -> sub_i
     *     if (add = 0x10000050 + ((*lineINT - 3) * 0x80) + (sub_i * 0x10) *sub = sub_i;
     */
}
/* ===================================================================================================================
 *                                                  INTERRUPT HANDLER                                                *
 * ================================================================================================================= */

/*
 * Every time an interrupt is raised this handler is called. It determines which line and device raise the interrupt
 * and calls the specific handler.
 */
void interruptHandler(){
    /* Time management */
    if (currentProc->p_tuser_start != 0) {
        currentProc->p_tuser_tot += TOD_LO - currentProc->p_tuser_start;
        currentProc->p_tuser_start = 0;
    }

    state_t *oldAreaInt = (state_t *) OLD_AREA_INTERRUPT;

    /* For each device line checks if there is a pending interrupt */
    for(int line = INTER_PROCESSOR; line <= TERMINAL_DEVICE; line++) {
        if (getIPline(oldAreaInt->cause, line)) {

            /* not used, but handled */
            if (line == INTER_PROCESSOR);

            else if (line == BUS_INTERVAL_TIMER) intervalTimer();

            /* Context Switch handling */
            else if (line == PROCESSOR_LOCAL_TIMER) {
                /* this variable is TRUE iff the only interrupt pending from processor local timer */
                unsigned int contextSwitch_only = getCAUSE() & 0x0000ff00;
                if (contextSwitch_only == TRUE) {
                    contextSwitch(oldAreaInt);
                } else PLT = 1;

                /* Disk, tape, network and printer device handling */
            } else if (line >= DISK_DEVICE && line < TERMINAL_DEVICE) {
                for (int device = 0; device < 8; device++) {
                    if (get_device_pending(line, device)) {
                        handling_IO(line, device);
                    }
                }
            } else if (line >= TERMINAL_DEVICE)
                /* Terminal handling */
                terminalHandler();
        }
    }

    currentProc->p_tuser_start = TOD_LO;
    if (PLT){
        PLT = 0;
        contextSwitch(oldAreaInt);
    }
    else LDST(oldAreaInt);
}

/*========================================== INTER_PROCESSOR =========================================================*/
/* not used */

/*========================================= PROCESSOR_LOCAL_TIMER ====================================================*/
void contextSwitch(state_t* oldArea){
    //save the current processor state in the pcb state
    copyState(oldArea, &currentProc->p_s);
    scheduler();
}

/*============================================ BUS_INTERVAL_TIMER ====================================================*/
void intervalTimer(){
    /* Remove all the blocked pcbs in the semaphore and reinsert into the ready queue */
    while(Verhogen(&pseudo_clock_sem) != NULL);

    /* ACK */
    setIntervalTimer(CLOCK_TIME_MS*1000);
}

/*========================================== DISK, TAPE, NETWORK, PRINTER ============================================*/
/*
 * This function is called when a process needs an I/O operation with a device.
 * Given the memory address of the device, its line and device are computed and the corresponding semaphore.
 *
 * If the semphore is free, command is executed and pcb moved into a waitToComplete.
 * else, pcb is blocked in the I/O queue.
 * (Terminals are handled different respect the other devices).
 *
 * pcb = process that request I/O operation
 * reg = memory address of the device
 * subdev = only for terminals, indicated if it is a transmitting (0) o receiving (1) operation
 */
unsigned int request_IO(pcb_t *pcb, memaddr reg, unsigned int subdev) {
    unsigned int status;
    int sem;
    int line, dev;

    /* Compute line and subdevice from register address */
    find_dev(reg, &line, &dev);

    if (line != TERMINAL_DEVICE) {

        dtpreg_t *dev_register = (dtpreg_t *)reg;

        /* Checks if device is available */
        /* if it is not, pcb added to the queue of waiting proc by the vPasseren */
        if (vPasseren(pcb, &waitDevice_semd[line][dev])) {
            /* device is available */

            waitToComplete_proc[line][dev] = pcb;
            outProcQ(&ready_queue, pcb);


            /* start executing command */
            dev_register->command = pcb->device_command;
            waitforCommand(&dev_register->status);
            status = dev_register->status;
        }
    } else {
        /* Handling terminal device */

        termreg_t *term_register = (termreg_t *)reg;

        /* Checks if terminal is available */
        /* if it is not, pcb added to the queue of waiting proc by the vPasseren */
        if (vPasseren(pcb, &waitTerminal_semd[dev][subdev])) {
            /* device is available */

            waitToCompleteTerminal_proc[dev][subdev] = pcb;
            outProcQ(&ready_queue, pcb);

            if (subdev == TRANSMITTION) {
                /* start executing command */
                term_register->transm_command = pcb->device_command;
                waitforCommand(&term_register->transm_status);
                status = term_register->transm_status;
            } else if (subdev == RECEIVING) {
                /* start executing command */
                term_register->recv_command = pcb->device_command;
                waitforCommand(&term_register->recv_status);
                status = term_register->recv_status;
            }
        }
    }
    return status;
}


/*
 * This function is called when a device (not terminal) raises an interrupt.
 * This means that an I/O operation is completed, so the blocked process must be inserted into the ready queue,
 * semaphore value must be updated and another command must be executed is there is some process in
 * the I/O queue for that device. If the I/O queue is empty, device should be acknowledge.
 *
 * n_line = device line
 * n_device = subdevice number
 */
void handling_IO(int n_line, int n_device) {

    dtpreg_t *device = (dtpreg_t *) DEV_REG_ADDR(n_line, n_device);
    pcb_t *next_process, *terminatedIO_proc;

    /* Blocked process is inserted into the ready queue */
    terminatedIO_proc = waitToComplete_proc[n_line][n_device];
    insertProcReady(terminatedIO_proc);
    waitToComplete_proc[n_line][n_device]=NULL;
    /* Checks if other process is waiting for a I/O operation */
    if (waitDevice_semd[n_line][n_device]) {
        /* Selects next process */
        next_process = Verhogen(&waitDevice_semd[n_line][n_device]);

        waitforAck(device->status);
        device->command = DEV_C_ACK;

        request_IO(next_process, (memaddr) device, 0);
    }
}

/*=============================================== TERMINAL_DEVICE ====================================================*/
void terminalHandler() {
    for (int i = 0; i <= 7; i++) {
        if (get_device_pending(7, i)) {

            /*save the pending Terminal register */
            termreg_t *termIO = (termreg_t *) DEV_REG_ADDR(7, i);
            /*Save Both transmitting and receiving status */
            unsigned int trasm_status = (termIO->transm_status);
            unsigned int rcv_status = (termIO->recv_status);

            /* For purposes of prioritizing pending interrupts, terminal transmission (i.e. writing
                to the terminal) is of higher priority than terminal receipt (i.e. reading from the
                terminal), for this reason is handled first.
                */

            /*To make it possible to determine which sub-device has a pending interrupt there are two sub-device “ready” conditions:
              * Device Ready and Character Received/Transmitted.
              * Updating status is necessary only if the operation is completed (with success or error) but device
              * is not ready yet, otherwise the interrupt is already handled and the device is ready to serve. */

            subDeviceHandler(termIO, trasm_status, i, TRANSMITTION);
            subDeviceHandler(termIO, rcv_status, i, RECEIVING);
        }
    }
}

/*
 * subDeviceHandler is the function that handle the Terminal Subdevices interrupt;
 *
 * termIO = point to the register of the device that has a pending interrupt;
 * status = contains the status of the subdevice to handle;
 * device = is the number of the device that issues the interrupt;
 * subdevice = is an integer that specify which sub-device handle;
 *
 */
void subDeviceHandler(termreg_t *termIO, unsigned int status, unsigned int device, unsigned int subdevice) {
    pcb_t *next_process, *terminatedIO_proc;

    if (status != DEV_S_BUSY && status != DEV_S_NOT_INSTALLED && status != DEV_S_READY) {
        /* Wake up the process that was waiting for the completion of the interrupt */
        if (waitToCompleteTerminal_proc[device][subdevice] != NULL) {
            terminatedIO_proc = waitToCompleteTerminal_proc[device][subdevice];
            if (terminatedIO_proc != currentProc) insertProcQ(&ready_queue, terminatedIO_proc);
            waitToCompleteTerminal_proc[device][subdevice] = NULL;
        }

        /* Acking command */
        switch (subdevice) {
            case TRANSMITTION:
                termIO->transm_command = TRM_CMD_ACK;
                waitforAck(termIO->transm_status);
                break;

            case RECEIVING:
                termIO->recv_command = TRM_CMD_ACK;
                waitforAck(termIO->recv_status);
                break;

            default:
                PANIC();
        }

        /*Check for processes that are requiring the i-th terminal transmitting sub-device */
        if (waitTerminal_semd[device][subdevice] < 0) {
            next_process = Verhogen(&waitTerminal_semd[device][subdevice]);
            request_IO(next_process, (memaddr) DEV_REG_ADDR(7, device), next_process->subdevice_command);
        } else waitTerminal_semd[device][subdevice] += 1;
    }
}