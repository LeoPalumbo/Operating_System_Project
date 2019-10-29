/**************************** Module ASL.c *****************************
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


#include "../H/asl.h"

//aux functions
HIDDEN int *getKey(struct list_head* pos);
HIDDEN void initSemd(struct semd_t* semd, int* key);
HIDDEN void setProcKey(struct pcb_t* p, int* semdkey);
HIDDEN void checkEmptySemd(struct semd_t* semd);

//active semaphore list - ASL
HIDDEN struct list_head asl;
//free semaphore list - semdFree
HIDDEN struct list_head semdFree;
//sentinel node of ASL
HIDDEN struct list_head *semd_h;
//sentinel node of semdFree list
struct list_head *semdFree_h;
static struct semd_t semdTable[MAXPROC];

/***********************************************************************/

/***********************************************************************
 *                                                                     *
 *                        AUXILIARY FUNCTIONS                          *
 *                                                                     *
 ***********************************************************************/

/***********************************************************************
 *                                                                     * 
 * pos = list_head of the corresponding semaphore                      *
 * return = Semaphore key                                              *
 *                                                                     *
 ***********************************************************************/
 HIDDEN int* getKey(struct list_head* pos){
   return container_of(pos, struct semd_t, s_next)->s_key;
 }


/**********************************************************************
 *                                                                    *
 * Adds a new semaphore to the ASL with a specified key               *
 * semd = pointer to the semaphore to be added                        *
 * key = specified key                                                *
 * return = void                                                      *
 *                                                                    *
 **********************************************************************/
 HIDDEN void initSemd(struct semd_t* semd, int* key){
   list_add_tail(&semd->s_next, semd_h);
   semd->s_key = key;
 }

/**********************************************************************
 *                                                                    *
 * Given a process pointed by p, modifies its semdkey to the          *
 * specified value                                                    *
 * p = pointer to a process                                           *
 * semdKey = new value for p_semKey                                   *
 * return = void                                                      *
 *                                                                    *
 **********************************************************************/
 HIDDEN void setProcKey(struct pcb_t* p, int* semdkey){
   p->p_semkey = semdkey;
 }


/*********************************************************************
 *                                                                   *
 * If semd procQ is empty, semd is removed from ASL and add to       *
 * semdFree list                                                     *
 * semd = pointer to a semaphore                                     *
 * return = void                                                     *
 *                                                                   *
 *********************************************************************/
 HIDDEN void checkEmptySemd(struct semd_t* semd){
   if(list_empty(&semd->s_procQ)){
     list_del(&semd->s_next);
     list_add_tail(&semd->s_next,semdFree_h);
   }
 }

 /*********************************************************************/

 /**********************************************************************
  *                                                                    *
  *                        ASL FUNCTIONS                               *
  *                                                                    *
  **********************************************************************/


/***********************************************************************
* Given a key value, returns the semaphore associated with that key,   *
* if exists in ASL, NULL otherwise.                                    *
*                                                                      *
* key = specified key value of the semaphore                           *
* return = semaphore associated with key.                              *
*          NULL, if key = NULL                                         *
*          NULL, if a semaphore with that list doesn't exist in ASL    *
*                                                                      *
***********************************************************************/
struct semd_t* getSemd(int *key){
  struct list_head* pos;
  struct semd_t* semd = NULL;

  //valid key
  if(key != NULL){
    list_for_each(pos, semd_h){
      if(getKey(pos) == key)

        //get semd pointed by pos
        semd = container_of(pos,struct semd_t, s_next);
    }
  }

  return semd;
}



/***********************************************************************
 * The process pointed by p is added to the process queue of the       *                                                              *
 * semaphore associated with key. If the semaphore is not in the ASL,  *
 * it is allocated from semdFree list if possibile.                    *
 *                                                                     *
 * key = specified key value of the semaphore                          *
 * p = pointer to process to be added to semaphore process queue       *
 * return = TRUE, if is not possible to allocate a new semaphore       *
 *          FALSE, otherwise                                           *
 *                                                                     *
 ***********************************************************************/
int insertBlocked(int *key, pcb_t* p){
  struct semd_t* semd = getSemd(key);

  //invalid semd
  if(semd == NULL){
    //not available semaphore
    if(list_empty(semdFree_h)) return TRUE;
    else {
      //pick a new semd from semdFree list, init it with proper key
      semd = container_of(list_next(semdFree_h), struct semd_t, s_next);
      list_del(list_next(semdFree_h));
      initSemd(semd, key);
    }
  }
  //add process to the semd's s_procQ
  list_add_tail(&p->p_next, &semd->s_procQ);

  //set process semdKey to the key of the semaphore on which the process is blocked
  setProcKey(p, key);

  return FALSE;
}

