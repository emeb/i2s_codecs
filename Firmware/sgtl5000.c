/*
 * SGTL5000.c - SGTL5000 codec I2C control port driver for rp2040_audio
 * 06-03-25 E. Brombaugh
 */

#include <stdio.h>
#include "main.h"
#include "hardware/i2c.h"
#include "sgtl5000.h"

#define I2C_PORT i2c0
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17

/* The 7 bits SGTL5000 address (sent through I2C interface) */
#define SGTL5000_I2C_ADDR 0x0A

/* Register names */
#define CHIP_ID 0x0000
#define CHIP_DIG_POWER 0x0002
#define CHIP_CLK_CTRL 0x0004
#define CHIP_I2S_CTRL 0x0006
#define CHIP_SSS_CTRL 0x000A
#define CHIP_ADCDAC_CTRL 0x000E
#define CHIP_DAC_VOL 0x0010
#define CHIP_PAD_STRENGTH 0x0014
#define CHIP_ANA_ADC_CTRL 0x0020
#define CHIP_ANA_HP_CTRL 0x0022
#define CHIP_ANA_CTRL 0x0024
#define CHIP_LINREG_CTRL 0x0026
#define CHIP_REF_CTRL 0x0028
#define CHIP_MIC_CTRL 0x002A
#define CHIP_LINE_OUT_CTRL 0x002C
#define CHIP_LINE_OUT_VOL 0x002E
#define CHIP_ANA_POWER 0x0030
#define CHIP_PLL_CTRL 0x0032
#define CHIP_CLK_TOP_CTRL 0x0034
#define CHIP_ANA_STATUS 0x0036
#define CHIP_ANA_TEST1 0x0038
#define CHIP_ANA_TEST2 0x003A
#define CHIP_SHORT_CTRL 0x003C
#define DAP_CONTROL 0x0100
#define DAP_PEQ 0x0102
#define DAP_BASS_ENHANCE 0x0104
#define DAP_BASS_ENHANCE_CTRL 0x0106
#define DAP_AUDIO_EQ 0x0108
#define DAP_SGTL_SURROUND 0x010A
#define DAP_FILTER_COEF_ACCESS 0x010C
#define DAP_COEF_WR_B0_MSB 0x010E
#define DAP_COEF_WR_B0_LSB 0x0110
#define DAP_AUDIO_EQ_BASS_BAND0 0x0116
#define DAP_AUDIO_EQ_BAND1 0x0118
#define DAP_AUDIO_EQ_BAND2 0x011A
#define DAP_AUDIO_EQ_BAND3 0x011C
#define DAP_AUDIO_EQ_TREBLE_BAND4 0x011E
#define DAP_MAIN_CHAN 0x0120
#define DAP_MIX_CHAN 0x0122
#define DAP_AVC_CTRL 0x0124
#define DAP_AVC_THRESHOLD 0x0126
#define DAP_AVC_ATTACK 0x0128
#define DAP_AVC_DECAY 0x012A
#define DAP_COEF_WR_B1_MSB 0x012C
#define DAP_COEF_WR_B1_LSB 0x012E
#define DAP_COEF_WR_B2_MSB 0x0130
#define DAP_COEF_WR_B2_LSB 0x0132
#define DAP_COEF_WR_A1_MSB 0x0134
#define DAP_COEF_WR_A1_LSB 0x0136
#define DAP_COEF_WR_A2_MSB 0x0138
#define DAP_COEF_WR_A2_LSB 0x013A
#define SETTING_DELAY 0xFFFE
#define SETTING_EOF 0xFFFF

/* Initialization data */
static uint16_t codec_settings[] = 
{
	// Power configuration
	CHIP_DIG_POWER,		0x0000,	// all off during setup
	CHIP_CLK_CTRL,		0x0008,	// MCLK/1, 48khz, 256x
	CHIP_ANA_POWER,		0x7060, // Power up ADC st, DAC st, Ref
	SETTING_DELAY,		20,		// 10ms delay
	CHIP_LINREG_CTRL,	0x006C,	// Charge-pump uses VDDIO rail when > 3.1V
	
	// Reference voltages
	CHIP_REF_CTRL,		0x01F0,	// VAG ~VDDA/2, nominal bias, fast pop
	CHIP_LINE_OUT_CTRL,	0x0322,	// Lineout bias 1.65V, 0.36mA drive
	
	// power
	CHIP_ANA_POWER,		0x40EB, // Power up LINEOUT, HP, ADC, DAC, Ref
	CHIP_DIG_POWER,		0x0073,	// ADC, DAC, DAP, LINEOUT
	
	// line output volume
	CHIP_LINE_OUT_VOL,	0x0F0F,	// suggested values for 3.3V VDDA/VDDIO
	
	// Rate and format
	CHIP_CLK_CTRL,		0x0008,	// MCLK/1, 48khz, 256x
	CHIP_I2S_CTRL,		0x0130, // 16-bit I2S slave
	
#if 0
	// DAP on input 
	CHIP_SSS_CTRL,		0x0070,	// i2s->dap, dap->dac
	DAP_CONTROL,		0x0001,	// DAP enabled
	DAP_AUDIO_EQ,		0x0003,	// 5-band EQ
	DAP_AUDIO_EQ_BASS_BAND0,	0x004F,	// band 0
	DAP_AUDIO_EQ_BAND1,	0x003B,	// band 1
#else
	// normal routing
	CHIP_SSS_CTRL,		0x0010,	// i2s->dac, adc->i2s, no dap
#endif
	
	// Mutes
	CHIP_ADCDAC_CTRL,	0x0200, // Unmute DAC outputs
	CHIP_ANA_CTRL,		0x0026,	// Unmute Line Out, ADC in
	
	SETTING_EOF,				// EOF
};


