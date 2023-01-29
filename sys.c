// SPDX-FileCopyrightText: 2016 Noeliel
//
// SPDX-License-Identifier: LGPL-2.0-only

#include "env.h"

union MEMORY mem;
struct CPU_REGISTERS cpu_regs;
word stack[0x20];
word *SP;
word PC;
byte sound_timer; // 60Hz
byte delay_timer; // 60Hz
byte display[DISPLAY_WIDTH * DISPLAY_HEIGHT];
union KEYBOARD kb;

_Bool cpu_running;
_Bool display_refresh;

static void cpu_reset()
{
    PC = 0x200;
    SP = stack;
    delay_timer = 0;
    sound_timer = 0;
    display_refresh = 0;
    cpu_running = 0;
    cpu_regs.i = 0;
    for (byte reg = 0; reg < 0x10; reg++)
        cpu_regs.V[reg] = 0;
    
}

static void cpu_tick();
extern void redraw();

static void cpu_exec()
{
    cpu_running = 1;
    
    for (;;)
    {
        //clock_t tick_start = clock();
        
        cpu_tick();
        
        if (display_refresh != 0)
        {
            redraw();
            display_refresh = 0;
        }
        
        if (cpu_running == 0)
            return;
        
        //useconds_t interval = floor(CLOCKS_PER_SEC / 480) - ((clock() - tick_start) * 10);
        //printf("%u\n", interval);
        usleep(1800);
        //usleep(2700); // slower
    }
}

static void illegal_opcode(word opcode)
{
    printf("Instructed to execute illegal opcode 0x%04x, skipping...\n", opcode);
    // cpu_running = 0;
}

