/*
 * audio.c - G081 audio processing
 * 10-10-2021 E. Brombaugh
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "audio.h"

#define WAV_PHS 10
#define WAV_LEN (1<<WAV_PHS)
#define INTERP_BITS 10

int32_t phs, frq;
int16_t sinetab[1024];
volatile uint8_t core0_mode, core1_mode;

/*
 * init audio handler
 */
void Audio_Init(void)
{
	/* init audio mode */
	core0_mode = core1_mode = 0;
	
	/* starting phase and freq */
	phs = 0;
	frq = (int32_t)floorf(100.0F * powf(2.0F, 32.0F) / (float)Fsample);
	//frq = 0x000f0000;
	
	printf("Audio_Init: Fsample = %d, frq = 0x%08X\n", frq);
	
	/* build sinewave LUT */
	float th = 0.0F, thinc = 6.2832F/((float)WAV_LEN);
	for(int i=0;i<WAV_LEN;i++)
	{
		sinetab[i] = floorf(32767.0F * sinf(th) + 0.5F);
		th += thinc;
	}
}

/*
 * Request mute on/off and block until complete
 */
void Audio_Set_Mute(uint8_t enable)
{
}

/*
 * disable core 1
 */
void Audio_Disable_Core(uint8_t disable)
{
	if(disable)
	{
		multicore_lockout_start_timeout_us(1000);
	}
	else
	{
		multicore_lockout_end_timeout_us(1000);
	}
}

/*
 * Audio foreground task - run by core 1
 */
void Audio_Fore(void)
{
	/* update mode */
	core1_mode = core0_mode;
}

/*
 * buffer math:
 * SMPS = 32 (same as stereo "FRAMES")
 * CHLS = 2
 * BUFSZ = SMPS*CHLS = 64
 * FRAMES_PER_BUFFER = BUFSZ/2 = SMPS = 32 (in 32-bit words)
 * length(I2S inbuf) = FRAMES_PER_BUFFER * 2 = 64 total (both halves) but 32-bit words
 * length(src) = length(I2S inbuf/2) = SMPS = 32 in 32-bit words
 * i iterations = SMPS = 32
 * src increments = i iterations * 2 = 64
 */

/*
 * change the audio generation mode
 */
void Audio_Mode(uint8_t new_mode)
{
	/* check for change needed */
	if((new_mode == core0_mode) || (new_mode >= AUDIO_MODES))
		return;
	
	/* change foreground mode */
	core0_mode = new_mode;
	
	/* wait for new mode to go live */
	while(core0_mode != core1_mode)
	{
	}
}

/*
 * sine waveform interp
 */
int16_t sine_interp(uint32_t phs)
{
	
	int32_t a, b, sum;
	uint32_t ip, fp;
	
	ip = phs>>(32-WAV_PHS);
	a = sinetab[ip];
	b = sinetab[(ip + 1)&(WAV_LEN-1)];
	
	fp = (phs & ((1<<(32-WAV_PHS))-1)) >> ((32-WAV_PHS)-INTERP_BITS);
	sum = b * fp;
	sum += a * (((1<<INTERP_BITS)-1)-fp);
	
	return sum >> INTERP_BITS; 
}

/*
 * handle new buffer of ADC data
 */
void __not_in_flash_func(Audio_Proc)(volatile int16_t *dst, volatile int16_t *src, int32_t len)
{
	int16_t wave;
	
	switch(core1_mode)
	{
		default:
		case 0:
		case 1:
			/* saw & sine gen */
			while(len)
			{
				if(core1_mode == 0)
					wave = phs >> 16;	// saw
				else
					wave = sine_interp((uint32_t)phs);		// sine
				
				*dst++ = wave;
				*dst++ = -wave;
				
				phs += frq;
				len-=2;
			}
			break;
				
		case 2:
			/* just pass-thru */
			while(len--)
				*dst++ = *src++;
			break;
	}
}
