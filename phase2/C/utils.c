/**************************** Module UTILS.c *****************************
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

#include "../H/utils.h"

/* FIX THIS VERSION */
void initSemaphore(int sem[N_DEV_PER_IL][N_EXT_IL - 1], int value, int i, int j){
  for(int y = 0; y < i; y++){
    for(int z = 0; z < j; z++){
      sem[y][z] = value;
    }
  }
}
/* FIX THIS VERSION */
void initSemaphore_Term(int sem[N_DEV_PER_IL][2], int value, int i, int j){
    for(int y = 0; y < i; y++){
        for(int z = 0; z < j; z++){
            sem[y][z] = value;
        }
    }
}

void setIntervalTimer(int time){
  /* ???????? */
  INTERVAL_TIMER = (U32)time;
}

/*
  Creates a new processor state for the ROM new areas.
  Defines the register status and PC and stores the new state in the supplied physical address.
  handler = memory address of the value of PC
  area = physical addres in which state is stores
  return = VOID
*/
void initArea(memaddr handler, memaddr area){
    state_t *state = (state_t *)area;

    setStatus(state, IEC_OFF, KUC_ON, MASK_NEW_PO, MASK_INT_OFF, VMC_OFF, TIMER_ON);
    state->entry_hi = 0;
    state->cause = 0;
    state->hi = 0;
    state->lo = 0;
    for(int i = 0; i < STATE_GPR_LEN; i++)
      state->gpr[i] = 0;

    setPC_SP(state, handler, RAMTOP);
}

/*
  In register reg, turns ON bit(s) indicated by the bitmask
  shifted of offset position to the left
  reg = pointer to register to be modified
  bitmask = bitmask applied
  offset = number of shifted bit begin from right
  return = VOID
*/
void setBitmask(unsigned int* reg, int bitmask, int offset){
  *reg |= (bitmask << offset);
}


/*
  Set state field to given values
  state = pointer to the state variabile to modify
  interrupt = 0 interrupt disable;
              1 interrupt enable;
  mode = 0 kernel mode;
         1 user mode;
  intMask = 8 bit value idicates which interrupt line is enable
  virtualMemory = 0 virtualMemory disable;
                  1 virtualMemory enable;
  timer = 0 timer disable;
          1 timer enable;
  return = VOID
*/
void setStatus(state_t* state, int IEc, int KUc, int maskPO, int intMask, int VMc, int timer){
  //set status register
  state->status = 0;
  //set IEc,KUc,IEp,KUp,IEo,KUo
  if(IEc) state->status |= (1 << OFFSET_IEC);
  if(KUc) state->status |= (1 << OFFSET_KUC);

  // IEp = 1, KUp = IEo = KUo = 0
  setBitmask(&(state->status), maskPO, OFFSET_IEP);

  //set interrupt mask
  setBitmask(&(state->status), intMask, OFFSET_INT);

  //set VMc, timer
  if(VMc) state->status |= (1 << OFFSET_VMC);
  if(timer) state->status |= (1 << OFFSET_TIMER);

  //set CPU id
  state->status |= CPU_1 << OFFSET_CPU;
  //state->status = 0x800FF03;// 1 << 2 | 1 << 27 | 0xFF00;
}

/*
  Set program counter and stack pointer of state to a given values
  state = pointer to the state to modify
  pc = new program counter value
  stackPointer = new stack pointer value
  retun = VOID
*/
void setPC_SP(state_t* state, int pc, int stackPointer){
  state->pc_epc = pc;
  state->reg_sp = stackPointer;
}


/*
  Copy state value from src to dest
  src = pointer to source state
  dest = pointer to destination state
  return = VOID
*/
void breakpointCopyState(){}
void copyState(state_t* src, state_t * dest){
    dest->cause = src->cause;
    dest->entry_hi = src->entry_hi;
    for(int i = 0; i < STATE_GPR_LEN; i++)
      dest->gpr[i] = src->gpr[i];
    dest->hi = src->hi;
    dest->lo = src->lo;
    dest->pc_epc = src->pc_epc;
    dest->status = src->status;
    breakpointCopyState();
}
void waitforAck(unsigned int status) {
    while ((status & STATUSMASK) != DEV_S_READY);
}

void waitforCommand(unsigned int *status){
    unsigned int c;
    int stop = 1;
    while(stop){
        c = (*(status)) & 0x0000000f;
        if (c == 3) WAIT();
        else stop = 0;
    }
}