/***********************************************************************
 *                                                                     *
 * Given a semaphore key, removes and returns the first process        *
 * blocked in that semaphore, if it exists. If the semaphore process   *
 * queue is now empty, removes the semaphore from the ASL and adds it  *
 * to semdFree list. Sets the semdkey of the removed process to NULL.  *
 *                                                                     *
 * key = specified key value of the semaphore                          *
 * return = fisrt process blocked on the semaphore associated with key *
 *          NULL, if a semaphore with s_key = key doesn't exist        *
 *                                                                     *
 ***********************************************************************/
 struct pcb_t *removeBlocked(int *key){
  struct semd_t* semd = getSemd(key);
  struct pcb_t* rmvProc = NULL;
  struct list_head* pos = NULL;

  if(semd != NULL){

    //get first process from semd s_procQ
    pos = list_next(&semd->s_procQ);
    rmvProc = container_of(pos, struct pcb_t, p_next);

    list_del(pos);

    //remove semd from ASL, if necessary
    checkEmptySemd(semd);

    //unlocked process gets null as semdkey
    setProcKey(rmvProc,NULL);
  }

  return rmvProc;
}


/***********************************************************************
 *                                                                     *
 * Removes and returns the process pointed by p from the process queue *
 * of the semaphore on which the process is blocked. If the semaphore  *
 * process queue is now empty, removes the semaphore from the ASL and  *
 * adds it to semdFree list. Sets the semdkey of the removed process   *
 * to NULL.                                                            *
 *                                                                     *
 * p = pointer to the process to be removed                            *
 * return = process p                                                  *
 *          NULL, if there is no semaphore with s_key = key            *
 *          NULL, if p is not blocked                                  *
 *                                                                     *
 ***********************************************************************/
struct pcb_t* outBlocked(pcb_t *p){
  struct semd_t* semd = getSemd(p->p_semkey);
  struct list_head* pos;

  //process is not blocked
  if(semd == NULL) return NULL;
  else {
    //process is blocked in the semd's s_procQ
    pos = &p->p_next;
    list_del(pos);

    //remove semd from ASL, if necessary
    checkEmptySemd(semd);

    //unlocked process gets null as semdkey
    setProcKey(p,NULL);

    return p;
  }
}

/***********************************************************************
 *                                                                     *
 * Returns the first process of the Semaphore's process queue          *
 * associated with key.                                                *
 *                                                                     *
 * key = specified key value of the semaphore                          *
 * return = first process blocked on the semaphore                     *
 *        NULL, if semaphore with that key is not present in the ASL   *
 *                                                                     *
 ***********************************************************************/
struct pcb_t* headBlocked(int *key){
  struct semd_t* semd = getSemd(key);
  struct list_head* pos = NULL;
  struct pcb_t* firstProc = NULL;

  //semd is in ASL
  if(semd != NULL){

    //get first process of semd s_procQ
    pos = list_next(&semd->s_procQ);
    firstProc = container_of(pos, struct pcb_t, p_next);
  }

  return firstProc;
}

/***********************************************************************
 *                                                                     *
 * Removes the process pointed by from the process queue of the        *
 * semaphore on which it is blocked. furthermore, removes all the      *
 * processes rooted in p from the process queue of the semaphores      *
 * on which they are blocked                                           *
 *                                                                     *
 * p = pointer to process to be removed                                *
 * return = void                                                       *
 *                                                                     *
 ***********************************************************************/
void outChildBlocked(pcb_t *p){
   /* Eliminiamo il pcb indicato da p */
   struct pcb_t *parent_pcb = outBlocked(p);

   /* se p ha anche dei figli */
   if (!(list_empty (&parent_pcb->p_child))){
     /* punta alla sentinella della lista dei figli */
     struct list_head *tmp_list=&parent_pcb->p_child;
     struct list_head *pos;
     for (pos=(tmp_list)->next; pos != tmp_list; pos=pos->next){
	    /* ripetiamo la outChildBlocked sulla lista dei suoi figli */
      struct pcb_t *tmp=container_of(pos,struct pcb_t,p_sib);
      outChildBlocked(tmp);
     }
  }
}

/***********************************************************************
 *                                                                     *
 * Inizializes semdFree in order to contain all the elements from      *
 * semdTable. This method is invoked only once to inizialize data      *
 * structures                                                          *
 *                                                                     *
 ***********************************************************************/
void initASL(){
  struct semd_t* tmpSemd;

  INIT_LIST_HEAD(&asl);
  INIT_LIST_HEAD(&semdFree);
  semdFree_h = &semdFree;
  semd_h = &asl;


  for(int i = 0; i < MAXPROC; ++i){
    tmpSemd = &(semdTable[i]);

    /* field setting */
    tmpSemd->s_key = NULL;
    list_add(&tmpSemd->s_next, semdFree_h);
    INIT_LIST_HEAD(&tmpSemd->s_procQ);

  }
}
