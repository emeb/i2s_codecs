/*
 * aic3101.h - AIC3101 codec I2C control port driver for RP2040
 * 10-28-21 E. Brombaugh
 */

#ifndef __aic3101__
#define __aic3101__

int32_t AIC3101_Init(void);
int32_t AIC3101_Reset(void);
int32_t AIC3101_WriteRegister(uint8_t RegisterAddr, uint8_t RegisterValue);
int32_t AIC3101_ReadRegister(uint8_t RegisterAddr, uint8_t *RegisterValue);
int32_t AIC3101_Dump_Regs(void);

#endif
