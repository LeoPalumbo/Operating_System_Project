/**************************** Module HANDLER.c *************************
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
#ifndef SYSCALL_H
#define SYSCALL_H
#include <umps/libumps.h>
#include <umps/regdef.h>

#include "p2test_rikaya.h"
#include "scheduler.h"
#include "utils_def.h"
#include "utils.h"
#include "const_rikaya.h"
#include "pcb.h"
#include "asl.h"
#include "interrupts.h"

#define SYSBP 0
#define TLB 1
#define PROGRAM_TRAP 2

unsigned int getCauseExcCode(unsigned int cause);
void syscallHandler();
pcb_t *Verhogen(int *semval);
bool Passeren(int *semval);
bool vPasseren(pcb_t* proc, int *semval);
void Wait_Clock();
void Set_Tutor();
unsigned int Do_Io(unsigned int command, memaddr reg, unsigned int sub_dev);
void Get_Pid_Ppid(void ** pid, void ** ppid );
void Get_Cpu_Time(unsigned int *user_time, unsigned int *kernel_time, unsigned int *wallclock_time);
int Create_Process(state_t *statep, int priority, void ** cpid);
int Terminate_Process(void ** pid);
int Specpassup(int type, state_t *old, state_t *new);
#endif
