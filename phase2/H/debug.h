//
// Created by Pietro Miotti on 2019-08-25.
//

#ifndef PHASE2DEF_DEBUG_H
#define PHASE2DEF_DEBUG_H

#define MAXBUF 1024
char buffer[MAXBUF];
void printf(char* str){
    char *ptr = buffer;
    while(*(ptr++) = *(str++));
}

#endif //PHASE2DEF_DEBUG_H
