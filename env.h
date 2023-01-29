// SPDX-FileCopyrightText: 2016 Noeliel
//
// SPDX-License-Identifier: LGPL-2.0-only

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define DISPLAY_WIDTH   64
#define DISPLAY_HEIGHT  32

typedef uint8_t byte;
typedef uint16_t word;

union MEMORY {
    byte raw[0x1000];
    
    struct {
        byte font_data[0x200];
        byte mem1[0xCA0];
        byte mem2[0x60];
        byte display[0x100];
    };
};

struct CPU_REGISTERS {
    byte V[0x10];
    word i;
};

extern word stack[0x20];
extern word *SP;
extern word PC;

extern byte sound_timer; // 60Hz
extern byte delay_timer; // 60Hz

extern _Bool display_refresh;

extern byte display[DISPLAY_WIDTH * DISPLAY_HEIGHT];

union KEYBOARD {
    word raw;
    
    struct __attribute__((packed)) {
        _Bool k0 : 1;
        _Bool k1 : 1;
        _Bool k2 : 1;
        _Bool k3 : 1;
        _Bool k4 : 1;
        _Bool k5 : 1;
        _Bool k6 : 1;
        _Bool k7 : 1;
        _Bool k8 : 1;
        _Bool k9 : 1;
        _Bool kA : 1;
        _Bool kB : 1;
        _Bool kC : 1;
        _Bool kD : 1;
        _Bool kE : 1;
        _Bool kF : 1;
    };
};

extern void run(char *rom_path);
extern void set_key_state(byte key, _Bool state);