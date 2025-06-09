/*
 * main.c - part of wm8731_i2s. Simple driver for WM8731 I2S codec
 * WM8731 requires full-duplex operation so the generic I2S
 * output driver provided for RPi RP2040 will not work for it.
 * 10-28-21 E. Brombaugh
 * Based on original ideas from
 * https://gist.github.com/jonbro/3da573315f066be8ea390db39256f9a7
 * and the pico-extras library.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "main.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/structs/clocks.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "pico/unique_id.h"
#include "i2s_fulldup.h"
#include "wm8731.h"
#include "aic3101.h"
#include "nau88c22.h"
#include "sgtl5000.h"
#include "uda1345.h"
#include "audio.h"
#include "led.h"
#include "button.h"

/* build version in simple format */
const char *fwVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/* blink timing for various states */
const uint8_t blink_time[3][6] =
{
	{10, 50, 0, 0, 0, 0},	// state 0 = 1 short
	{10, 10, 10, 50, 0, 0},	// state 1 = 2 short
	{10, 10, 50, 50, 0, 0},	// state 2 = 1 short 1 long
};	
uint8_t state, bt_idx;

/*
 * my version of sleep_ms() that may work after flashing
 */
void my_sleep_ms(uint64_t ms)
{
	/* convert ms to us */
	ms *= 1000;
	
	/* add to current time */
	ms += time_us_64();

	/* wait */
	while(time_us_64() < ms) {}
}

//#define TARGET_SYSCLK 159750
//#define TARGET_SYSCLK 61440

/*
 * main prog
 */
int main()
{
	int i;
	bool sysclk_stat;
	uint64_t led_time, cmd_time;
	pico_unique_board_id_t id_out;
	uint8_t codec_err = 0, cmd = 0;
	
	/* set sysclk prior to init serial */
	//sysclk_stat = set_sys_clock_khz(TARGET_SYSCLK, false);
	
    stdio_init_all();
	
	/*
	 * after flashing sleep_ms() never returns
	 * need to do HW reset to recover, or some sort of jank in OOCD scripts
	 * that I haven't figured out yet.
	 */
	my_sleep_ms(100);
	printf("\n\nRP2040 I2S Test\n");
	printf("CHIP_ID: 0x%08X\n\r", *(volatile uint32_t *)(SYSINFO_BASE));
	printf("BOARD_ID: 0x");
	pico_get_unique_board_id(&id_out);
	for(i=0;i<PICO_UNIQUE_BOARD_ID_SIZE_BYTES;i++)
		printf("%02X", id_out.id[i]);
	printf("\n");
	printf("Build Date: %s\n\r", bdate);
	printf("Build Time: %s\n\r", btime);
	printf("\n");
	
	/* setup buttons & LED */
	LEDInit();
	puts("LED Initialized\n");
	
	/* start button polling */
	button_init();
	puts("Button Initialized\n");

	/* init I2S I/O */
	init_i2s_fulldup();
	printf("I2S Initialized\n");
	
	/* init Audio AFTER I2S!!! - needs Fsample computed */
	Audio_Init();
	printf("Audio Initialized\n");
	
	/* init codec */
#if	defined(CODEC_WM8731)
	if(!WM8731_Init())
		printf("WM8731 Codec Initialized\n");
	else
	{
		printf("WM8731 Codec Init Failed...\n");
		codec_err = 1;
	}
#elif defined(CODEC_AIC3101)
	if(!AIC3101_Init())
	{
		printf("AIC3101 Codec Initialized\n");
		AIC3101_Dump_Regs();
	}
	else
	{
		printf("AIC3101 Codec Init Failed...\n");
		codec_err = 1;
	}
#elif defined(CODEC_NAU88C22)
	if(!NAU88C22_Init())
	{
		uint16_t id, rev;
		NAU88C22_ReadRegister(63, &id);
		NAU88C22_ReadRegister(62, &rev);
		printf("NAU88C22 Codec Initialized - ID = 0x%03X, Rev = 0x%03X\n", id, rev);
		//NAU88C22_Dump_Regs();
	}
	else
	{
		printf("NAU88C22 Codec Init Failed...\n");
		codec_err = 1;
	}
#elif defined(CODEC_SGTL5000)
	if(!SGTL5000_Init())
	{
		uint16_t id;
		SGTL5000_ReadRegister(0x0000, &id);
		printf("SGTL5000 Codec Initialized - ID = 0x%04X\n", id);
		//SGTL5000_Dump_Regs();
	}
	else
	{
		printf("SGTL5000 Codec Init Failed...\n");
		codec_err = 1;
	}
#elif defined(CODEC_UDA1345)
	if(!UDA1345_Init())
	{
		printf("UDA1345 Codec Initialized\n");
	}
	else
	{
		printf("UDA1345 Codec Init Failed...\n");
		codec_err = 1;
	}
#else
#error "Please define a codec in main.h"
#endif

	/* hangup if codec error */
	if(codec_err)
	{
		printf("Stalled...\n");
		while(1)
		{
			LEDToggle();
			my_sleep_ms(50);
		}
	}

	/* start blink sequence */
	state = 0;
	bt_idx = 0;
	led_time = time_us_64() + 10000 * blink_time[state][bt_idx++];
	LEDOn();
	
	/* loop here forever */
	printf("Looping\n\n");
	cmd_time = time_us_64() + 500000;
    while(true)
    {
		/* periodic LED toggle */
		if(time_us_64() >= led_time)
		{
			uint8_t ms = blink_time[state][bt_idx++];
			if(!ms)
			{
				bt_idx = 0;
				ms = blink_time[state][bt_idx++];
			}
			led_time = time_us_64() + 10000 * ms;
			LEDToggle();
		}
		
		/* check for button press */
		if(button_re())
		{
			/* advance state */
			state = (state+1)%3;
			Audio_Mode(state);
			printf("State %d\n", state);
			
			/* restart blink sequence */
			bt_idx = 0;
			led_time = time_us_64() + 10000 * blink_time[state][bt_idx++];
			LEDOn();
		}
		
		/* periodic Codec cmd */
		if(time_us_64() >= cmd_time)
		{
			cmd_time = time_us_64() + 50000;
			//UDA1345_Volume(cmd);
			//UDA1345_Mute(cmd);
			//cmd = cmd ^ 1;
			cmd++;
		}
	}
}