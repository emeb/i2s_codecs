/*
 * main.h - part of rp2040_audio. Common headers for all stuff.
 * 03-27-22 E. Brombaugh
 */

#ifndef __main__
#define __main__

#include "pico/stdlib.h"

//#define CODEC_WM8731
#define CODEC_AIC3101
//#define CODEC_NAU88C22
//#define CODEC_SGTL5000

void my_sleep_ms(uint64_t ms);

#endif
