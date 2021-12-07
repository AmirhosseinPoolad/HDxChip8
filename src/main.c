#include "stdio.h"
#include "stdlib.h"

#define PROGRAM_START 0x200;

int main()
{
    //System memory
    unsigned char memory[4096];
    //General porpuse registers
    //(this ancient chip has more sensible register naming than x86. They're just named VA to VF.)
    //Keep in mind that VF is used as a FLAG register and should not be used in programs.
    unsigned char V[16];
    //Index register generally used for storing memory addresses
    unsigned short I;
    //Program counter
    unsigned short PC;
    //There's a stack that's used to remember where we were before a subroutine call. Maximum 16 levels.
    unsigned short stack[16];
    //Stack pointer to remember which the top most level of stack
    unsigned char SP;
}