static void cpu_tick()
{
    word opcode = (mem.raw[PC] << 8) | mem.raw[PC + 1];
    PC += 0x2;
    word tmp = 0;
    
    //printf("0x%04x\n", opcode);
    
    switch ((opcode & 0xF000) >> 12)
    {
        case 0x0:
            
            switch (opcode & 0xFF)
            {
                case 0xE0: // 0x00E0
                    // clear screen
                    memset(display, 0, sizeof(display));
                    display_refresh = 1;
                    break;
                    
                case 0xEE: // test pls ;_;
                    PC = (* SP);
                    SP--;
                    break;
                    
                default: // 0x0NNN
                    // call RCA 1802 program at address NNN (not necessary for most roms according to wikipedia
                    illegal_opcode(opcode);
                    break;
            }
            
            break;
            
        case 0x1: // done
            PC = opcode & 0xFFF;
            break;
            
        case 0x2: // done (pls test)
            SP++;
            (* SP) = PC;
            PC = opcode & 0xFFF;
            break;
            
        case 0x3: // done
            
            if (cpu_regs.V[(opcode & 0xF00) >> 8] == (opcode & 0xFF))
                PC += 0x2;
            
            break;
            
        case 0x4: // done
            
            if (cpu_regs.V[(opcode & 0xF00) >> 8] != (opcode & 0xFF))
                PC += 0x2;
            
            break;
            
        case 0x5: // done
            
            if (cpu_regs.V[(opcode & 0xF00) >> 8] == cpu_regs.V[(opcode & 0xF0) >> 4])
                PC += 0x2;
            
            break;
            
        case 0x6: // done
            cpu_regs.V[(opcode & 0xF00) >> 8] = (opcode & 0xFF);
            break;
        
        case 0x7: // done (pls test) // maybe this doesn't need a carry?
        {
            tmp = cpu_regs.V[(opcode & 0xF00) >> 8] + (opcode & 0xFF);
            //cpu_regs.V[0xF] = (tmp >> 8) & 0x1;
            cpu_regs.V[(opcode & 0xF00) >> 8] = tmp & 0xFF;
            
            break;
        }
            
        case 0x8: // done
        {
            switch (opcode & 0xF)
            {
                case 0x0:
                    cpu_regs.V[(opcode & 0xF00) >> 8] = cpu_regs.V[(opcode & 0xF0) >> 4];
                    break;
                    
                case 0x1:
                    cpu_regs.V[(opcode & 0xF00) >> 8] |= cpu_regs.V[(opcode & 0xF0) >> 4];
                    break;
                    
                case 0x2:
                    cpu_regs.V[(opcode & 0xF00) >> 8] &= cpu_regs.V[(opcode & 0xF0) >> 4];
                    break;
                
                case 0x3:
                    cpu_regs.V[(opcode & 0xF00) >> 8] ^= cpu_regs.V[(opcode & 0xF0) >> 4];
                    break;
                    
                case 0x4:
                {
                    tmp = cpu_regs.V[(opcode & 0xF00) >> 8] + cpu_regs.V[(opcode & 0xF0) >> 4];
                    cpu_regs.V[0xF] = (tmp >> 8) & 0x1;
                    cpu_regs.V[(opcode & 0xF00) >> 8] = tmp & 0xFF;
                    break;
                }
                    
                case 0x5:
                {
                    cpu_regs.V[0xF] = (cpu_regs.V[(opcode & 0xF00) >> 8] > cpu_regs.V[(opcode & 0xF0) >> 4] ? 1 : 0);
                    cpu_regs.V[(opcode & 0xF00) >> 8] = (cpu_regs.V[(opcode & 0xF00) >> 8] - cpu_regs.V[(opcode & 0xF0) >> 4]) & 0xFF;
                    break;
                }
                    
                case 0x6:
                    cpu_regs.V[0xF] = cpu_regs.V[(opcode & 0xF00) >> 8] & 0x1;
                    cpu_regs.V[(opcode & 0xF00) >> 8] = (cpu_regs.V[(opcode & 0xF00) >> 8] >> 1) & 0xFF;
                    break;
                    
                case 0x7:
                    cpu_regs.V[0xF] = (cpu_regs.V[(opcode & 0xF00) >> 8] <= cpu_regs.V[(opcode & 0xF0) >> 4] ? 1 : 0);
                    cpu_regs.V[(opcode & 0xF00) >> 8] = (cpu_regs.V[(opcode & 0xF0) >> 4] - cpu_regs.V[(opcode & 0xF00) >> 8]) & 0xFF;
                    break;
                    
                case 0xE:
                    cpu_regs.V[0xF] = (cpu_regs.V[(opcode & 0xF00) >> 8] >> 7) & 0x1;
                    cpu_regs.V[(opcode & 0xF00) >> 8] = (cpu_regs.V[(opcode & 0xF00) >> 8] << 1) & 0xFF;
                    break;
                    
                default:
                    illegal_opcode(opcode);
                    break;
            }
            
            break;
        }
            
        case 0x9: // done
            
            if (cpu_regs.V[(opcode & 0xF00) >> 8] != cpu_regs.V[(opcode & 0xF0) >> 4])
                PC += 0x2;
            
            break;
            
        case 0xA: // done
            cpu_regs.i = opcode & 0xFFF;
            break;
            
        case 0xB: // done
            PC = ((opcode & 0xFFF) + cpu_regs.V[0x0]) & 0xFFFF;
            break;
            
        case 0xC: // done
            cpu_regs.V[(opcode & 0xF00) >> 8] = (opcode & 0xFF) & (rand() % 0x100);
            break;
            
        case 0xD:
            
            cpu_regs.V[0xF] = 0x0;
            
            for (byte y = 0; y < (opcode & 0xF); y++) // y-lines
            {
                for (byte x = 0; x < 8; x++) // x-lines
                {
                    if ((mem.raw[cpu_regs.i + y] & (0x80 >> x)) != 0)
                    {
                        if ((display[(((cpu_regs.V[(opcode & 0xF00) >> 8] + x) % DISPLAY_WIDTH) + (DISPLAY_WIDTH * ((cpu_regs.V[(opcode & 0xF0) >> 4] + y) % DISPLAY_HEIGHT)))] & 0x1) == 0x1)
                            cpu_regs.V[0xF] = 0x1;
                        
                        display[(((cpu_regs.V[(opcode & 0xF00) >> 8] + x) % DISPLAY_WIDTH) + (DISPLAY_WIDTH * ((cpu_regs.V[(opcode & 0xF0) >> 4] + y) % DISPLAY_HEIGHT)))] ^= 0x1;
                    }
                }
            }
            
            display_refresh = 1;
            
            break;
            
        case 0xE: // done
            
            tmp = (kb.raw >> cpu_regs.V[(opcode & 0xF00) >> 8]) & 0x1;
            
            switch (opcode & 0xFF)
            {
                case 0x9E:
                    if (tmp != 0)
                        PC += 0x2;
                    break;
                
                case 0xA1:
                    if (tmp == 0)
                        PC += 0x2;
                    break;
                
                default:
                    illegal_opcode(opcode);
                    break;
            }
            
            break;
            
        case 0xF:
            
            switch (opcode & 0xFF)
            {
                case 0x07:
                    cpu_regs.V[(opcode & 0xF00) >> 8] = delay_timer;
                    break;
                    
                case 0x0A:
                    illegal_opcode(opcode);
                    // await key press, store in cpu_regs.V[(opcode & 0xF00) >> 8]
                    break;
                    
                case 0x15:
                    delay_timer = cpu_regs.V[(opcode & 0xF00) >> 8];
                    break;
                    
                case 0x18:
                    sound_timer = cpu_regs.V[(opcode & 0xF00) >> 8];
                    break;
                    
                case 0x1E:
                    cpu_regs.i = (cpu_regs.i + cpu_regs.V[(opcode & 0xF00) >> 8]) & 0xFFFF;
                    break;
                    
                case 0x29: // pls test
                    cpu_regs.i = (0x5 * cpu_regs.V[(opcode & 0xF00) >> 8]) & 0xFFFF;
                    break;
                    
                case 0x33:
                    mem.raw[cpu_regs.i] = cpu_regs.V[(opcode & 0xF00) >> 8] / 100;
                    mem.raw[cpu_regs.i + 1] = (cpu_regs.V[(opcode & 0xF00) >> 8] / 10) % 10;
                    mem.raw[cpu_regs.i + 2] = cpu_regs.V[(opcode & 0xF00) >> 8] % 10;
                    break;
                    
                case 0x55:
                    
                    for (byte i = 0; i <= ((opcode & 0xF00) >> 8); i++)
                        mem.raw[cpu_regs.i + i] = cpu_regs.V[i];
                    
                    break;
                    
                case 0x65:
                    
                    for (byte i = 0; i <= ((opcode & 0xF00) >> 8); i++)
                        cpu_regs.V[i] = mem.raw[cpu_regs.i + i];
                    
                    break;
                    
                default:
                    illegal_opcode(opcode);
                    break;
            }
            
            break;
            
        default: // done
            illegal_opcode(opcode);
            break;
    }
    
    // todo: moar timer stuff
    
    if (delay_timer > 0)
        delay_timer--;
    
    if (sound_timer > 0)
    {
        //if (sound_timer == 1) { /* beep */ }
        
        sound_timer--;
    }
}

