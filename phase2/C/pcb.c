/**************************** Module PCB.c *****************************
*                                                                      *
* Copyright (C) 2019 Matteo Mele, Leonardo Pio Palumbo                 *
*                                                                      *
* You should have received a copy of the GNU General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>. *
*                                                                      *
***********************************************************************/

/* Per il maxproc */

#include "../H/pcb.h"

//aux functions
HIDDEN void initPcbToNull(pcb_t *pcb);
HIDDEN pcb_t* getPcb(struct list_head* head);
HIDDEN int getPriority(struct list_head* pos);

//list of free process
HIDDEN struct list_head pcbFree;
HIDDEN struct pcb_t pcbFree_table[MAXPROC];
//sentinel node of free pcbFree
struct list_head* pcbfree_h;

/***********************************************************************
 *                                                                     *
 *                         AUXILIARY FUNCTIONS                         *
 *                                                                     *
 ***********************************************************************/


/***********************************************************************
 *                                                                     *
 * Inizializes all the fields of the process pointed by pcb to 0/NULL  *
 * pcb = pointer to process to be inizialized                          *
 *                                                                     *
 ***********************************************************************/
void initPcbToNull(pcb_t *pcb){
   INIT_LIST_HEAD(&pcb->p_next);
   pcb->p_parent = NULL;
   pcb->p_semkey = NULL;
   INIT_LIST_HEAD(&pcb->p_child);
   INIT_LIST_HEAD(&pcb->p_sib);

   //set process state
   pcb->p_s.entry_hi = 0;
   pcb->p_s.cause = 0;
   pcb->p_s.status = 0;
   pcb->p_s.pc_epc = 0;
   pcb->p_s.hi = 0;
   pcb->p_s.lo = 0;
   for(int i = 0; i < STATE_GPR_LEN; i++)
      pcb->p_s.gpr[i] = 0;
   for(int i = 0; i < 3; i++) pcb->has_spec[i] = false;

    pcb->priority = 0;
    pcb->original_priority = 0;
}
/**********************************************************************
 *                                                                    *
 * Given a list_head, return the corresponding process                *
 * pos = list_head of the process                                     *
 * return = corresponding process                                     *
 *                                                                    *
 **********************************************************************/
HIDDEN pcb_t* getPcb(struct list_head* head){
  return container_of(list_next(head), struct pcb_t, p_next);
}

/**********************************************************************
 *                                                                    *
 * Given a list_head, return the corresponding process priority       *
 * pos = list_head of the process                                     *
 * return = corresponding process priority                            *
 *                                                                    *
 **********************************************************************/
int getPriority(struct list_head* head){
  return (container_of(head, struct pcb_t, p_next)->priority);
}


/**********************************************************************
 * Set priority and original_priority field of p PCB to given value   *
 * return = void                                                      *
 *                                                                    *
 **********************************************************************/
void setPriority(struct pcb_t* p, int newPriority){
  p->priority = newPriority;
  p->original_priority = newPriority;
}

/**********************************************************************
*  Increase priority of process p by 1                                *
*  return = void                                                      *
***********************************************************************/
void increasePriority(struct pcb_t* p){
  ++p->priority;
}

/**********************************************************************
*  Priority field of process p is restored to original value          *
***********************************************************************/
void restorePriority(struct pcb_t* p){
    p->priority = p->original_priority;
}

/***********************************************************************
*                                                                      *
* Inizializes pcbFree in order to contain all the elements from        *
* pcbFree_table. This method is invoked only once to inizialize data   *
* structures                                                           *
*								                                                       *
************************************************************************/
void initPcbs(void){
   struct pcb_t* proc;

   //init list of free process
   INIT_LIST_HEAD(&pcbFree);
   pcbfree_h = &pcbFree;

   for (int i = 0; i < MAXPROC; i++){
     proc = &pcbFree_table[i];
     list_add_tail(&proc->p_next, &pcbFree);
   }
}
/************************************************************************
 *                                                                      *
 * Inserts process pointed by p to pcbFree                              *
 * p = pointer to process to be added                                   *
 *                                                                      *
 ************************************************************************/
void freePcb(pcb_t *p){
   list_add(&p->p_next, &pcbFree);
}
/************************************************************************
 *                                                                      *
 * Picks a process from semdFree, if possible, inizializes all the      *
 * fields to 0/NULL, returns the new process. Returns NULL otherwise.   *
 * return = new process inizialized to 0/NULL                           *
 *            NULL, if free process is not available                    *
 *                                                                      *
 ************************************************************************/
pcb_t *allocPcb(void){
 struct pcb_t* proc = NULL;
 struct list_head* pos;

 if (list_empty(&pcbFree) == FALSE) {
   //remove first process of pcbFree
  pos = list_next(&pcbFree);
  list_del(pos);

  proc = container_of(pos, struct pcb_t, p_next);
  initPcbToNull(proc);
 }

 return proc;
}

/***********************************************************************
 *                                                                     *
 * Inizializes a process queue pointed by head                         *
 * head = pointer to process queue                                     *
 *                                                                     *
 ***********************************************************************/
void mkEmptyProcQ(struct list_head* head){
  INIT_LIST_HEAD(head);
}

