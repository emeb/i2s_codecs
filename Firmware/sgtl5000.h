/*
 * SGTL5000.h - SGTL5000 codec I2C control port driver for RP2040
 * 06-03-25 E. Brombaugh
 */

#ifndef __SGTL5000__
#define __SGTL5000__

int32_t SGTL5000_WriteRegister(uint16_t Reg, uint16_t Data);
int32_t SGTL5000_ReadRegister(uint16_t Reg, uint16_t *Data);
int32_t SGTL5000_Reset(void);
int32_t SGTL5000_Dump_Regs(void);
int32_t SGTL5000_Init(void);

#endif
