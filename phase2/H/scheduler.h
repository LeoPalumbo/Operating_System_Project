/**************************** Module SCHEDULER.h *****************************
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
#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <umps/libumps.h>
#include <umps/arch.h>
#include <umps/types.h>

#include "const.h"
#include "listx.h"
#include "utils.h"
#include "utils_def.h"
#include "pcb.h"

extern struct list_head ready_queue;
extern struct pcb_t* currentProc;

void scheduler();
void insertProcReady(struct pcb_t* newProc);
void removeProcReady(struct pcb_t* rmProc);
void aging();
void scheduler_add(pcb_t *process);
#endif
