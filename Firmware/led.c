/*
 * led.c - 01Space RP2040 LED
 */

#include "led.h"


/*
 * Initialize the LED
 */
void LEDInit(void)
{
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
}

/*
 * Turn on LED
 */
void LEDOn(void)
{
	gpio_put(LED_PIN, 1);
}

/*
 * Turn off LED
 */
void LEDOff(void)
{
	gpio_put(LED_PIN, 0);
}

/*
 * Toggle LED
 */
void LEDToggle(void)
{
	gpio_put(LED_PIN, gpio_get(LED_PIN)^1);
}

