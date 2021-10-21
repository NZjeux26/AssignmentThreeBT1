#ifndef CHIP_8_H_
#define CHIP_8_H_

extern unsigned char fontset[80];
extern unsigned char memory[4096]; //Mem is 4kb of RAM 0x000-0x1FF reserved
extern unsigned char V[16]; //16 8-bit registers for general use
extern unsigned char keypad[16]; // 16 key hex kaypad
extern unsigned char display[64 * 32]; //64x32px display, will need to be modded later to fit the ttgo LCD
extern unsigned char dt;//delay timer
extern unsigned char st;// sound timer
extern unsigned short I; // special 16-bit register for storing memory addresses
extern unsigned short pc;//program counter
extern unsigned char sp; //stack pointer
extern unsigned short stack[16]; //16 16-bit values to store addresses
extern unsigned char draw_flag;
extern unsigned char sound_flag;//don't see this being used but kept anyway.

//function declerations for later.
void init_CPU(void);
int load_ROM(char* filename);
void emulate_cycle(void);//removed void fo testing

#endif