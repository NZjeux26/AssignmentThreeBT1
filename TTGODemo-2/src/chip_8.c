#include "chip_8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <math.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <nvs_flash.h>
#include "input_output.h"

#define debug_print(fmt, ...)
/* 
   do{                                              
      if(DEBUG) fprintf(stderr, fmt, _VA_ARGS_); 
    }while (0)

 */
int DEBUG = 1;
extern int errno;

unsigned char fontset[80] = {
   0xF0, 0x90, 0x90, 0x90, 0xf0, //0
   0x20, 0x60, 0x20, 0x20, 0x70, //1
   0xf0, 0x10, 0xf0, 0x80, 0xf0, //2
   0xf0, 0x10, 0xf0, 0x10, 0xf0, //3
   0x90, 0x90, 0xf0, 0x10, 0x10, //4
   0xf0, 0x80, 0xf0, 0x10, 0xf0, //5
   0xf0, 0x80, 0xf0, 0x90, 0xf0, //6
   0xf0, 0x10, 0x20, 0x30, 0x40, //7
   0xf0, 0x90, 0xf0, 0x90, 0xf0, //8
   0xf0, 0x90, 0xf0, 0x10, 0xf0, //9
   0xf0, 0x90, 0xf0, 0x90, 0x90, //A
   0xe0, 0x90, 0xe0, 0x90, 0xe0, //b
   0xf0, 0x80, 0x80, 0x80, 0xf0, //c
   0xe0, 0x90, 0x90, 0x90, 0xe0, //d
   0xf0, 0x80, 0xf0, 0x80, 0xf0, //e
   0xf0, 0x80, 0xf0, 0xf0, 0x80, //f
};

unsigned char memory[4096] = {0};
unsigned char V[16] = {0};
unsigned short I = 0;
unsigned short pc = 0x200; // most programs start at 0x200 as anything below is used by interpter
unsigned char sp = 0;
unsigned short stack[16] = {0};
unsigned char keypad[16] = {0};
unsigned char display[64 * 32] = {0};
unsigned char dt = 0;
unsigned char st = 0;
unsigned char draw_flag = 0;
unsigned char sound_flag = 0;

int load_ROM(char* filename){
    FILE* fp = fopen(filename, "r");
    
    if(fp == NULL) return errno;

    struct stat st;
    stat(filename, &st);
    size_t fsize = st.st_size;

    size_t bytes_read = fread(memory + 0x200, 1, sizeof(memory) - 0x200, fp);

    if(bytes_read != fsize){
        return -1;
    }

    fclose(fp);
    
    return 0;
}

void init_CPU(void){
    srand((unsigned int)time(NULL));
    mempcpy(memory, fontset, sizeof(fontset));
}

