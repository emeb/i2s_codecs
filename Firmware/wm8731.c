/*
 * wm8731.h - WM8731 codec I2C control port driver for rp2040_audio
 * 10-28-21 E. Brombaugh
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "wm8731.h"

#define I2C_PORT i2c0
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17
#define CSB_PIN 20

/* The 7 bits WM8731 address (sent through I2C interface) */
#define W8731_ADDR_0 0x1A

/* how many usable regs for shadow regs */
#define W8731_NUM_REGS 10

enum wm8731_regs
{
	REG_LLIN,
	REG_RLIN,
	REG_LHP,
	REG_RHP,
	REG_APATH,
	REG_DPATH,
	REG_PCTL,
	REG_DAIF,
	REG_SMPL,
	REG_ACT,
	REG_RST = 0x0f,
	REG_EOF = 0xffff
};

/* configuration list */
const uint16_t w8731_init_data[] =
{
	REG_RST, 	0x000,	// Reg 0F: soft reset
	REG_LLIN, 	0x017,	// Reg 00: Left Line In (0dB, mute off)
	REG_RLIN,	0x017,	// Reg 01: Right Line In (0dB, mute off)
	REG_LHP,	0x079,	// Reg 02: Left Headphone out (0dB)
	REG_RHP,	0x079,	// Reg 03: Right Headphone out (0dB)
	REG_APATH,	0x012,	// Reg 04: Analog Audio Path Control (DAC sel, Mute Mic)
	REG_DPATH,	0x000,	// Reg 05: Digital Audio Path Control (mute on = 0x8)
//	REG_PCTL,	0x062,	// Reg 06: Power Down Control (Clkout, Osc, Mic Off)
	REG_PCTL,	0x022,	// Reg 06: Power Down Control (Osc, Mic Off)
	REG_DAIF,	0x002,	// Reg 07: Digital Audio Interface Format (msb, 16-bit, slave, I2S)
	REG_SMPL,	0x000,	// Reg 08: Sampling Control (Normal, 256x, 48k ADC/DAC)
	REG_ACT,	0x001,	// Reg 09: Active Control
	REG_EOF				// End of list
};

uint16_t w8731_shadow[W8731_NUM_REGS];

/*
 * Do all hardware setup for WM8731 including reset & config
 */
int32_t WM8731_Init(void)
{
	/* setup i2c */
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    i2c_init(I2C_PORT, 100*1000);
	
	/* configure CSB for low state => I2C Addr 0 = 0 */
	gpio_init(CSB_PIN);
	gpio_set_dir(CSB_PIN, GPIO_OUT);
	gpio_put(CSB_PIN, 0);
	
	return WM8731_Reset();
}

/**
  * @brief  Writes a Byte to a given register into the WM8731 audio WM8731
			through the control interface (I2C)
  * @param  RegisterAddr: The address (location) of the register to be written.
  * @param  RegisterValue: the 9-bit value to be written into destination register.
  * @retval 0 if correct communication, else wrong communication
  */
int32_t WM8731_WriteRegister(uint16_t DevAddr, uint8_t RegisterAddr,
	uint16_t RegisterValue)
{
	int32_t status;
	uint8_t i2c_msg[2];

	/* update shadow regs */
	if(RegisterAddr<W8731_NUM_REGS)
		w8731_shadow[RegisterAddr] = RegisterValue;

	/* Assemble 2-byte data in WM8731 format */
    i2c_msg[0] = ((RegisterAddr<<1)&0xFE) | ((RegisterValue>>8)&0x01);
	i2c_msg[1] = RegisterValue&0xFF;

	status = i2c_write_timeout_us(I2C_PORT, DevAddr, i2c_msg, 2, false, 10000);

	/* Check the communication status */
	if(status != 2)
	{
		/* Reset the I2C communication bus */
		printf("WM8731_WriteRegister: write to DevAddr 0x%02X / RegisterAddr 0x%02X failed - resetting.\n\r",
			DevAddr, RegisterAddr);
		
		i2c_deinit(I2C_PORT);
		i2c_init(I2C_PORT, 100*1000);
		
		return 1;
	}
	
	printf("WM8731_WriteRegister: write to DevAddr 0x%02X / RegisterAddr 0x%02X = 0x%03X\n\r",
		DevAddr, RegisterAddr, RegisterValue);

	return 0;
}

/**
  * @brief  Resets the audio WM8731. It restores the default configuration of the
  *         WM8731 (this function shall be called before initializing the WM8731).
  * @note   This function calls an external driver function: The IO Expander driver.
  * @param  None
  * @retval None
  */
int32_t WM8731_Reset(void)
{
	int32_t result = 0;
	uint16_t reg, data;
	uint8_t idx = 0, tries;

	/* Load reg/data pairs from table */
	while((reg = w8731_init_data[2*idx]) != REG_EOF)
	{
		data =  w8731_init_data[2*idx+1];
		tries = 0;
		while((WM8731_WriteRegister((W8731_ADDR_0), reg, data)!=0) && (tries++ <5));
		if(tries>4)
            result++;
		idx++;
	}

	return result;
}

/*
 * mute/unmute the WM8731 outputs
 */
void WM8731_Mute(uint8_t enable)
{
	uint8_t mute = enable ? 0x08 : 0x00;

	/* send mute cmd */
	WM8731_WriteRegister((W8731_ADDR_0), REG_DPATH, mute);

	/* wait a bit for it to complete */
	sleep_ms(20);
}

/*
 * set WM8731 headphone volume
 */
void WM8731_HPVol(uint8_t vol)
{
	/* set both chls volume */
	WM8731_WriteRegister((W8731_ADDR_0), REG_LHP, 0x180 | (vol & 0x7f));
}

/*
 * set WM8731 Input Source
 */
void WM8731_InSrc(uint8_t src)
{
	uint16_t temp_reg;

	/* set line or mic input */
	temp_reg = (w8731_shadow[REG_APATH] & 0x1F9) | (src ? 0x04 : 0x02);
	WM8731_WriteRegister((W8731_ADDR_0), REG_APATH, temp_reg);

	/* set line/mic power */
	//temp_reg = (w8731_shadow[REG_PCTL] & 0x1FC) | (src ? 0x01 : 0x02);
	//WM8731_WriteRegister(((W8731_ADDR_0)<<1), REG_PCTL, temp_reg);
}

/*
 * set WM8731 Input volume
 */
void WM8731_InVol(uint8_t vol)
{
	/* set both chls volume */
	WM8731_WriteRegister((W8731_ADDR_0), REG_LLIN, 0x100 | (vol & 0x3f));
}

/*
 * set WM8731 Mic Boost
 */
void WM8731_MicBoost(uint8_t boost)
{
	uint16_t temp_reg;

	/* set/clear mic boost bit */
	temp_reg = (w8731_shadow[REG_APATH] & 0x1FE) | (boost ? 1 : 0);
	WM8731_WriteRegister((W8731_ADDR_0), REG_APATH, temp_reg);
}
