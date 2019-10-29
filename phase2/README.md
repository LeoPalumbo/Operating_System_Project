# SO-Project PHASE 2 - umps2
This program is the third part of SO-Project.
It consist of:
  - **System initialization**
  - **Process scheduling**
  - **System call handler**
  - **Exception handler**

---
### System initialization
It's the same of Phase 1.5 but in this phase there is also the initialization of semaphores.

### Process scheduling
It's the same of phase 1.5.

### System call handler
There are 10 possible system call:
  - SYSCALL 1 (SYS1) Get_CPU_Time: we have been added in pcb structure 3 fields:
    - p_ttime (contains the total time of process)
    - p_tuser_start (contains the starting time of user mode execution)
	  - p_tuser_tot (contains the total time of user execution)
	  - p_tkernel_start (contains the starting time in kernel mode execution)
	  - p_tkernel_tot (contains the total time in kernel mode execution)
  The SYS1 saves in the fields of currentProc the information about this timer.
  - SYSCALL 2 (SYS2) Create_Process: it creates a new process as child of the caller process and insert it in the ready queue.
  - SYSCALL 3 (SYS3) Terminate_Process: it terminates the process identified from pid, but not his progeny. It returns 0 for succesfull, -1 for error.
  - SYSCALL 4 (SYS4) Verhogen: it increments the value of the semaphore passed as parameter. If the value of semaphore is negative, before the Verhogen operation, this function unlocks a process blocked on this semaphore.
  - SYSCALL 5 (SYS5) Passeren: it decrements the value of the semaphore passed as parameter. If the value of semaphore became negative after the Passeren operation, the process that has called the function remaning blocked on this semaphore until a Verhogen on the semaphore unlocks it.
  - SYSCALL 6 (SYS6) Wait_Clock: it suspends the process for a clock (100 ms).
  - SYSCALL 7 (SYS7) Do_IO: it active an I/O operation. It copy the command parameter in the command of the register of the device indicated as second argument.
  - SYSCALL 8 (SYS8) Set_Tutor: it set the currentProc as the tutor of all processes offspring that should remaning orphans. We have added the field ##tutor## in pcb structure.
  - SYSCALL 9 (SYS9) Spec_Passup: it register which handler of superior level should be active in case of Syscall/breakpoint, TLB o Program trap. The syscall returns 0 for succesfull and -1 for error.
    We have added:
    - a boolean variable has_speec[type] (type=3) that indicate if a process has already the superior level handler for the type.  
    - spec_TLB_old, spec_PgmTrap_old, spec_Sys_old (that contains the value of old area if a TLB or PGM or SYS are raised)
    - spec_TLB_new, spec_PgmTrap_new, spec_Sys_new (that contains the state to load)
    Spec_Passup return 0 for succesfull or -1 for error.
  - SYSCALL 10 (SYS10) Get_pid_ppid: it assigns the id of currentProc at *pid (if pid != NULL) and the id of parent process at *ppid (if ppid != NULL).
  - SYSCALL > 10 must be sent to superior level handler.

### Exception handler
The kernel must handle the interrupt line from 1 to 7. To identify the interrupt line we have create the getIP() function.
To identify the interrupt line we have create the getDevicePending function.
The interrupt are handled from lower to great. We had problem with the Processor Local Timer interrupt: if there were multiple interrupt pending, the only the Processor Local Timer was handled. This because the Processor Local Timer handler call the scheduler. So we have added a boolean value PLT that becames 1 if there isn't only the Processor Local Timer interrupt pending.
We also use:
  - waitToComplete_proc: Matrix of pcb which have sent a command to a device and are waiting for termination
  - waitToCompleteTerminal_proc: Matrix of pcb which have sent a command to a device and are waiting for termination
  - waitDevice_semd: Matrix of semaphore for I/O queue

## COMPILATION PROCESS
To compile is necessary to launch the commans:
  `make`
inside the /C directory

## IMPLEMENTATION CHOICES
All the functions and the data structures have been implemented with reference to the specifications set out in http://www.cs.unibo.it/~renzo/so/rikaya/phase2_2019.pdf

## ISSUES
Our project does not complete the test, in particular process p5 is blocked in printing its message, apparently without any reason consider that previous stamp are been completed. We think that there is a sort of "duplication of the process", one copy in the ready queue (remove correctly by terminate process after an another call to Spec_Passup already defined) and a copy still blocked on the printing semaphore. Even if Terminate_Process check if the process is eventually blocked and removes process from the semaphore before it will be removed from the ready queue.

Despite this issue, the project is full implemented in all the requested function.
Maybe, with professors' advice, we will be able to fix it!
___

## AUTHORS
**Matteo Mele**,
**Pietro Miotti**,
**Leonardo Pio Palumbo**.
