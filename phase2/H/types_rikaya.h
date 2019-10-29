#ifndef _TYPES11_H
#define _TYPES11_H

#include <umps/types.h>
#include "listx.h"
#include "const.h"

typedef unsigned int memaddr;

/* Process Control Block (PCB) data structure */
typedef struct pcb_t {
    /*process queue fields */
    struct list_head p_next;

    /*process tree fields */
    struct pcb_t *p_parent;
    struct list_head p_child, p_sib;

    bool tutor;
    /* processor state, etc */
    state_t p_s;

    /* process priority */
    int	priority;
    int	original_priority;

    /* key of the semaphore on which the process is eventually blocked */
    int	*p_semkey;

    /* Time management */
    int p_ttime;
    int p_tuser_start;
    int p_tuser_tot;
    int p_tkernel_start;
    int p_tkernel_tot;

    /* Special exceptions management */
    state_t *spec_TLB_old;
    state_t *spec_TLB_new;
    state_t *spec_PgmTrap_old;
    state_t *spec_PgmTrap_new;
    state_t *spec_Sys_old;
    state_t *spec_Sys_new;
    bool has_spec[3];

    /* IO command */
    unsigned int device_command;
    unsigned int subdevice_command;
} pcb_t;


/* Semaphore Descriptor (SEMD) data structure */
typedef struct semd_t {
	struct list_head	s_next;
	
	// Semaphore key
	int					*s_key;
	
	// Queue of PCBs blocked on the semaphore
	struct list_head	s_procQ;
} semd_t;

typedef struct semdev {
	 semd_t disk[DEV_PER_INT];
	 semd_t tape[DEV_PER_INT];
	 semd_t network[DEV_PER_INT];
	 semd_t printer[DEV_PER_INT];
	 semd_t terminalR[DEV_PER_INT];
	 semd_t terminalT[DEV_PER_INT];
} semdev;

#endif
