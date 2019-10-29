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

#include "../H/tlb.h"

/*==============================================================================
                            TLB HANDLER
==============================================================================*/
void tlbHandler(){
    //handle trap exceprions
    state_t *old_area = (state_t *)OLD_AREA_TLB;
    old_area->pc_epc += WORD_SIZE;
    if (!currentProc->has_spec[TLB]) Terminate_Process(0);

    copyState(old_area, currentProc->spec_TLB_old);
    LDST(currentProc->spec_TLB_new);
}