void set_key_state(byte key, _Bool state)
{
    kb.raw ^= (-state ^ kb.raw) & (1 << key);
}

static void load_rom(char *rom_path)
{
    long rom_size = 0;
    FILE *fbuf = fopen(rom_path, "r");
    
    if (fbuf == NULL)
    {
        printf("Error trying to read rom file: %s\n", rom_path);
        return;
    }
    
    fseek(fbuf, 0, SEEK_END);
    rom_size = ftell(fbuf);
    rewind(fbuf);
    
    fread((mem.raw + 0x200), rom_size, 1, fbuf);
    
    fclose(fbuf);
}

void run(char *rom_path)
{
    // load rom into memory
    load_rom(rom_path);
    
    byte font_data[] = {
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
    
    memcpy(&mem.font_data, font_data, sizeof(font_data));
    
    cpu_reset();
    
    /*display[0] = 1;
    display[1 + (DISPLAY_WIDTH * 1)] = 1;
    display[2 + (DISPLAY_WIDTH * 2)] = 1;
    display[3 + (DISPLAY_WIDTH * 3)] = 1;
    display[4 + (DISPLAY_WIDTH * 4)] = 1;
    display[5 + (DISPLAY_WIDTH * 5)] = 1;
    display[6 + (DISPLAY_WIDTH * 6)] = 1;
    display[7 + (DISPLAY_WIDTH * 7)] = 1;*/
    
    //redraw();
    cpu_exec();
}