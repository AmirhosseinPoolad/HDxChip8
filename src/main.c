#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "SDL.h"

#define MEMORY_SIZE 4096
#define PROGRAM_START 0x200

#define FONTSET_START 0x050
#define FONT_HEIGHT 5
#define FONT_WIDTH 8

#define SPRITE_WIDTH 8

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

#define DEVICE_SCREEN_WIDTH 640
#define DEVICE_SCREEN_HEIGHT 320

int screenCoordinatesToIndex(unsigned char x, unsigned char y);
void drawByteArrayToTexture(unsigned char *screen, SDL_Texture *screenTexture);

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
    unsigned char screen[SCREEN_WIDTH * SCREEN_HEIGHT];
    memset(screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
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
    for (unsigned int i = 0; i < bufferSize; i++)
        memory[PROGRAM_START + i] = romBuffer[i];
    unsigned int exit = 0;

    //Setup SDL stuff
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "HDxChip8",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        DEVICE_SCREEN_WIDTH,
        DEVICE_SCREEN_HEIGHT,
        0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_Texture *text = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    unsigned int startTicks;
    unsigned int endTicks;
    unsigned char keyDown;
    while (!exit) //main loop
    {
        startTicks = SDL_GetTicks();
        //Handle input
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
                exit = 1;
            if (e.type == SDL_KEYDOWN)
            {
                switch (e.key.keysym.sym)
                {
                case SDLK_1:
                    key[0x1] = 1;
                    keyDown = 0x1;
                    break;
                case SDLK_2:
                    key[0x2] = 1;
                    keyDown = 0x2;
                    break;
                case SDLK_3:
                    key[0x3] = 1;
                    keyDown = 0x3;
                    break;
                case SDLK_4:
                    key[0xC] = 1;
                    keyDown = 0xC;
                    break;
                case SDLK_q:
                    key[0x4] = 1;
                    keyDown = 0x4;
                    break;
                case SDLK_w:
                    key[0x5] = 1;
                    keyDown = 0x5;
                    break;
                case SDLK_e:
                    key[0x6] = 1;
                    keyDown = 0x6;
                    break;
                case SDLK_r:
                    key[0xD] = 1;
                    keyDown = 0xD;
                    break;
                case SDLK_a:
                    key[0x7] = 1;
                    keyDown = 0x7;
                    break;
                case SDLK_s:
                    key[0x8] = 1;
                    keyDown = 0x8;
                    break;
                case SDLK_d:
                    key[0x9] = 1;
                    keyDown = 0x9;
                    break;
                case SDLK_f:
                    key[0xE] = 1;
                    keyDown = 0xE;
                    break;
                case SDLK_z:
                    key[0xA] = 1;
                    keyDown = 0xA;
                    break;
                case SDLK_x:
                    key[0x0] = 1;
                    keyDown = 0x0;
                    break;
                case SDLK_c:
                    key[0xB] = 1;
                    keyDown = 0xB;
                    break;
                case SDLK_v:
                    key[0xF] = 1;
                    keyDown = 0xF;
                    break;
                }
            }
            if (e.type == SDL_KEYUP)
            {
                switch (e.key.keysym.sym)
                {
                case SDLK_1:
                    key[0x1] = 0;
                    break;
                case SDLK_2:
                    key[0x2] = 0;
                    break;
                case SDLK_3:
                    key[0x3] = 0;
                    break;
                case SDLK_4:
                    key[0xC] = 0;
                    break;
                case SDLK_q:
                    key[0x4] = 0;
                    break;
                case SDLK_w:
                    key[0x5] = 0;
                    break;
                case SDLK_e:
                    key[0x6] = 0;
                    break;
                case SDLK_r:
                    key[0xD] = 0;
                    break;
                case SDLK_a:
                    key[0x7] = 0;
                    break;
                case SDLK_s:
                    key[0x8] = 0;
                    break;
                case SDLK_d:
                    key[0x8] = 0;
                    break;
                case SDLK_f:
                    key[0xE] = 0;
                    break;
                case SDLK_z:
                    key[0xA] = 0;
                    break;
                case SDLK_x:
                    key[0x0] = 0;
                    break;
                case SDLK_c:
                    key[0xB] = 0;
                    break;
                case SDLK_v:
                    key[0xF] = 0;
                    break;
                }
            }
        }
        //each element is one byte, so we shift the first bite
        //into the most significant byte and the one after it into the least significant byte of opcode
        opcode = memory[PC] << 8 | memory[PC + 1];
        switch (opcode & 0xF000)
        {
        case 0x0000:
            switch (opcode & 0x000F)
            {
            case 0x0000: //00E0 --- Clear the display
                memset(screen, 0, sizeof(screen));
                PC += 2;
                break;
            case 0x000E: //00EE --- eturn from a subroutine
                if (SP != 0)
                {
                    SP--;           //remove top of the stack
                    PC = stack[SP]; //program counter = address at top of the stack
                    PC += 2;
                }
                else
                {
                    PC += 2;
                }
                break;
            }
            break;
        case 0x1000: //1nnn --- jump PC to nnn
            PC = opcode & 0x0FFF;
            break;
        case 0x2000: //2nnn --- call subroutine at nnn
            stack[SP] = PC;
            PC = opcode & 0x0FFF;
            SP++;
            break;
        case 0x3000: //3xkk --- skip next instruction if Vx == kk
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                PC += 4; //two for this execution, 2 for the next
            else
                PC += 2;
            break;
        case 0x4000: //4xkk --- skip next instruction if Vx != kk
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                PC += 4;
            else
                PC += 2;
            break;
        case 0x5000: //5xy0 --- skip next instruction if Vx == Vy
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
                PC += 4;
            else
                PC += 2;
            break;
        case 0x6000: //6xkk --- set Vx = kk
            V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
            PC += 2;
            break;
        case 0x7000: //7xkk --- set Vx = Vx + kk;
            V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
            PC += 2;
            break;
        case 0x8000:
            switch (opcode & 0x000F)
            {
            case 0x0000: //8xy0 --- Set Vx = Vy
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                PC += 2;
                break;
            case 0x0001: //8xy1 --- Set Vx = Vx OR Vy
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] | V[(opcode & 0x00F0) >> 4];
                PC += 2;
                break;
            case 0x0002: //8xy2 --- Set Vx = Vx AND Vy
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] & V[(opcode & 0x00F0) >> 4];
                PC += 2;
                break;
            case 0x0003: //8xy3 --- Set Vx = Vx XOR Vy
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] ^ V[(opcode & 0x00F0) >> 4];
                PC += 2;
                break;
            case 0x0004: //8xy4 --- Set Vx = Vx + Vy, set VF = carry
                if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
                //if Vy is bigger than 255 - Vx
                {
                    V[0xF] = 1; //carry
                }
                else
                {
                    V[0xF] = 0; //no carry
                }
                V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                PC += 2;
                break;
            case 0x0005: //8xy5 --- Set Vx = Vx - Vy, set VF = NOT borrow
                if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
                //if Vy is bigger than Vx
                {
                    V[0xF] = 0; //borrow (no carry)
                }
                else
                {
                    V[0xF] = 1; //no borrow (carry)
                }
                V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                PC += 2;
                break;
            case 0x0006: //8xy6 --- Set Vx = Vx SHR 1 (carry in VF)
                if (V[(opcode & 0x0F00) >> 8] & 0x0001)
                {
                    V[0xF] = 1;
                }
                else
                {
                    V[0xF] = 0;
                }
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] >> 1;
                PC += 2;
                break;
            case 0x0007: //8xy7 --- Set Vx = Vy - Vx, set VF = NOT borrow
                if (V[(opcode & 0x00F0) >> 4] < V[(opcode & 0x0F00) >> 8])
                //if Vx is bigger than Vy
                {
                    V[0xF] = 0; //borrow (no carry)
                }
                else
                {
                    V[0xF] = 1; //no borrow (carry)
                }
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                PC += 2;
                break;
            case 0x000E: //8xyE --- Set Vx = Vx SHL 1 (carry in VF)
                if ((V[(opcode & 0x0F00) >> 8] & 0x8000) == 0x8000)
                {
                    V[0xF] = 1;
                }
                else
                {
                    V[0xF] = 0;
                }
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] << 1;
                PC += 2;
                break;
            }
            break;
        case 0x9000: //9xy0 --- Skip next instruction if Vx != Vy
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
                PC += 4;
            else
                PC += 2;
            break;
        case 0xA000: //Annn --- Set I = nnn
            I = opcode & 0x0FFF;
            PC += 2;
            break;
        case 0xB000: //Bnnn --- Jump to location nnn + V0
            PC = (opcode & 0x0FFF) + V[0x0];
            break;
        case 0xC000: //Cxkk --- Set Vx = random byte AND kk
        {
            unsigned char random = rand() % 256;
            V[(opcode & 0x0F00) >> 8] = random & (opcode & 0x00FF);
            PC += 2;
        }
        break;
        case 0xD000: //Dxyn --- Display n-byte (n rows) sprite starting at memory location I at (Vx, Vy), set VF = collision.
            //Drawing is done by XORing the sprite with the screen. If any pixel gets erased during this a collision has happened
            {
                unsigned short x = V[(opcode & 0x0F00) >> 8];
                unsigned short y = V[(opcode & 0x00F0) >> 4];
                unsigned short n = (opcode & 0x000F);
                for (int j = 0; j < n; j++)
                {
                    for (int i = 0; i < 8; i++)
                    {
                        //get pixel (i,j) of sprite
                        unsigned char pixelValue = (memory[I + j] & (0x80 >> i)) >> (7 - i);
                        //if pixel and screen are both 1
                        if ((screen[screenCoordinatesToIndex(x + i, y + j)] == 1) && (pixelValue == 1))
                            V[0xF] = 1; //collision
                        //draw the pixel
                        screen[screenCoordinatesToIndex(x + i, y + j)] ^= pixelValue;
                    }
                }
                PC += 2;
            }
            break;
        case 0xE000:
            switch (opcode & 0x00FF)
            {
            case 0x009E: //Ex9E --- Skip next instruction if key with the value of Vx is pressed.
                if (key[V[(opcode & 0x0F00) >> 8]])
                {
                    PC += 4;
                }
                else
                {
                    PC += 2;
                }
                break;
            case 0x00A1: //ExA1 --- Skip next instruction if key with the value of Vx is not pressed.
                if (!key[V[(opcode & 0x0F00) >> 8]])
                {
                    PC += 4;
                }
                else
                {
                    PC += 2;
                }
                break;
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF)
            {
            case 0x0007: //Fx07 --- Set Vx = delay timer value.
                V[(opcode & 0x0F00) >> 8] = delay_timer;
                PC += 2;
                break;
            case 0x000A: //Fx0A --- Wait for a key press, store the value of the key in Vx.
                if (keyDown != 0)
                {
                    V[(opcode & 0x0F00) >> 8] = keyDown;
                    PC += 2;
                }
                break;
            case 0x0015: //Fx15 --- Set delay timer = Vx.
                delay_timer = V[(opcode & 0x0F00) >> 8];
                PC += 2;
                break;
            case 0x0018: //Fx18 --- Set sound timer = Vx.
                sound_timer = V[(opcode & 0x0F00) >> 8];
                PC += 2;
                break;
            case 0x001E: //Fx1E --- Set I = I + Vx.
                I += V[(opcode & 0x0F00) >> 8];
                PC += 2;
                break;
            case 0x0029: //Fx29 --- Set I = location of sprite for digit Vx.
            {
                I = FONTSET_START + V[(opcode & 0x0F00) >> 8] * FONT_HEIGHT;
                PC += 2;
            }
            break;
            case 0x0033: //Fx33 --- Store BCD representation of Vx in memory locations I, I+1, and I+2.
            {
                unsigned char num = V[(opcode & 0x0F00) >> 8];
                unsigned char ones = num % 10;
                unsigned char tens = (num / 10) % 10;
                unsigned char hundreds = (num / 100) % 10;
                memory[I] = hundreds;
                memory[I + 1] = tens;
                memory[I + 2] = ones;
                PC += 2;
            }
            break;
            case 0x0055: //Fx55 --- Store registers V0 through Vx in memory starting at location I.
            {
                int x = (opcode & 0x0F00) >> 8;
                for (int i = 0; i <= x; i++)
                {
                    memory[I + i] = V[i];
                }
                PC += 2;
            }
            break;
            case 0x0065: //Fx65 --- Read registers V0 through Vx from memory starting at location I.
            {
                int x = (opcode & 0x0F00) >> 8;
                for (int i = 0; i <= x; i++)
                {
                    V[i] = memory[I + i];
                }
                PC += 2;
            }
            break;
            }
            break;
        default:
            break;
        }
        if (delay_timer > 0)
            delay_timer--;
        if (sound_timer > 0)
            sound_timer--;
        drawByteArrayToTexture(screen, text);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, text, NULL, NULL);
        SDL_RenderPresent(renderer);
        endTicks = SDL_GetTicks();
        if ((endTicks - startTicks) < 16)
            SDL_Delay(16 - (endTicks - startTicks));
    }
}

int screenCoordinatesToIndex(unsigned char x, unsigned char y)
{
    int finalX = x, finalY = y;
    if (x >= SCREEN_WIDTH)
        finalX = x % SCREEN_WIDTH;
    if (y >= SCREEN_HEIGHT)
        finalY = y % SCREEN_HEIGHT;
    return (finalY * SCREEN_WIDTH) + finalX;
}

void drawByteArrayToTexture(unsigned char *screen, SDL_Texture *screenTexture)
{
    uint32_t textureBuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
    for (int y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            int index = screenCoordinatesToIndex(x, y);
            if (screen[index] == 1)
            {
                textureBuffer[index] = 0xFFFFFFFF;
            }
            else
            {
                textureBuffer[index] = 0x00000000;
            }
        }
    }
    SDL_UpdateTexture(screenTexture, NULL, textureBuffer, SCREEN_WIDTH * sizeof(uint32_t));
}