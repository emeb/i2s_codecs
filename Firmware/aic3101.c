/*
 * aic3101.h - AIC3101 codec I2C control port driver for rp2040_audio
 * 10-28-21 E. Brombaugh
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "aic3101.h"

#define I2C_PORT i2c0
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17
#define CSB_PIN 20

/* The 7 bits AIC3101 address (sent through I2C interface) */
#define AIC3101_ADDR 0x18

/* Codec register settings. contents is ADDR, DATA per line */
#define REG_EOF 0xff
#if 0
/* With PLL, with Int MCLK */
static uint8_t codec_settings[] = 
{
	3,		0x91,	// PLL A - PLL ena, Q=2, P=1
	4,		0x80,	// PLL B - J=32 : PLL rate = ((32.0 * 2) / (1 * 8)) * BCLK = Fs*256
	7,		0x0A,	// datapath setup - left dac/left in, right dac/right in
	11,		0x02,	// ovfl setup - PLL R = 2
	15,		0x00,	// Left PGA - unmuted, 0dB
	16,		0x00,	// Right PGA - unmuted, 0dB
	19,		0x04,	// Left ADC - enabled, MIC1LP single, 0dB
	22,		0x04,	// Right ADC - enabled, MIC1RP single, 0dB
	37,		0xC0,	// DAC Power - left/right DACs enabled
	41,		0x50,	// DAC Output Switching - use L3/R3 & independent vol
	43,		0x00,	// Left DAC - unmuted, 0dB
	44,		0x00,	// Right DAC - unmuted, 0dB
	86,		0x09,	// Left LOP/M - umuted, 0dB, enabled (NOTE - DS error, bit 0 is R/W)
	93,		0x09,	// Right LOP/M - umuted, 0dB, enabled (NOTE - DS error, bit 0 is R/W)
	102,	0xA2,	// Clockgen - CLKDIV_IN uses BCLK, PLLDIV_IN uses BCLK
	109,	0xC0,	// DAC Current - 100% increase over default
	REG_EOF,		// EOF
};
#else
/* No PLL, with Ext MCLK */
static uint8_t codec_settings[] = 
{
	7,		0x0A,	// datapath setup - left dac/left in, right dac/right in
	19,		0x04,	// Left ADC - enabled, 0dB
	15,		0x00,	// Left PGA - unmuted, 0dB
	16,		0x00,	// Right PGA - unmuted, 0dB
	19,		0x04,	// Left ADC - enabled, MIC1LP single, 0dB
	22,		0x04,	// Right ADC - enabled, 0dB
	37,		0xC0,	// DAC Power - left/right DACs enabled
	41,		0x50,	// DAC Output Switching - use L3/R3 & independent vol
	43,		0x00,	// Left DAC - unmuted, 0dB
	44,		0x00,	// Right DAC - unmuted, 0dB
	86,		0x09,	// Left LOP/M - umuted, 0dB, enabled (NOTE - DS error, bit 0 is R/W)
	93,		0x09,	// Right LOP/M - umuted, 0dB, enabled (NOTE - DS error, bit 0 is R/W)
	101,	0x01,	// Clock - CODEC_CLKIN uses CLKDIV_OUT
	109,	0xC0,	// DAC Current - 100% increase over default
	REG_EOF,		// EOF
};
#endif

/*
 * Do all hardware setup for AIC3101 including reset & config
 */
int32_t AIC3101_Init(void)
{
	/* setup i2c */
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    i2c_init(I2C_PORT, 100*1000);
	
	/* configure CSB for high state => no reset */
	gpio_init(CSB_PIN);
	gpio_set_dir(CSB_PIN, GPIO_OUT);
	gpio_put(CSB_PIN, 1);
	
	return AIC3101_Reset();
}

/**
  * @brief  Writes a Byte to a given register into the AIC3101 audio AIC3101
			through the control interface (I2C)
  * @param  RegisterAddr: The address (location) of the register to be written.
  * @param  RegisterValue: the value to be written into destination register.
  * @retval 0 if correct communication, else wrong communication
  */
