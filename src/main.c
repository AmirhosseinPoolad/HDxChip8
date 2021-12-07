#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define PROGRAM_START 0x200
#define FONTSET_START 0x050

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
    //These two registers count to zero at 60HZ
    unsigned char delay_timer;
    unsigned char sound_timer;
    //CHIP-8 screen is 64*32 black and white i.e. only pure black or pure white.
    //coordinates start from top left
    unsigned char screen[64 * 32];
    //CHIP-8 uses a hexadecimal keypad that we map to the keyboard as such:
    //Keypad                   Keyboard
    //+-+-+-+-+                +-+-+-+-+
    //|1|2|3|C|                |1|2|3|4|
    //+-+-+-+-+                +-+-+-+-+
    //|4|5|6|D|                |Q|W|E|R|
    //+-+-+-+-+       =>       +-+-+-+-+
    //|7|8|9|E|                |A|S|D|F|
    //+-+-+-+-+                +-+-+-+-+
    //|A|0|B|F|                |Z|X|C|V|
    //+-+-+-+-+                +-+-+-+-+
    //This array is used to store the status of keys
    unsigned char key[16];
    //CHIP-8 sprites work as an array of 8 bit values, where each 8 bit is a row of pixels
    //Each bit in a row corresponds to the color of pixel.
    //The opcode Dxyn draws an n byte (so n rows) sprite starting from screen coordinate (x,y)
    //In memory addresses 0x050 to 0x0A0 of a CHIP-8 there's a hexadecimal fontset.
    unsigned char chip8_fontset[80] =
        {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };
    //now we copy the fontset into the memory
    //16 characters, each 5 bytes
    for (int i = 0; i < 16 * 5; i++)
        memory[FONTSET_START + i] = chip8_fontset[i];
}