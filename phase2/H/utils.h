/**************************** Module UTILS.h *****************************
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
#ifndef UTILS_H
#define UTILS_H
#include <umps/libumps.h>
#include <umps/arch.h>
#include <umps/types.h>

#include "p2test_rikaya.h"
#include "const.h"
#include "listx.h"
#include "utils_def.h"
#include "const_rikaya.h"



void initArea(memaddr handler, memaddr area);
void setStatus(state_t* state, int IEc, int KUc, int maskPO, int intMask, int VMc, int timer);
void setPC_SP(state_t* state, int pc, int stackPointer);
unsigned int getCauseExcCode(unsigned int cause);
void copyState(state_t* src, state_t * dest);
void setIntervalTimer(int time);
void initSemaphore(int sem[N_DEV_PER_IL][N_EXT_IL - 1], int value, int i, int j);
void initSemaphore_Term(int sem[N_DEV_PER_IL][2], int value, int i, int j);
void waitforAck(unsigned int status);
void waitforCommand(unsigned int *status);
#endif

