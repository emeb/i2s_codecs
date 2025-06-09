/*
 * UDA1345.h - UDA1345 codec bitbang control port driver for RP2040
 * 06-08-25 E. Brombaugh
 */

#include <stdio.h>
#include "main.h"
#include "hardware/i2c.h"
#include "uda1345.h"

#define L3_DATA_PIN 16
#define L3_CLK_PIN 17
#define L3_MODE_PIN 20

/* The 6 bits UDA1345 L3 address (sent through bitbang interface) */
#define UDA1345_L3_ADDR 0x05

/* L3 control registers - write only */
#define L3_DA_VOLUME 0x00
#define L3_DA_DEEMPH 0x02
#define L3_DA_PWRCTL 0x03
#define L3_ST_SYSCLK 0x10

/*
 * shift 8 bits out
 */
void L3_TX8(uint8_t data)
{
	uint8_t cnt = 8;
	
	/* shift out 8 bits lsbit first */
	while(cnt--)
	{
		gpio_put(L3_DATA_PIN, data&1);
		sleep_us(1);
		gpio_put(L3_CLK_PIN, 0);
		sleep_us(1);
		gpio_put(L3_CLK_PIN, 1);
		sleep_us(1);
		data >>= 1;
	}
	sleep_us(2);
}

/**
  * @brief  Writes a Byte to a given register into the UDA1345
			through the control interface (I2C)
  * @param  Reg: The 7-bit address of the register to be written.
  * @param  Data: the 9-bit value to be written into destination register.
  * @retval 0 if correct communication, else wrong communication
  */
int32_t UDA1345_WriteRegister(uint8_t Reg, uint8_t Data)
{
	uint8_t add, dat;
	
	/* Assemble address & data in UDA1345 format */
	add = (UDA1345_L3_ADDR<<2) | ((Reg & 0x10) ? 0x02 : 0x00);
	dat = ((Reg & 0x03) << 6) | (Data & 0x3F);
	
	/* send address */
	gpio_put(L3_MODE_PIN, 0);
	L3_TX8(add);
	
	/* send data */
	gpio_put(L3_MODE_PIN, 1);
	L3_TX8(dat);
	
	printf("UDA1345_WriteRegister: write Reg 0x%02X = 0x%02X\n\r", Reg, Data);

	return 0;
}

/**
  * @brief  Resets the audio UDA1345. It restores the default configuration of the
  *         UDA1345 (this function shall be called before initializing the UDA1345).
  * @note   This function calls an external driver function: The IO Expander driver.
  * @param  None
  * @retval None
  */
int32_t UDA1345_Reset(void)
{
	UDA1345_WriteRegister(L3_DA_PWRCTL, 0x03);		// ADC on, DAC on
	UDA1345_WriteRegister(L3_DA_VOLUME, 0x00);		// Full volume
	UDA1345_WriteRegister(L3_DA_DEEMPH, 0x00);		// No Deemph, no mute
	UDA1345_WriteRegister(L3_ST_SYSCLK, 0x20);		// 256x, I2S, No DC blk
	return 0;
}

/*
 * set DAC volume in 1dB steps 
 */
int32_t UDA1345_Volume(int8_t vol)
{
	UDA1345_WriteRegister(L3_DA_VOLUME, vol);
	return 0;
}

/*
 * Mute DAC
 */
int32_t UDA1345_Mute(int8_t mute)
{
	UDA1345_WriteRegister(L3_DA_DEEMPH, mute ? 0x04 : 0x00);
	return 0;
}

/*
 * Do all hardware setup for UDA1345 including reset & config
 */
int32_t UDA1345_Init(void)
{
	/* setup bitbang output */
	gpio_init(L3_DATA_PIN);
	gpio_set_dir(L3_DATA_PIN, GPIO_OUT);
	gpio_put(L3_DATA_PIN, 0);
	
	gpio_init(L3_CLK_PIN);
	gpio_set_dir(L3_CLK_PIN, GPIO_OUT);
	gpio_put(L3_CLK_PIN, 1);
	
	gpio_init(L3_MODE_PIN);
	gpio_set_dir(L3_MODE_PIN, GPIO_OUT);
	gpio_put(L3_MODE_PIN, 1);
	
	sleep_us(10);
	
	UDA1345_Reset();
	
	return 0;
}

