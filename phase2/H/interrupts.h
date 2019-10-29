/**************************** Module INTERRUPTS.h *****************************
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
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <umps/libumps.h>
#include "pcb.h"
#include "asl.h"
#include "scheduler.h"
#include "syscall.h"
#include "utils_def.h"
#include "const_rikaya.h"
#include "utils_def.h"

void interruptInit();
HIDDEN unsigned int getIPline(unsigned int cause, int line);
HIDDEN bool get_device_pending(int line, int device);
HIDDEN void find_dev(unsigned int add, int *lineInt, int *devInt);

void interruptHandler();
void lineHandler(state_t* oldArea, int line, int device);

void contextSwitch(state_t* oldArea);
void intervalTimer();
unsigned int request_IO(pcb_t *pcb, memaddr reg, unsigned int subdev);
void handling_IO(int line, int device);
void terminalHandler();
void subDeviceHandler(termreg_t* termIO, unsigned int status, unsigned int device, unsigned int subdevice);

#endif