/**
  * @brief  Writes a Byte to a given register into the SGTL5000
			through the control interface (I2C)
  * @param  Reg: The 7-bit address of the register to be written.
  * @param  Data: the 9-bit value to be written into destination register.
  * @retval 0 if correct communication, else wrong communication
  */
int32_t SGTL5000_WriteRegister(uint16_t Reg, uint16_t Data)
{
	int32_t status;
	uint8_t i2c_msg[4];

	/* Assemble 4-byte data in SGTL5000 format */
    i2c_msg[0] = Reg >> 8;
	i2c_msg[1] = Reg&0xFF;
    i2c_msg[2] = Data >> 8;
	i2c_msg[3] = Data&0xFF;

	status = i2c_write_timeout_us(I2C_PORT, SGTL5000_I2C_ADDR, i2c_msg, 4, false, 10000);

	/* Check the communication status */
	if(status != 4)
	{
		/* Reset the I2C communication bus */
		printf("SGTL5000_WriteRegister: write to DevAddr 0x%02X / Reg 0x%04X failed - resetting.\n\r",
			SGTL5000_I2C_ADDR, Reg);
		
		i2c_deinit(I2C_PORT);
		i2c_init(I2C_PORT, 100*1000);
		
		return 1;
	}
	
	printf("SGTL5000_WriteRegister: write to DevAddr 0x%02X / Reg 0x%04X = 0x%04X\n\r",
		SGTL5000_I2C_ADDR, Reg, Data);

	return 0;
}

/**
  * @brief  Writes a Byte to a given register into the SGTL5000
			through the control interface (I2C)
  * @param  Reg: The 7-bit address of the register to be written.
  * @param  Data: the 9-bit value to be written into destination register.
  * @retval 0 if correct communication, else wrong communication
  */
int32_t SGTL5000_ReadRegister(uint16_t Reg, uint16_t *Data)
{
	int32_t status;
	uint8_t i2c_msg[2];

	/* send register address in two bytes with nostop */
    i2c_msg[0] = Reg >> 8;
	i2c_msg[1] = Reg&0xFF;

	status = i2c_write_timeout_us(I2C_PORT, SGTL5000_I2C_ADDR, i2c_msg, 2, true, 10000);

	/* Check the communication status */
	if(status != 2)
	{
		/* Reset the I2C communication bus */
		printf("SGTL5000_ReadRegister: send reg to DevAddr 0x%02X / Reg 0x%04X failed - resetting.\n\r",
			SGTL5000_I2C_ADDR, Reg);
		
		i2c_deinit(I2C_PORT);
		i2c_init(I2C_PORT, 100*1000);
		
		return 1;
	}
	
	/* get 2-byte read data */
	status = i2c_read_timeout_us(I2C_PORT, SGTL5000_I2C_ADDR, i2c_msg, 2, false, 10000);

	/* Check the communication status */
	if(status != 2)
	{
		/* Reset the I2C communication bus */
		printf("SGTL5000_ReadRegister: get data from DevAddr 0x%02X failed - resetting.\n\r",
			SGTL5000_I2C_ADDR);
		
		i2c_deinit(I2C_PORT);
		i2c_init(I2C_PORT, 100*1000);
		
		return 1;
	}

	/* assemble 9-bit result */
	*Data = (i2c_msg[0]<<8) | i2c_msg[1];
	
	printf("SGTL5000_ReadRegister: read from DevAddr 0x%02X / Reg 0x%04X = 0x%04X\n\r",
		SGTL5000_I2C_ADDR, Reg, *Data);

	return 0;
}

/**
  * @brief  Resets the audio SGTL5000. It restores the default configuration of the
  *         SGTL5000 (this function shall be called before initializing the SGTL5000).
  * @note   This function calls an external driver function: The IO Expander driver.
  * @param  None
  * @retval None
  */
int32_t SGTL5000_Reset(void)
{
	uint8_t idx = 0, tries;
	uint16_t reg, data;
	int32_t result = 0;
	
	while((reg = codec_settings[2*idx]) != SETTING_EOF)
	{
		data = codec_settings[2*idx + 1];
		if(reg != SETTING_DELAY)
		{
			/* valid register - write data */
			/*
			 * NOTE: Sometimes writing reg 1 fails. Seems to succeed on 2nd try.
			 */
			tries = 0;
			while((SGTL5000_WriteRegister(reg, data)!=0) && (tries++ <5));
			if(tries > 4)
				result++;
		}
		else
		{
			/* time delay */
			printf("SGTL5000_Reset: delay %d ms\n\r", data);
			my_sleep_ms(data);
		}
		
		idx++;
	}

	return result;
}

/*
 * diagnostic to spew all the registers
 */
int32_t SGTL5000_Dump_Regs(void)
{
	uint16_t reg = 0;
	uint16_t data;
	printf("Dumping SGTL5000 registers...\n");
	while(reg < 64)
	{
		SGTL5000_ReadRegister(reg, &data);
		reg+=2;
	}
}

/*
 * Do all hardware setup for SGTL5000 including reset & config
 */
int32_t SGTL5000_Init(void)
{
	/* setup i2c */
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    i2c_init(I2C_PORT, 100*1000);
	
	return SGTL5000_Reset();
}

