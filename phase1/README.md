# SO-Project PHASE 1 - umps2


___

## FUNCTIONS 
### FILE PCB.C
___ 
#### LIST MANAGEMENT
* initPcbs
* freePcb
* allocPcb
* mkEmptyProcQ
* emptyProcQ 
* insertProcQ
* headProcQ
* removeProcQ
* outProcQ

#### TREE MANAGEMENT
* emptyChild
* insertChild
* removeChild
* outChild

#### AUXILIARY
* initPcbToNull
* getPcb
* getPriority
___

### FILE ASL.C
___
* getSemd
* insertBlocked
* removeBlocked
* outBlocked
* headBlocked
* outChildBlocked
* initAsl

#### AUXILIARY
* getKey
* initSemd
* setProcKey
* checkEmptySemd


## COMPILATION PROCESS
To compile the program is necessary to run
'make -f Makefile-templ'
in the '/C' directory.

## IMPLEMENTATION CHOICES

All the functions and the data structures have been implemented with reference to the specifications set out in http://www.cs.unibo.it/~renzo/so/rikaya/phase1_2019.pdf.

___

## AUTHORS
**Matteo Mele**,
**Leonardo Pio Palumbo**.