//see http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#memmap for instruction set 
void emulate_cycle(void){
    draw_flag = 0;
    sound_flag = 0;

    unsigned short op = memory[pc] << 8 | memory[pc + 1];

    unsigned short x = (op & 0x0f00) >> 8;
    unsigned short y = (op & 0x00f0) >> 4;

    //kk in the documentation == op & 0x0ff
    //nnn == op & 0x0fff

    switch(op & 0xf000){

        case 0x0000:
        switch(op & 0x00ff){
            case 0x00e0://Clear display
            debug_print("[OK] 0x%x: 00E0\n", op);
            for(int i=0; i< 64 * 32; ++i){
                display[i] = 0;
            }

            pc += 2;
            break;

            case 0x00ee://return from sub
            debug_print("[OK] 0x%x: 00EE\n",op);
            pc = stack[sp];//sets program counter to address of top of the stack
            sp--;//subtracts one from sp pointer
            pc += 2;
            break;

            default:
            debug_print("[FAILURE]X Unknown opCode: 0x%x\n", op);
            break;
        }//switch2

        case 0x1000://jumps to address NNN
        debug_print("[OK] 0x%x: 1NNN\n", op);
        pc = op & 0x0fff;
        break;

        case 0x2000: //calls subrou at NNN
        debug_print("[OK] 0x%x: 2NNN\n", op);
        sp += 1;
        stack[sp] = pc;
        pc = op & 0x0fff;
        break;

        case 0x3000:
        debug_print("[OK] 0x%x: 3XNN\n", op);
        if(V[x] == (op & 0x00ff)){
            pc += 2;
        }
        pc+= 2;
        break;

        case 0x4000:
        debug_print("[OK] 0x%x: 4XNN\n", op);
        if(V[x] != (op & 0x00ff)){
            pc +=2;
        }
        pc += 2;
        break;

        case 0x5000:
        debug_print("[OK] 0x%x: 5XY0\n", op);
        if(V[x] == V[y]){
            pc+= 2;
        }
        pc+=2;
        break;

        case 0x6000:
        debug_print("[OK] 0x%x: 6XNN\n", op);
        V[x] = (op & 0x00ff);
        pc+=2;
        break;

        case 0x7000:
        debug_print("[OK] 0x%x: 7XNN\n", op);
        V[x] += op & 0x00ff;
        pc+=2;
        break;

        case 0x8000:
          switch(op & 0x000f){
            case 0x0000:  
            debug_print("[OK] 0x%x: 8XY0\n", op);
            V[x] = V[y];
            pc+=2;
            break;

            case 0x0001:
            debug_print("[OK] 0x%x: 8XY1\n", op);
            V[x] = (V[x] | V[y]);
            pc+=2;
            break;

            case 0x0002:
            debug_print("[OK] 0x%x: 8XY2\n", op);
            V[x] = (V[x] & V[y]);
            pc+=2;
            break;

            case 0x0003:
            debug_print("[OK] 0x%x: 8XY3\n", op);
            V[x] = (V[x] ^ V[x]);
            pc+=2;
            break;

            case 0x0004:
            debug_print("[OK] 0x%x: 8XY4\n", op);
            V[0xF] = (V[x] + V[y] > 0xff) ? 1 : 0;
            V[x] += V[y];
            pc+=2;
            break;

            case 0x0005:
            debug_print("[OK] 0x%x: 8XY5\n", op);
            V[0xf] = (V[x] > V[y]) ? 1 : 0;
            V[x] -= V[y];
            pc+=2;
            break;

            case 0x0006:
            debug_print("[OK] 0x%x: 8XY6\n", op);
            V[0xf] = V[x] & 0x1;
            V[x] = (V[x] >> 1);
            pc+=2;
            break;

            case 0x0007:
            debug_print("[OK] 0x%x: 8XY7\n", op);
            V[0xf] = (V[y] > V[x]) ? 1 : 0;
            V[x] = V[y] - V[x];
            pc+=2;
            break;

            case 0x000E:
            debug_print("[OK] 0x%x: 8XYE\n", op);
            V[0xf] = (V[x] >> 7) & 0x1;
            V[x] = (V[x] << 1);
            pc+=2;
            break;

            default:
            printf("[FAILED]Y Unkown op: 0x%x\n", op);

        }//0x8

        case 0x9000:
        debug_print("[OK] 0x%x: 9XY0\n", op);
        if(V[x] != V[y]) pc+=2;
        pc+=2;
        break;

        case 0xA000:
        debug_print("[OK] 0x%x: ANNN\n", op);
        I = op & 0x0fff;
        pc+=2;
        break;

        case 0xB000:
        debug_print("[OK] 0x%x: BNNN\n", op);
        pc = (op & 0x0fff) + V[0];
        break;

        case 0xC000:
        debug_print("[OK] 0x%x: CXNN\n", op);
        V[x] = (rand() % 256) & (op & 0x00ff);
        pc+=2;
        break;

        case 0xD000:
            debug_print("[OK] 0x%X: DXYN\n", op);
            draw_flag = 1;

            unsigned short height = op & 0x000f;
            unsigned short px;

            V[0xF] = 0;

            for (int yline = 0; yline < height; yline++) {
                
                px = memory[I + yline];

                for (int xline = 0; xline < 8; xline++) {
             
                    if ((px & (0x80 >> xline)) != 0) {
                   
                        if (display[(V[x] + xline + ((V[y] + yline) * 64))] == 1) {
                            V[0xf] = 1;
                        }
                        display[V[x] + xline + ((V[y] + yline) * 64)] ^= 1;
                    }
                }
            }
            pc += 2;
        break;

        case 0xE000:
            switch (op & 0x00FF) {
                
                case 0x009E:
                    debug_print("[OK] 0x%x: EX9E\n", op);
                    if (keypad[V[x]]) {
                        pc += 2;
                    }
                    pc += 2;
                    break;

                case 0x00A1:
                    debug_print("[OK] 0x%x: EXA1\n", op);
                    if (!keypad[V[x]]) {
                        pc += 2;
                    }
                    pc += 2;
                    break;

                default:
                    printf("[FAILED]C Unknown op: 0x%X", op);
            }
        break;

        case 0xF000:
            switch (op & 0x00FF) {
  
                case 0x0007:
                    debug_print("[OK] 0x%x: FX07\n", op);
                    V[x] = dt;
                    pc += 2;
                    break;

                case 0x000A:
                    debug_print("[OK] 0x%x: FX0A\n", op);

                    for (int i = 0; i < 16; i++) {
                        if (keypad[i]) {
                            V[x] = i;
                            pc += 2;
                            break;
                        }
                    }
                    break;

                case 0x0015:
                    debug_print("[OK] 0x%x: FX15\n", op);
                    dt = V[x];
                    pc += 2;
                    break;

                case 0x0018:
                    debug_print("[OK] 0x%x: FX18\n", op);
                    st = V[x];
                    pc += 2;
                    break;

                case 0x001E:
                    debug_print("[OK] 0x%x: FX1E\n", op);
                    I += V[x];
                    pc += 2;
                    break;

                case 0x0029:
                    debug_print("[OK] 0x%x: FX29\n", op);
                    I = V[x] * 5;
                    pc += 2;
                    break;

                case 0x0033:
                    debug_print("[OK] 0x%x: FX33\n", op);

                    memory[I] = (V[x] % 1000) / 100;
                    memory[I + 1] = (V[x] % 100) / 10;
                    memory[I + 2] = (V[x] % 10);

                    pc += 2;
                    break;

                case 0x0055:
                    debug_print("[OK] 0x%x: FX55\n", op);

                    for (int i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }

                    pc += 2;
                    break;

                case 0x0065:
                    debug_print("[OK] 0x%x: FX65\n", op);

                    for (int i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }

                    pc += 2;
                    break;

                default:
                    vTaskDelay(500);
                    printf("[FAILED]V Unknown op: 0x%x\n", op);
                    break;
            }
            break;

        default:
            debug_print("[FAILED]B Unknown opcode: 0x%x\n", op);
            break;
    }
      //Update timers:
     
      //Decrement timers if they are > 0
     
    if (dt > 0) dt -= 1;
    if (st > 0) {
        sound_flag = 1;
        printf("BEEP\n");
        st -= 1;
    }

}//main switch

