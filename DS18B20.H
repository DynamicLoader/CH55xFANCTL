#ifndef __DS18B20_H__
#define __DS18B20_H__

#include "CH554.H"
#include "DEBUG.H"

// 引脚定义
sbit OneWire_DQ = P1 ^ 1;


typedef unsigned char uchar;


UINT16 DS18B20_ReadT(unsigned char* ROM);
// uchar SearchRomID(uchar RomID[4][8]);
void DS18B20_ConvertT(void);
void DS18B20_ReadRomCode(unsigned char* romcode);

#endif