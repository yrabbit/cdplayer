#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT (64 / 8) // This is a fraudulent configuration feature: the screen is still divided into 8 stripes

void init_screen(void);

// send vram to lcd
void refresh_screen(void);

// vram routines
void clear_screen(void);

#endif // SCREEN_H