int32_t AIC3101_WriteRegister(uint8_t RegisterAddr, uint8_t RegisterValue)
{
	int32_t status;
	uint8_t i2c_msg[2];

	/* Assemble 2-byte data in AIC3101 format */
    i2c_msg[0] = RegisterAddr;
	i2c_msg[1] = RegisterValue;

	status = i2c_write_timeout_us(I2C_PORT, AIC3101_ADDR, i2c_msg, 2, false, 10000);

	/* Check the communication status */
	if(status != 2)
	{
		/* Reset the I2C communication bus */
		printf("AIC3101_WriteRegister: write to DevAddr 0x%02X / RegisterAddr 0x%02X failed - resetting.\n\r",
			AIC3101_ADDR, RegisterAddr);
		
		i2c_deinit(I2C_PORT);
		i2c_init(I2C_PORT, 100*1000);
		
		return 1;
	}
	
	printf("AIC3101_WriteRegister: write to DevAddr 0x%02X / RegisterAddr 0x%02X = 0x%02X\n\r",
		AIC3101_ADDR, RegisterAddr, RegisterValue);

	return 0;
}

/**
  * @brief  Reads a Byte from a given register of the AIC3101
			through the control interface (I2C)
  * @param  Reg: The 7-bit address of the register to be written.
  * @param  Data: the 9-bit value to be written into destination register.
  * @retval 0 if correct communication, else wrong communication
  */
int32_t AIC3101_ReadRegister(uint8_t RegisterAddr, uint8_t *RegisterValue)
{
	int32_t status;
	uint8_t i2c_msg;

	status = i2c_write_timeout_us(I2C_PORT, AIC3101_ADDR, &RegisterAddr, 1, true, 10000);

	/* Check the communication status */
	if(status != 1)
	{
		/* Reset the I2C communication bus */
		printf("AIC3101_ReadRegister: send reg to DevAddr 0x%02X / Reg %2d failed - resetting.\n\r",
			AIC3101_ADDR, RegisterAddr);
		
		i2c_deinit(I2C_PORT);
		i2c_init(I2C_PORT, 100*1000);
		
		return 1;
	}
	
	/* get 1-byte read data */
	status = i2c_read_timeout_us(I2C_PORT, AIC3101_ADDR, RegisterValue, 1, false, 10000);

	/* Check the communication status */
	if(status != 1)
	{
		/* Reset the I2C communication bus */
		printf("AIC3101_ReadRegister: get data from DevAddr 0x%02X failed - resetting.\n\r",
			AIC3101_ADDR);
		
		i2c_deinit(I2C_PORT);
		i2c_init(I2C_PORT, 100*1000);
		
		return 1;
	}
	
	printf("AIC3101_ReadRegister: read from DevAddr 0x%02X / Reg %2d = 0x%0X\n\r",
		AIC3101_ADDR, RegisterAddr, *RegisterValue);

	return 0;
}

/**
  * @brief  Resets the audio AIC3101. It restores the default configuration of the
  *         AIC3101 (this function shall be called before initializing the AIC3101).
  * @note   This function calls an external driver function: The IO Expander driver.
  * @param  None
  * @retval None
  */
int32_t AIC3101_Reset(void)
{
	int32_t result = 0;
	uint8_t reg, data;
	uint8_t idx = 0, tries;

	/* hardware reset */
	gpio_put(CSB_PIN, 0);
	sleep_ms(1);
	gpio_put(CSB_PIN, 1);
	sleep_ms(1);

	/* Load reg/data pairs from table */
	while((reg = codec_settings[2*idx]) != REG_EOF)
	{
		data =  codec_settings[2*idx+1];
		tries = 0;
		while((AIC3101_WriteRegister(reg, data)!=0) && (tries++ <5));
		if(tries>4)
            result++;
		idx++;
	}

	return result;
}

/*
 * diagnostic to spew all the registers
 */
int32_t AIC3101_Dump_Regs(void)
{
	uint8_t reg = 0, data;
	printf("Dumping AIC3101 registers...\n");
	while(reg < 109)
	{
		AIC3101_ReadRegister(reg, &data);
		reg++;
	}
}

