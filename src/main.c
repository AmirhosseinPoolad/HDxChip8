#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define MEMORY_SIZE 4096
#define PROGRAM_START 0x200

#define FONTSET_START 0x050
#define FONT_HEIGHT 5
#define FONT_WIDTH 8

#define SPRITE_WIDTH 8

int main()
{
    //System memory
    unsigned char memory[MEMORY_SIZE];
    //General porpuse registers
    //(this ancient chip has more sensible register naming than x86. They're just named VA to VF.)
    //Keep in mind that VF is used as a FLAG register and should not be used in programs.
    unsigned char V[16];
    //Index register generally used for storing memory addresses
    unsigned short I = 0;
    //Program counter
    unsigned short PC = PROGRAM_START;
    //Used to store current Opcode. Each CHIP-8 opcode is 2 bytes.
    unsigned short opcode;
    //There's a stack that's used to remember where we were before a subroutine call. Maximum 16 levels.
    unsigned short stack[16];
    //Stack pointer to remember which the top most level of stack
    unsigned char SP = 0;
    //These two registers count to zero at 60HZ
    unsigned char delay_timer = 0;
    unsigned char sound_timer = 0;
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
    char path[256];
    printf("Enter ROM Adress\n");
    scanf("%s", path);
    //Copying the rom into memory
    unsigned char *romBuffer = malloc(MEMORY_SIZE - PROGRAM_START);
    FILE *romFile = fopen(path, "rb");
    if (romFile == NULL)
    {
        printf("Cannot open file. closing the program.");
        fclose(romFile);
        free(romBuffer);
        return 1;
    }
    unsigned int bufferSize = fread(romBuffer, sizeof(unsigned char), MEMORY_SIZE - PROGRAM_START, romFile);
    for (int i = 0; i < bufferSize; i++)
        memory[PROGRAM_START + i] = romBuffer[i];
    unsigned int exit = 0;
    while (!exit) //main loop
    {
        //each element is one byte, so we shift the first bite
        //into the most significant byte and the one after it into the least significant byte of opcode
        opcode = memory[PC] << 8 | memory[PC + 1];
        switch (opcode & 0xF000)
        {
        case 0x0000:
            switch (opcode & 0x000F)
            {
            case 0x0000: //Clear the display
                memset(screen, 0, sizeof(screen));
                break;
            case 0x000E:        //return from a subroutine
                PC = stack[SP]; //program counter = address at top of the stack
                SP--;           //remove top of the stack
                break;
            }
            break;
        case 0x1000: //1nnn --- jump PC to nnn
            PC = opcode & 0x0FFF;
        case 0x2000: //2nnn --- call subroutine at nnn
            stack[SP] = PC;
            PC = opcode & 0x0FFF;
            SP++;
            break;
        case 0x3000: //3xkk --- skip next instruction if Vx == kk
            if (V[(opcode & 0x0F00) >> 2] == (opcode & 0x00FF))
                PC += 2;
            break;
        case 0x4000: //4xkk --- skip next instruction if Vx != kk
            if (V[(opcode & 0x0F00) >> 2] != (opcode & 0x00FF))
                PC += 2;
            break;
        default:
            break;
        }
    }
}