/***********************************************************************
 *                                                                     *
 * Checks if list pointed by head is empty                             *
 *                                                                     *
 * return = TRUE, if list is empty                                     *
 *           FALSE, otherwise                                          *
 *                                                                     *
 ***********************************************************************/
int emptyProcQ(struct list_head* head){
  return list_empty(head);
}

/**********************************************************************
 *                                                                    *
 * Inserts process p in the queue pointed by head.                    *
 * Process are ordered based on priority field (decreasing order).    *
 * If there are multiple process with the same priority of p, p is    *
 * added AFTER these previous process. To guarantee No starvation     *
 *                                                                    *
 * head = list where p needs to be added                              *
 * p = process to be added                                            *
 * return = void.                                                     *
 *                                                                    *
 **********************************************************************/
void insertProcQ(struct list_head* head, pcb_t* p){
  if(head != NULL){
    //get pointer to list_head element of p
    struct list_head *newProc = &(p->p_next);
    /* empty list*/
    if(list_empty(head))list_add(newProc,head);
    else{
      struct list_head* pos = NULL;

      //pos now points to the process that follows p
      for (pos = (head)->next; pos != (head) && (getPriority(pos) >= p->priority); pos = pos->next);
      //adds p in the proper spot
      __list_add(newProc, list_prev(pos), pos);
    }
  }
}


/********************************************************************
 *                                                                  *
 * Returns the first element of the process queue pointed by head,  *
 * NULL otherwise                                                   *
 *                                                                  *
 * return = first element of the process Queue                      *
 *            NULL, if queue is empty                               *
 *                                                                  *
 ********************************************************************/
struct pcb_t* headProcQ(struct list_head* head){
  if(list_empty(head) || head==NULL) return NULL;
  struct pcb_t *aux = container_of(list_next(head), struct pcb_t, p_next);
  return aux;
}

/*******************************************************************
 *                                                                 *
 * Removes and returns the first proc from the process Queue       *
 * pointed by head.                                                *
 *                                                                 *
 * head = pointer to process queue                                 *
 * return = first process from process queue, if exists            *
 *                                                                 *
 *******************************************************************/
struct pcb_t* removeProcQ(struct list_head* head){

  if(list_empty(head)) return NULL;
  else {
    pcb_t* remvProc = getPcb(head);
    list_del(head->next);
    return remvProc;
  }
}

/***********************************************************************
 *                                                                     *
 * Removes and returns the process p from process queue pointed by     *
 * head, if p is present.                                              *
 *                                                                     *
 * head = pointer to process queue where p is eventually located       *
 * p = process to be removed                                           *
 * return = p - process to be removed                                  *
 *            NULL, if p doesn't apper in the process Queue            *
 *                                                                     *
 ***********************************************************************/
pcb_t* outProcQ(struct list_head* head, pcb_t *p){
  struct list_head *tmp;
  list_for_each(tmp,head){
    if(tmp == &p->p_next){
      list_del(&p->p_next);
      return p;
    }
  }
  return NULL;
}
/***********************************************************************
 *                                                                     *
 * Checks if p has any child                                           *
 *                                                                     *
 * return = TRUE, p has no child                                          *
 *          FALSE, p has child                                    *
 *                                                                     *
 ***********************************************************************/
int emptyChild(pcb_t *p){
   return(list_empty(&p->p_child));
}
/***********************************************************************
 *                                                                     *
 * Inserts p as last child of the process prnt                         *
 *                                                                     *
 * prnt = parent process                                               *
 * p = child process to be added                                       *
 *                                                                     *
 ***********************************************************************/
void insertChild(pcb_t *prnt, pcb_t *p){
   list_add_tail(&p->p_sib,&prnt->p_child);
   p->p_parent=prnt;
}

/***********************************************************************
 *                                                                     *
 * Removes and returns the first child of the process pointed by p,    *
 * if exists.                                                          *
 *                                                                     *
 * p = pointer to process                                              *
 * return = removed child process                                      *
 *          NULL, is p has no child                                    *
 *                                                                     *
 ***********************************************************************/
pcb_t *removeChild(pcb_t *p){
   if(emptyChild(p) || p==NULL) return NULL;
   else {
     //get pointer to p's first child
    struct pcb_t *pos;
	  struct list_head *t = &(p->p_child);
    pos = container_of(list_next(t), struct pcb_t, p_sib);
    list_del(list_next(t));

    //removes first child, parent setting to NULL
	  pos->p_parent=NULL;
    return pos;
   }
}

/***********************************************************************
 *                                                                     *
 * Removes and returns the process pointed by p from the parent's list *
 * of children. p could be located in any position in the list         *
 * p = process to be removed                                           *
 * return = p - removed process                                        *
 *           NULL - if p has NULL as p_parent                          *
 *                                                                     *
 ***********************************************************************/
struct pcb_t *outChild(pcb_t *p){
   if(p->p_parent==NULL) return NULL;
   else{
    struct list_head *tmp = &p->p_sib;
    //removes p, parent setting to NULL
    list_del(tmp);
    p->p_parent = NULL;
    return p;
   }
}
