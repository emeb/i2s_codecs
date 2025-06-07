/*
 * led.c - 01Space RP2040 LED
 */

#ifndef __led__
#define __led__

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN 3

void LEDInit(void);
void LEDOn(void);
void LEDOff(void);
void LEDToggle(void);

#endif
