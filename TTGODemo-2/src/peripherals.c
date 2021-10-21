#include "peripherals.h"
#include <freertos/FreeRTOS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graphics.h"
#include "st7789v.h"
#include "BTKey/SDL.h"
//need to map keys (probably through BT) 
SDL_Scancode keymappings[16] = {//since i have to use SDL to manage the BT Keyboard, this section actually stays the same....i think.
    SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
    SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
    SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V};

int QUIT = 0;

void draw(unsigned char* display){
    cls(0);
    for(int y = 0; y<32; y++){//example uses SDL graphics and the functions called did the same as this so it might work. unsure about the see through, or the screensize. but idc
        for(int x = 0; x<64; x++){
            draw_rectangle(x*8,y*8,8,8,0xFFFF);
        }
    }
    flip_frame();
}

