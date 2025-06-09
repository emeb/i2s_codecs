/*
 * UDA1345.h - UDA1345 codec bitbang control port driver for RP2040
 * 06-08-25 E. Brombaugh
 */

#ifndef __UDA1345__
#define __UDA1345__

int32_t UDA1345_WriteRegister(uint8_t Reg, uint8_t Data);
int32_t UDA1345_Reset(void);
int32_t UDA1345_Volume(int8_t vol);
int32_t UDA1345_Mute(int8_t mute);
int32_t UDA1345_Init(void);

#endif
